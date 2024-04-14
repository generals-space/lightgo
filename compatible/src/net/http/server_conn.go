package http

import (
	"bufio"
	"crypto/tls"
	"errors"
	"io"
	"io/ioutil"
	"log"
	"net"
	"runtime"
	"strings"
	"sync"
	"sync/atomic"
	"time"
)

// This should be >= 512 bytes for DetectContentType,
// but otherwise it's somewhat arbitrary.
const bufferBeforeChunkingSize = 2048

// eofReader is a non-nil io.ReadCloser that always returns EOF.
// It embeds a *strings.Reader so it still has a WriteTo method
// and io.Copy won't need a buffer.
var eofReader = &struct {
	*strings.Reader
	io.Closer
}{
	strings.NewReader(""),
	ioutil.NopCloser(nil),
}

////////////////////////////////////////////////////////////////////////////////

// 这是 net/http 的核心框架, 起到了承前启后的作用.
//
// 前承 socket server 通过 accept 生成 connect socket, 并从其中读取 http 协议内容, 
// 构造 request 对象, 生成并调用 serverHandler.ServeHTTP(); 
// 后启开发者声明的 handler, 
//
// 	@implementOf: Handler
//
// serverHandler delegates to either the server's Handler or
// DefaultServeMux and also handles "OPTIONS *" requests.
type serverHandler struct {
	srv *Server
}

// caller:
// 	1. conn.serve()
func (sh serverHandler) ServeHTTP(rw ResponseWriter, req *Request) {
	handler := sh.srv.Handler
	if handler == nil {
		handler = DefaultServeMux
	}
	if req.RequestURI == "*" && req.Method == "OPTIONS" {
		handler = globalOptionsHandler{}
	}
	handler.ServeHTTP(rw, req)
}

////////////////////////////////////////////////////////////////////////////////

// initNPNRequest is an HTTP handler that initializes certain
// uninitialized fields in its *Request. Such partially-initialized
// Requests come from NPN protocol handlers.
type initNPNRequest struct {
	c *tls.Conn
	h serverHandler
}

func (h initNPNRequest) ServeHTTP(rw ResponseWriter, req *Request) {
	if req.TLS == nil {
		req.TLS = &tls.ConnectionState{}
		*req.TLS = h.c.ConnectionState()
	}
	if req.Body == nil {
		req.Body = eofReader
	}
	if req.RemoteAddr == "" {
		req.RemoteAddr = h.c.RemoteAddr().String()
	}
	h.h.ServeHTTP(rw, req)
}

var errTooLarge = errors.New("http: request too large")

// conn 存储着 connect socket 的相关信息
//
// A conn represents the server side of an HTTP connection.
type conn struct {
	// remoteAddr 格式为 IP:port, 如 127.0.0.1:46100
	// network address of remote side
	remoteAddr string
	server     *Server // the Server on which the connection arrived
	// connect socket 对象.
	// i/o connection
	rwc      net.Conn
	sr       liveSwitchReader     // where the LimitReader reads from; usually the rwc
	lr       *io.LimitedReader    // io.LimitReader(sr)
	buf      *bufio.ReadWriter    // buffered(lr,rwc), reading from bufio->limitReader->sr->rwc
	tlsState *tls.ConnectionState // or nil when not using TLS

	mu           sync.Mutex // guards the following
	clientGone   bool       // if client has disconnected mid-request
	closeNotifyc chan bool  // made lazily
	hijackedv    bool       // connection has been hijacked by handler

	//	@compatible: addAt v1.8
	curState atomic.Value // of ConnState
}

func (c *conn) hijacked() bool {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.hijackedv
}

// hijack 劫持
//
// caller:
// 	1. compatible/src/net/http/server_response.go -> response.Hijack()
func (c *conn) hijack() (rwc net.Conn, buf *bufio.ReadWriter, err error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.hijackedv {
		return nil, nil, ErrHijacked
	}
	if c.closeNotifyc != nil {
		return nil, nil, errors.New("http: Hijack is incompatible with use of CloseNotifier")
	}
	c.hijackedv = true
	rwc = c.rwc
	buf = c.buf
	c.rwc = nil
	c.buf = nil
	return
}

func (c *conn) closeNotify() <-chan bool {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.closeNotifyc == nil {
		c.closeNotifyc = make(chan bool, 1)
		if c.hijackedv {
			// to obey the function signature, even though
			// it'll never receive a value.
			return c.closeNotifyc
		}
		pr, pw := io.Pipe()

		readSource := c.sr.r
		c.sr.Lock()
		c.sr.r = pr
		c.sr.Unlock()
		go func() {
			_, err := io.Copy(pw, readSource)
			if err == nil {
				err = io.EOF
			}
			pw.CloseWithError(err)
			c.noteClientGone()
		}()
	}
	return c.closeNotifyc
}

func (c *conn) noteClientGone() {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.closeNotifyc != nil && !c.clientGone {
		c.closeNotifyc <- true
	}
	c.clientGone = true
}

// caller:
// 	1. conn.serve()
//
// Read next request from connection.
func (c *conn) readRequest() (w *response, err error) {
	if c.hijacked() {
		return nil, ErrHijacked
	}

	if d := c.server.ReadTimeout; d != 0 {
		c.rwc.SetReadDeadline(time.Now().Add(d))
	}
	if d := c.server.WriteTimeout; d != 0 {
		defer func() {
			c.rwc.SetWriteDeadline(time.Now().Add(d))
		}()
	}

	c.lr.N = int64(c.server.maxHeaderBytes()) + 4096 /* bufio slop */
	var req *Request
	if req, err = ReadRequest(c.buf.Reader); err != nil {
		if c.lr.N == 0 {
			return nil, errTooLarge
		}
		return nil, err
	}
	c.lr.N = noLimit

	req.RemoteAddr = c.remoteAddr
	req.TLS = c.tlsState

	w = &response{
		conn:          c,
		req:           req,
		handlerHeader: make(Header),
		contentLength: -1,
	}
	w.cw.res = w
	w.w = newBufioWriterSize(&w.cw, bufferBeforeChunkingSize)
	return w, nil
}

func (c *conn) finalFlush() {
	if c.buf != nil {
		c.buf.Flush()

		// Steal the bufio.Reader (~4KB worth of memory) and its associated
		// reader for a future connection.
		putBufioReader(c.buf.Reader)

		// Steal the bufio.Writer (~4KB worth of memory) and its associated
		// writer for a future connection.
		putBufioWriter(c.buf.Writer)

		c.buf = nil
	}
}

// Close the connection.
func (c *conn) close() {
	c.finalFlush()
	if c.rwc != nil {
		c.rwc.Close()
		c.rwc = nil
	}
}

// rstAvoidanceDelay is the amount of time we sleep after closing the
// write side of a TCP connection before closing the entire socket.
// By sleeping, we increase the chances that the client sees our FIN
// and processes its final data before they process the subsequent RST
// from closing a connection with known unread data.
// This RST seems to occur mostly on BSD systems. (And Windows?)
// This timeout is somewhat arbitrary (~latency around the planet).
const rstAvoidanceDelay = 500 * time.Millisecond

// closeWrite flushes any outstanding data and sends a FIN packet (if
// client is connected via TCP), signalling that we're done.  We then
// pause for a bit, hoping the client processes it before `any
// subsequent RST.
//
// See http://golang.org/issue/3595
func (c *conn) closeWriteAndWait() {
	c.finalFlush()
	if tcp, ok := c.rwc.(*net.TCPConn); ok {
		tcp.CloseWrite()
	}
	time.Sleep(rstAvoidanceDelay)
}

// validNPN reports whether the proto is not a blacklisted Next
// Protocol Negotiation protocol.  Empty and built-in protocol types
// are blacklisted and can't be overridden with alternate
// implementations.
func validNPN(proto string) bool {
	switch proto {
	case "", "http/1.1", "http/1.0":
		return false
	}
	return true
}

// 	@compatible: 该方法在 v1.3 版本初次添加
//
func (c *conn) setState(nc net.Conn, state ConnState) {
	if hook := c.server.ConnState; hook != nil {
		hook(nc, state)
	}
}

// serve 这里是实际处理 connect socket 的地方(tcp层面), 从 socket 中读取 http 协议内容,
// 构造 Request() 对象, 再转交给 http server.
//
// caller:
// 	1. Server.Serve() listen socket 每通过 accept() 接收到一个请求, 都会开一个协程
// 	调用该方法对这个请求进行处理, conn{} 对象中包含了该连接中的所有信息.
//
// Serve a new connection.
func (c *conn) serve() {
	defer func() {
		if err := recover(); err != nil {
			const size = 4096
			buf := make([]byte, size)
			buf = buf[:runtime.Stack(buf, false)]
			log.Printf("http: panic serving %v: %v\n%s", c.remoteAddr, err, buf)
		}
		if !c.hijacked() {
			c.close()
		}
	}()

	// 如果是 https, 则进入该块, 进行额外的握手处理.
	if tlsConn, ok := c.rwc.(*tls.Conn); ok {
		if d := c.server.ReadTimeout; d != 0 {
			c.rwc.SetReadDeadline(time.Now().Add(d))
		}
		if d := c.server.WriteTimeout; d != 0 {
			c.rwc.SetWriteDeadline(time.Now().Add(d))
		}
		if err := tlsConn.Handshake(); err != nil {
			return
		}
		c.tlsState = new(tls.ConnectionState)
		*c.tlsState = tlsConn.ConnectionState()
		if proto := c.tlsState.NegotiatedProtocol; validNPN(proto) {
			if fn := c.server.TLSNextProto[proto]; fn != nil {
				h := initNPNRequest{tlsConn, serverHandler{c.server}}
				fn(c.server, tlsConn, h)
			}
			return
		}
	}

	for {
		// 从 connect socket 中读取 http 协议信息, 构造 request 对象,
		// 将其赋值到 response{} 中的成员字段并返回.
		resp, err := c.readRequest()
		if err != nil {
			if err == errTooLarge {
				// Their HTTP client may or may not be able to read this
				// if we're responding to them and hanging up
				// while they're still writing their request. 
				// Undefined behavior.
				io.WriteString(c.rwc, "HTTP/1.1 413 Request Entity Too Large\r\n\r\n")
				c.closeWriteAndWait()
				break
			} else if err == io.EOF {
				break // Don't reply
			} else if neterr, ok := err.(net.Error); ok && neterr.Timeout() {
				break // Don't reply
			}
			io.WriteString(c.rwc, "HTTP/1.1 400 Bad Request\r\n\r\n")
			break
		}

		// Expect 100 Continue support
		req := resp.req
		if req.expectsContinue() {
			if req.ProtoAtLeast(1, 1) {
				// Wrap the Body reader with one that replies on the connection
				req.Body = &expectContinueReader{readCloser: req.Body, resp: resp}
			}
			if req.ContentLength == 0 {
				resp.Header().Set("Connection", "close")
				resp.WriteHeader(StatusBadRequest)
				resp.finishRequest()
				break
			}
			req.Header.Del("Expect")
		} else if req.Header.get("Expect") != "" {
			resp.sendExpectationFailed()
			break
		}

		// 进入 http server 处理流程.
		//
		// HTTP cannot have multiple simultaneous active requests.[*]
		// Until the server replies to this request, it can't read another,
		// so we might as well run the handler in this goroutine.
		// [*] Not strictly true: HTTP pipelining. 
		// We could let them all process in parallel even if their responses
		// need to be serialized.
		serverHandler{c.server}.ServeHTTP(resp, resp.req)
		if c.hijacked() {
			return
		}
		resp.finishRequest()
		if resp.closeAfterReply {
			if resp.requestBodyLimitHit {
				c.closeWriteAndWait()
			}
			break
		}
	}
}
