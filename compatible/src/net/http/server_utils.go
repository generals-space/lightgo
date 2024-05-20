package http

import (
	"bufio"
	"io"
	"strings"
	"sync"
)

// A switchReader can have its Reader changed at runtime.
// It's not safe for concurrent Reads and switches.
type switchReader struct {
	io.Reader
}

// A switchWriter can have its Writer changed at runtime.
// It's not safe for concurrent Writes and switches.
type switchWriter struct {
	io.Writer
}

// A liveSwitchReader is a switchReader that's safe for concurrent
// reads and switches, if its mutex is held.
type liveSwitchReader struct {
	sync.Mutex
	r io.Reader
}

func (sr *liveSwitchReader) Read(p []byte) (n int, err error) {
	sr.Lock()
	r := sr.r
	sr.Unlock()
	return r.Read(p)
}

// StripPrefix returns a handler that serves HTTP requests
// by removing the given prefix from the request URL's Path
// and invoking the handler h. StripPrefix handles a
// request for a path that doesn't begin with prefix by
// replying with an HTTP 404 not found error.
func StripPrefix(prefix string, h Handler) Handler {
	if prefix == "" {
		return h
	}
	return HandlerFunc(func(w ResponseWriter, r *Request) {
		if p := strings.TrimPrefix(r.URL.Path, prefix); len(p) < len(r.URL.Path) {
			r.URL.Path = p
			h.ServeHTTP(w, r)
		} else {
			NotFound(w, r)
		}
	})
}

////////////////////////////////////////////////////////////////////////////////

// TODO: use a sync.Cache instead
var (
	bufioReaderCache   = make(chan *bufio.Reader, 4)
	bufioWriterCache2k = make(chan *bufio.Writer, 4)
	bufioWriterCache4k = make(chan *bufio.Writer, 4)
)

// caller:
// 	1. newBufioWriterSize()
// 	2. putBufioWriter()
func bufioWriterCache(size int) chan *bufio.Writer {
	switch size {
	case 2 << 10:
		return bufioWriterCache2k
	case 4 << 10:
		return bufioWriterCache4k
	}
	return nil
}

// caller:
// 	1. Server.newConn()
func newBufioReader(r io.Reader) *bufio.Reader {
	select {
	case p := <-bufioReaderCache:
		p.Reset(r)
		return p
	default:
		return bufio.NewReader(r)
	}
}

func putBufioReader(br *bufio.Reader) {
	br.Reset(nil)
	select {
	case bufioReaderCache <- br:
	default:
	}
}

// caller:
// 	1. Server.newConn()
// 	2. conn.readRequest()
func newBufioWriterSize(w io.Writer, size int) *bufio.Writer {
	select {
	case p := <-bufioWriterCache(size):
		p.Reset(w)
		return p
	default:
		return bufio.NewWriterSize(w, size)
	}
}

// caller:
// 	1. conn.finalFlush()
// 	2. response.finishRequest()
func putBufioWriter(bw *bufio.Writer) {
	bw.Reset(nil)
	select {
	case bufioWriterCache(bw.Available()) <- bw:
	default:
	}
}
