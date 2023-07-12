// 	@compatible: 本文件在 v1.12 版本初次添加

// Copyright 2018 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build !plan9

package os

import (
	"runtime"
)

// rawConn implements syscall.RawConn.
type rawConn struct {
	file *File
}

func (c *rawConn) Control(f func(uintptr)) error {
	if err := c.file.checkValid("SyscallConn.Control"); err != nil {
		return err
	}
	// err := c.file.pfd.RawControl(f)
	err := c.RawControl(f)
	runtime.KeepAlive(c.file)
	return err
}

func (c *rawConn) Read(f func(uintptr) bool) error {
	if err := c.file.checkValid("SyscallConn.Read"); err != nil {
		return err
	}
	// err := c.file.pfd.RawRead(f)
	err := c.RawRead(f)
	runtime.KeepAlive(c.file)
	return err
}

func (c *rawConn) Write(f func(uintptr) bool) error {
	if err := c.file.checkValid("SyscallConn.Write"); err != nil {
		return err
	}
	// err := c.file.pfd.RawWrite(f)
	err := c.RawWrite(f)
	runtime.KeepAlive(c.file)
	return err
}

func newRawConn(file *File) (*rawConn, error) {
	return &rawConn{file: file}, nil
}

////////////////////////////////////////////////////////////////////////////////

// RawControl invokes the user-defined function f for a non-IO operation.
func (c *rawConn) RawControl(f func(uintptr)) error {
	f(c.file.Fd())
	return nil
}

// RawRead invokes the user-defined function f for a read operation.
func (c *rawConn) RawRead(f func(uintptr) bool) error {
	f(c.file.Fd())
	return nil
}

// RawWrite invokes the user-defined function f for a write operation.
func (c *rawConn) RawWrite(f func(uintptr) bool) error {
	f(c.file.Fd())
	return nil
}
