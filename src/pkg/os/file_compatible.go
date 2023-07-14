package os

import "syscall"

// 	@compatible: 此方法在在 v1.12 版本添加
//
// SyscallConn returns a raw file.
// This implements the syscall.Conn interface.
func (f *File) SyscallConn() (syscall.RawConn, error) {
	if err := f.checkValid("SyscallConn"); err != nil {
		return nil, err
	}
	return newRawConn(f)
}

// 	@compatible: 此方法在在 v1.12 版本添加
//
// checkValid checks whether f is valid for use.
// If not, it returns an appropriate error, perhaps incorporating the operation name op.
func (f *File) checkValid(op string) error {
	if f == nil {
		return ErrInvalid
	}
	return nil
}
