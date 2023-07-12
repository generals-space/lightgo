package net

import (
	"syscall"
)

// 	@compatible: 此方法在 v1.9 版本初次添加
//
// SyscallConn returns a raw network connection.
// This implements the syscall.Conn interface.
func (c *IPConn) SyscallConn() (syscall.RawConn, error) {
	if !c.ok() {
		return nil, syscall.EINVAL
	}
	return newRawConn(c.fd)
}
