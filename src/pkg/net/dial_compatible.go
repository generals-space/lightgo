package net

import "context"

// 	@compatible: 此方法在 v1.7 版本添加
// 	@note: 对原方法做了改动, 直接调用已有的 d.Dial() 方法
//
// DialContext connects to the address on the named network using
// the provided context.
//
// The provided Context must be non-nil. If the context expires before
// the connection is complete, an error is returned. Once successfully
// connected, any expiration of the context will not affect the
// connection.
//
// See func Dial for a description of the network and address
// parameters.
func (d *Dialer) DialContext(ctx context.Context, network, address string) (Conn, error) {
	if ctx == nil {
		panic("nil context")
	}

	return d.Dial(network, address)
}
