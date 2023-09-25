package http

import (
	"crypto/tls"
	"errors"
	"log"
	"os"
	"strings"
)

// 	@compaitble: 此错误在 v1.5 版本添加
//
// cloneTLSConfig returns a shallow clone of the exported
// fields of cfg, ignoring the unexported sync.Once, which
// contains a mutex and must not be copied.
//
// The cfg must not be in active use by tls.Server, or else
// there can still be a race with tls.Server updating SessionTicketKey
// and our copying it, and also a race with the server setting
// SessionTicketsDisabled=false on failure to set the random
// ticket key.
//
// If cfg is nil, a new zero tls.Config is returned.
func cloneTLSConfig(cfg *tls.Config) *tls.Config {
	if cfg == nil {
		return &tls.Config{}
	}
	return &tls.Config{
		Rand:                     cfg.Rand,
		Time:                     cfg.Time,
		Certificates:             cfg.Certificates,
		NameToCertificate:        cfg.NameToCertificate,
		GetCertificate:           cfg.GetCertificate,
		RootCAs:                  cfg.RootCAs,
		NextProtos:               cfg.NextProtos,
		ServerName:               cfg.ServerName,
		ClientAuth:               cfg.ClientAuth,
		ClientCAs:                cfg.ClientCAs,
		InsecureSkipVerify:       cfg.InsecureSkipVerify,
		CipherSuites:             cfg.CipherSuites,
		PreferServerCipherSuites: cfg.PreferServerCipherSuites,
		SessionTicketsDisabled:   cfg.SessionTicketsDisabled,
		SessionTicketKey:         cfg.SessionTicketKey,
		ClientSessionCache:       cfg.ClientSessionCache,
		MinVersion:               cfg.MinVersion,
		MaxVersion:               cfg.MaxVersion,
		CurvePreferences:         cfg.CurvePreferences,
	}
}

////////////////////////////////////////////////////////////////////////////////

// 	@compaitble: 此错误在 v1.6 版本添加
//
// ErrSkipAltProtocol is a sentinel error value defined by Transport.RegisterProtocol.
var ErrSkipAltProtocol = errors.New("net/http: skip alternate protocol")

// 	@compaitble: 此错误在 v1.6 版本添加
//
func strSliceContains(ss []string, s string) bool {
	for _, v := range ss {
		if v == s {
			return true
		}
	}
	return false
}

// 	@compaitble: 此错误在 v1.6 版本添加
//
// onceSetNextProtoDefaults initializes TLSNextProto.
// It must be called via t.nextProtoOnce.Do.
func (t *Transport) onceSetNextProtoDefaults() {
	if strings.Contains(os.Getenv("GODEBUG"), "http2client=0") {
		return
	}
	if t.TLSNextProto != nil {
		// This is the documented way to disable http2 on a
		// Transport.
		return
	}
	if t.TLSClientConfig != nil {
		// Be conservative for now (for Go 1.6) at least and
		// don't automatically enable http2 if they've
		// specified a custom TLS config. Let them opt-in
		// themselves via http2.ConfigureTransport so we don't
		// surprise them by modifying their tls.Config.
		// Issue 14275.
		return
	}
	if t.ExpectContinueTimeout != 0 {
		// Unsupported in http2, so disable http2 for now.
		// Issue 13851.
		return
	}
	t2, err := http2configureTransport(t)
	if err != nil {
		log.Printf("Error enabling Transport HTTP/2 support: %v", err)
	} else {
		t.h2transport = t2
	}
}

////////////////////////////////////////////////////////////////////////////////

// 	@compaitble: 此方法在 v1.13 版本添加
//
// Clone returns a deep copy of t's exported fields.
func (t *Transport) Clone() *Transport {
	t.nextProtoOnce.Do(t.onceSetNextProtoDefaults)
	t2 := &Transport{
		Proxy:                  t.Proxy,
		DialContext:            t.DialContext,
		Dial:                   t.Dial,
		DialTLS:                t.DialTLS,
		TLSClientConfig:        t.TLSClientConfig.Clone(),
		TLSHandshakeTimeout:    t.TLSHandshakeTimeout,
		DisableKeepAlives:      t.DisableKeepAlives,
		DisableCompression:     t.DisableCompression,
		MaxIdleConns:           t.MaxIdleConns,
		MaxIdleConnsPerHost:    t.MaxIdleConnsPerHost,
		MaxConnsPerHost:        t.MaxConnsPerHost,
		IdleConnTimeout:        t.IdleConnTimeout,
		ResponseHeaderTimeout:  t.ResponseHeaderTimeout,
		ExpectContinueTimeout:  t.ExpectContinueTimeout,
		ProxyConnectHeader:     t.ProxyConnectHeader.Clone(),
		MaxResponseHeaderBytes: t.MaxResponseHeaderBytes,
		ForceAttemptHTTP2:      t.ForceAttemptHTTP2,
		WriteBufferSize:        t.WriteBufferSize,
		ReadBufferSize:         t.ReadBufferSize,
	}
	if !t.tlsNextProtoWasNil {
		npm := map[string]func(authority string, c *tls.Conn) RoundTripper{}
		for k, v := range t.TLSNextProto {
			npm[k] = v
		}
		t2.TLSNextProto = npm
	}
	return t2
}
