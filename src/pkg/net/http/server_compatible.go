package http

import (
	"crypto/tls"
	"errors"
	"net"
	"os"
	"strings"
)

// 	@compatible: 该方法在 v1.6 版本初次添加
//
// onceSetNextProtoDefaults configures HTTP/2, if the user hasn't
// configured otherwise. (by setting srv.TLSNextProto non-nil)
// It must only be called via srv.nextProtoOnce (use srv.setupHTTP2).
func (srv *Server) onceSetNextProtoDefaults() {
	if strings.Contains(os.Getenv("GODEBUG"), "http2server=0") {
		return
	}
	// Enable HTTP/2 by default if the user hasn't otherwise
	// configured their TLSNextProto map.
	if srv.TLSNextProto == nil {
		srv.nextProtoErr = http2ConfigureServer(srv, nil)
	}
}

////////////////////////////////////////////////////////////////////////////////

var (
	// 	@compatible: 该变量在 v1.7 版本初次添加
	//
	// ServerContextKey is a context key. It can be used in HTTP
	// handlers with context.WithValue to access the server that
	// started the handler. The associated value will be of
	// type *Server.
	ServerContextKey = &contextKey{"http-server"}

	// 	@compatible: 该变量在 v1.7 版本初次添加
	//
	// LocalAddrContextKey is a context key. It can be used in
	// HTTP handlers with context.WithValue to access the address
	// the local address the connection arrived on.
	// The associated value will be of type net.Addr.
	LocalAddrContextKey = &contextKey{"local-addr"}
)

////////////////////////////////////////////////////////////////////////////////
// 	@compatible: 该错误在 v1.8 版本初次添加
//
var ErrServerClosed = errors.New("http: Server closed")

// 	@compatible: 该错误在 v1.8 版本初次添加
//
// ErrAbortHandler is a sentinel panic value to abort a handler.
// While any panic from ServeHTTP aborts the response to the client,
// panicking with ErrAbortHandler also suppresses logging of a stack
// trace to the server's error log.
var ErrAbortHandler = errors.New("net/http: abort Handler")

// 	@compatible: 该方法在 v1.8 版本初次添加
//
// Close immediately closes all active net.Listeners and any
// connections in state StateNew, StateActive, or StateIdle. For a
// graceful shutdown, use Shutdown.
//
// Close does not attempt to close (and does not even know about)
// any hijacked connections, such as WebSockets.
//
// Close returns any error returned from closing the Server's
// underlying Listener(s).
func (srv *Server) Close() error {
	srv.mu.Lock()
	defer srv.mu.Unlock()
	srv.closeDoneChanLocked()
	err := srv.closeListenersLocked()
	for c := range srv.activeConn {
		c.rwc.Close()
		delete(srv.activeConn, c)
	}
	return err
}

// 	@compatible: 该方法在 v1.8 版本初次添加
//
func (s *Server) closeDoneChanLocked() {
	ch := s.getDoneChanLocked()
	select {
	case <-ch:
		// Already closed. Don't close again.
	default:
		// Safe to close here. We're the only closer, guarded
		// by s.mu.
		close(ch)
	}
}

// 	@compatible: 该方法在 v1.8 版本初次添加
//
func (s *Server) getDoneChanLocked() chan struct{} {
	if s.doneChan == nil {
		s.doneChan = make(chan struct{})
	}
	return s.doneChan
}

// 	@compatible: 该方法在 v1.8 版本初次添加
//
func (s *Server) closeListenersLocked() error {
	var err error
	for ln := range s.listeners {
		if cerr := ln.Close(); cerr != nil && err == nil {
			err = cerr
		}
		delete(s.listeners, ln)
	}
	return err
}

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 该方法在 v1.9 版本初次添加
//
// RegisterOnShutdown registers a function to call on Shutdown.
// This can be used to gracefully shutdown connections that have
// undergone NPN/ALPN protocol upgrade or that have been hijacked.
// This function should start protocol-specific graceful shutdown,
// but should not wait for shutdown to complete.
func (srv *Server) RegisterOnShutdown(f func()) {
	srv.mu.Lock()
	srv.onShutdown = append(srv.onShutdown, f)
	srv.mu.Unlock()
}

// 	@compatible: 该方法在 v1.9 版本初次添加
//
// ServeTLS accepts incoming connections on the Listener l, creating a
// new service goroutine for each. The service goroutines read requests and
// then call srv.Handler to reply to them.
//
// Additionally, files containing a certificate and matching private key for
// the server must be provided if neither the Server's TLSConfig.Certificates
// nor TLSConfig.GetCertificate are populated.. If the certificate is signed by
// a certificate authority, the certFile should be the concatenation of the
// server's certificate, any intermediates, and the CA's certificate.
//
// For HTTP/2 support, srv.TLSConfig should be initialized to the
// provided listener's TLS Config before calling Serve. If
// srv.TLSConfig is non-nil and doesn't include the string "h2" in
// Config.NextProtos, HTTP/2 support is not enabled.
//
// ServeTLS always returns a non-nil error. After Shutdown or Close, the
// returned error is ErrServerClosed.
func (srv *Server) ServeTLS(l net.Listener, certFile, keyFile string) error {
	// Setup HTTP/2 before srv.Serve, to initialize srv.TLSConfig
	// before we clone it and create the TLS Listener.
	if err := srv.setupHTTP2_ServeTLS(); err != nil {
		return err
	}

	config := cloneTLSConfig(srv.TLSConfig)
	if !strSliceContains(config.NextProtos, "http/1.1") {
		config.NextProtos = append(config.NextProtos, "http/1.1")
	}

	configHasCert := len(config.Certificates) > 0 || config.GetCertificate != nil
	if !configHasCert || certFile != "" || keyFile != "" {
		var err error
		config.Certificates = make([]tls.Certificate, 1)
		config.Certificates[0], err = tls.LoadX509KeyPair(certFile, keyFile)
		if err != nil {
			return err
		}
	}

	tlsListener := tls.NewListener(l, config)
	return srv.Serve(tlsListener)
}

// 	@compatible: 该方法在 v1.9 版本初次添加
//
// setupHTTP2_ServeTLS conditionally configures HTTP/2 on
// srv and returns whether there was an error setting it up. If it is
// not configured for policy reasons, nil is returned.
func (srv *Server) setupHTTP2_ServeTLS() error {
	srv.nextProtoOnce.Do(srv.onceSetNextProtoDefaults)
	return srv.nextProtoErr
}
