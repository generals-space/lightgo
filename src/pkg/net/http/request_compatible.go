package http

import (
	"context"
)



////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 该方法在 v1.7 版本初次添加
//
// Context returns the request's context. To change the context, use
// WithContext.
//
// The returned context is always non-nil; it defaults to the
// background context.
//
// For outgoing client requests, the context controls cancelation.
//
// For incoming server requests, the context is canceled when the
// ServeHTTP method returns. For its associated values, see
// ServerContextKey and LocalAddrContextKey.
func (r *Request) Context() context.Context {
	if r.ctx != nil {
		return r.ctx
	}
	return context.Background()
}

// 	@compatible: 该方法在 v1.7 版本初次添加
//
// WithContext returns a shallow copy of r with its context changed
// to ctx. The provided ctx must be non-nil.
func (r *Request) WithContext(ctx context.Context) *Request {
	if ctx == nil {
		panic("nil context")
	}
	r2 := new(Request)
	*r2 = *r
	r2.ctx = ctx
	return r2
}
