// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// HTTP server.  See RFC 2616.

package http

import (
	"bufio"
	"crypto/tls"
	"errors"
	"io"
	"log"
	"net"
	"sync"
	"time"
)

// Errors introduced by the HTTP server.
var (
	ErrWriteAfterFlush = errors.New("Conn.Write called after Flush")
	ErrBodyNotAllowed  = errors.New("http: request method or response status code does not allow body")
	ErrHijacked        = errors.New("Conn has been hijacked")
	ErrContentLength   = errors.New("Conn.Write wrote more than the declared Content-Length")
)

// Handler 开发者层面的处理逻辑.
//
// 	@implementBy: HandlerFunc (函数对象)
// 	@implementBy: redirectHandler{}
// 	@implementBy: timeoutHandler{}
// 	@implementBy: globalOptionsHandler{}
// 	@implementBy: fileHandler{} 静态文件服务器实现
// 	~~@implementBy: serverHandler{}~~ 这个不算, ta是 Handler 接口前面的处理流程, 不属于同一级别.
//
// Objects implementing the Handler interface can be
// registered to serve a particular path or subtree in the HTTP server.
//
// ServeHTTP should write reply headers and data to the ResponseWriter
// and then return.  Returning signals that the request is finished
// and that the HTTP server can move on to the next request on the connection.
type Handler interface {
	ServeHTTP(ResponseWriter, *Request)
}

////////////////////////////////////////////////////////////////////////////////
// 拆分 chunkWriter 到独立文件 server_chunkwriter.go
// 拆分 expectContinueReader 到独立的 server_chunkwriter.go 文件
// 拆分 chunkWriter 到独立文件 server_chunkwriter.go
// 拆分 ServeMux 到独立文件 server_mux.go
////////////////////////////////////////////////////////////////////////////////

// HandlerFunc 大部分应用场景都是进行类型转换, 将目标函数转换为标准的 handler.
//
// 	@implementOf: Handler
//
// The HandlerFunc type is an adapter to allow the use of ordinary functions
// as HTTP handlers.
// If f is a function with the appropriate signature, HandlerFunc(f) is a
// Handler object that calls f.
type HandlerFunc func(ResponseWriter, *Request)

// ServeHTTP 所有 HandlerFunc 类型函数对象的 ServeHTTP() 方法, 最终都调用了该函数本身.
//
// ServeHTTP calls f(w, r).
func (f HandlerFunc) ServeHTTP(w ResponseWriter, r *Request) {
	f(w, r)
}

// 	@param pattern: uri 路径
// 	@param handler: 对应的处理函数
//
// Handle registers the handler for the given pattern in the DefaultServeMux.
// The documentation for ServeMux explains how patterns are matched.
func Handle(pattern string, handler Handler) {
	DefaultServeMux.Handle(pattern, handler)
}

// HandleFunc registers the handler function for the given pattern
// in the DefaultServeMux.
// The documentation for ServeMux explains how patterns are matched.
func HandleFunc(pattern string, handler func(ResponseWriter, *Request)) {
	DefaultServeMux.HandleFunc(pattern, handler)
}

// A Server defines parameters for running an HTTP server.
type Server struct {
	Addr    string  // TCP address to listen on, ":http" if empty
	Handler Handler // handler to invoke, http.DefaultServeMux if nil
	// 与客户端初次建立 socket 连接后, 从客户端读取 http request 信息的超时时间.
	// 该超时时间是在底层 socket 对象上设置的.
	// 如果该值为0, 则无限制.
	//
	// maximum duration before timing out read of the request
	ReadTimeout time.Duration
	// 与 WriteTimeout 相同, 不过ta的设置时机是在第一次从客户端读取到 http request 后.
	//
	// maximum duration before timing out write of the response
	WriteTimeout time.Duration
	// maximum size of request headers, DefaultMaxHeaderBytes if 0
	MaxHeaderBytes int
	// optional TLS config, used by ListenAndServeTLS
	TLSConfig *tls.Config

	// TLSNextProto optionally specifies a function to take over ownership of
	//the provided TLS connection when an NPN protocol upgrade has occurred.
	// The map key is the protocol name negotiated.
	// The Handler argument should be used to handle HTTP requests
	// and will initialize the Request's TLS and RemoteAddr if not already set.
	// The connection is automatically closed when the function returns.
	TLSNextProto map[string]func(*Server, *tls.Conn, Handler)

	// 	@compatible: 该字段在 v1.3 版本初次添加
	//
	// ConnState specifies an optional callback function that is
	// called when a client connection changes state.
	// See the ConnState type and associated constants for details.
	ConnState func(net.Conn, ConnState)

	// 	@compatible: 该字段在 v1.3 版本初次添加
	//
	// ErrorLog specifies an optional logger for errors accepting
	// connections and unexpected behavior from handlers.
	// If nil, logging goes to os.Stderr via the log package's standard logger.
	ErrorLog *log.Logger
	////////////////////////////////////////////////////////////////////////////
	// 	@compatible: 该字段在 v1.6 版本初次添加
	//
	nextProtoOnce sync.Once // guards initialization of TLSNextProto in Serve
	nextProtoErr  error

	////////////////////////////////////////////////////////////////////////////
	// 	@compatible: 该字段在 v1.8 版本初次添加
	//
	mu sync.Mutex

	// 	@compatible: 该字段在 v1.8 版本初次添加
	//
	// IdleTimeout is the maximum amount of time to wait for the
	// next request when keep-alives are enabled. If IdleTimeout
	// is zero, the value of ReadTimeout is used. If both are
	// zero, there is no timeout.
	IdleTimeout time.Duration
	// 	@compatible: addAt v1.8
	//
	listeners map[net.Listener]struct{}
	// 	@compatible: addAt v1.8
	// ReadHeaderTimeout is the amount of time allowed to read
	// request headers. The connection's read deadline is reset
	// after reading the headers and the Handler can decide what
	// is considered too slow for the body.
	ReadHeaderTimeout time.Duration
	// 	@compatible: addAt v1.8
	// accessed atomically (non-zero means we're in Shutdown)
	inShutdown int32
	////////////////////////////////////////////////////////////////////////////
	// 	@compatible: addAt v1.9
	//
	onShutdown []func()
	// 	@compatible: addAt v1.9
	//
	doneChan chan struct{}
	// 	@compatible: addAt v1.9
	//
	activeConn map[*conn]struct{}
}

// noLimit is an effective infinite upper bound for io.LimitedReader
const noLimit int64 = (1 << 63) - 1

// debugServerConnections controls whether all server connections are wrapped
// with a verbose logging wrapper.
const debugServerConnections = false

// 	@param rwc: connect socket 对象
// (我把 socket 分为 listen socket 和 connect socket, rwc 就是后者).
//
// caller:
// 	1. Server.Serve() 每 accept() 一个请求, 就开一个协程调用该方法进行处理.
//
// Create new connection from rwc.
func (srv *Server) newConn(rwc net.Conn) (c *conn, err error) {
	c = new(conn)
	c.remoteAddr = rwc.RemoteAddr().String()
	c.server = srv
	c.rwc = rwc
	if debugServerConnections {
		c.rwc = newLoggingConn("server", c.rwc)
	}
	c.sr = liveSwitchReader{r: c.rwc}
	c.lr = io.LimitReader(&c.sr, noLimit).(*io.LimitedReader)
	br := newBufioReader(c.lr)
	bw := newBufioWriterSize(c.rwc, 4<<10)
	c.buf = bufio.NewReadWriter(br, bw)
	return c, nil
}

// DefaultMaxHeaderBytes is the maximum permitted size of the headers
// in an HTTP request.
// This can be overridden by setting Server.MaxHeaderBytes.
const DefaultMaxHeaderBytes = 1 << 20 // 1 MB

func (srv *Server) maxHeaderBytes() int {
	if srv.MaxHeaderBytes > 0 {
		return srv.MaxHeaderBytes
	}
	return DefaultMaxHeaderBytes
}

////////////////////////////////////////////////////////////////////////////////

// Serve accepts incoming HTTP connections on the listener l,
// creating a new service goroutine for each.
// The service goroutines read requests and then call handler to reply to them.
// Handler is typically nil, in which case the DefaultServeMux is used.
func Serve(l net.Listener, handler Handler) error {
	srv := &Server{Handler: handler}
	return srv.Serve(l)
}

// Serve 对目标 listen socket 循环执行 accept(), 每接收到一个请求, 都开一个协程去处理.
// (我把 socket 分为 listen socket 和 connect socket).
//
// 	@param listener: Listen(host, port) 的结果, 其实可以看作是一个 socket 对象.
//
// caller:
// 	1. Serve()
// 	2. Server.ListenAndServe()
// 	3. Server.ListenAndServeTLS()
//
// Serve accepts incoming connections on the Listener l, creating a
// new service goroutine for each.
// The service goroutines read requests and then call srv.Handler to reply to them.
func (srv *Server) Serve(listener net.Listener) error {
	defer listener.Close()
	var tempDelay time.Duration // how long to sleep on accept failure
	for {
		// 每接收到一个请求, 都开一个协程去处理
		rw, e := listener.Accept()
		if e != nil {
			if ne, ok := e.(net.Error); ok && ne.Temporary() {
				if tempDelay == 0 {
					tempDelay = 5 * time.Millisecond
				} else {
					tempDelay *= 2
				}
				if max := 1 * time.Second; tempDelay > max {
					tempDelay = max
				}
				log.Printf("http: Accept error: %v; retrying in %v", e, tempDelay)
				time.Sleep(tempDelay)
				continue
			}
			return e
		}
		tempDelay = 0
		c, err := srv.newConn(rw)
		if err != nil {
			continue
		}
		// 开协程, 处理当前连接的http请求
		go c.serve()
	}
}

////////////////////////////////////////////////////////////////////////////////

// ListenAndServe listens on the TCP network address addr
// and then calls Serve with handler to handle requests on incoming connections.
// Handler is typically nil, in which case the DefaultServeMux is used.
//
func ListenAndServe(addr string, handler Handler) error {
	server := &Server{Addr: addr, Handler: handler}
	return server.ListenAndServe()
}

// ListenAndServe listens on the TCP network address srv.Addr and then
// calls Serve to handle requests on incoming connections.
// If srv.Addr is blank, ":http" is used.
func (srv *Server) ListenAndServe() error {
	addr := srv.Addr
	if addr == "" {
		addr = ":http"
	}
	l, e := net.Listen("tcp", addr)
	if e != nil {
		return e
	}
	return srv.Serve(l)
}

////////////////////////////////////////////////////////////////////////////////

// ListenAndServeTLS acts identically to ListenAndServe, except that it
// expects HTTPS connections. Additionally, files containing a certificate and
// matching private key for the server must be provided. If the certificate
// is signed by a certificate authority, the certFile should be the concatenation
// of the server's certificate followed by the CA's certificate.
//
// One can use generate_cert.go in crypto/tls to generate cert.pem and key.pem.
func ListenAndServeTLS(addr string, certFile string, keyFile string, handler Handler) error {
	server := &Server{Addr: addr, Handler: handler}
	return server.ListenAndServeTLS(certFile, keyFile)
}

// ListenAndServeTLS listens on the TCP network address srv.Addr and
// then calls Serve to handle requests on incoming TLS connections.
//
// Filenames containing a certificate and matching private key for
// the server must be provided. If the certificate is signed by a
// certificate authority, the certFile should be the concatenation
// of the server's certificate followed by the CA's certificate.
//
// If srv.Addr is blank, ":https" is used.
func (srv *Server) ListenAndServeTLS(certFile, keyFile string) error {
	addr := srv.Addr
	if addr == "" {
		addr = ":https"
	}
	config := &tls.Config{}
	if srv.TLSConfig != nil {
		*config = *srv.TLSConfig
	}
	if config.NextProtos == nil {
		config.NextProtos = []string{"http/1.1"}
	}

	var err error
	config.Certificates = make([]tls.Certificate, 1)
	config.Certificates[0], err = tls.LoadX509KeyPair(certFile, keyFile)
	if err != nil {
		return err
	}

	conn, err := net.Listen("tcp", addr)
	if err != nil {
		return err
	}

	tlsListener := tls.NewListener(conn, config)
	return srv.Serve(tlsListener)
}
