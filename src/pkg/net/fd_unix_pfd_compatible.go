package net

// 	@compatible: 该文件取自
// [golang v1.9 fd_unix.go](https://github.com/golang/go/tree/go1.9/src/internal/poll/fd_unix.go)
//
// FD{} 结构体在 v1.9 版本初次添加, 其实是将 src/pkg/net/fd_unix.go -> netFD{} 中的
// 部分字段进行了抽象, 将 net/io 两个包(目前已知的只有这两个)中的 fd 成员统一管理.
//
// 这里并没有按照 v1.9 的方式照搬, 字段也只有必要的几个, 只在 net 包里做了部分改动.
//
type FD struct {
	// Lock sysfd and serialize access to Read and Write methods.
	fdmu  fdMutex
	Sysfd int
	// wait server
	pd pollDesc
	// Whether this is a file rather than a network socket.
	// 可移除, 暂不准备用来表示常规文件
	isFile bool
}

// RawControl invokes the user-defined function f for a non-IO operation.
func (fd *FD) RawControl(f func(uintptr)) error {
	if err := fd.incref(); err != nil {
		return err
	}
	defer fd.decref()
	f(uintptr(fd.Sysfd))
	return nil
}

// RawRead invokes the user-defined function f for a read operation.
func (fd *FD) RawRead(f func(uintptr) bool) error {
	if err := fd.readLock(); err != nil {
		return err
	}
	defer fd.readUnlock()
	// if err := fd.pd.prepareRead(fd.isFile); err != nil {
	if err := fd.pd.PrepareRead(); err != nil {
		return err
	}
	for {
		if f(uintptr(fd.Sysfd)) {
			return nil
		}
		// if err := fd.pd.waitRead(fd.isFile); err != nil {
		if err := fd.pd.WaitRead(); err != nil {
			return err
		}
	}
}

// RawWrite invokes the user-defined function f for a write operation.
func (fd *FD) RawWrite(f func(uintptr) bool) error {
	if err := fd.writeLock(); err != nil {
		return err
	}
	defer fd.writeUnlock()
	// if err := fd.pd.prepareWrite(fd.isFile); err != nil {
	if err := fd.pd.PrepareWrite(); err != nil {
		return err
	}
	for {
		if f(uintptr(fd.Sysfd)) {
			return nil
		}
		// if err := fd.pd.waitWrite(fd.isFile); err != nil {
		if err := fd.pd.WaitWrite(); err != nil {
			return err
		}
	}
}

func (fd *FD) destroy() error {return nil}
