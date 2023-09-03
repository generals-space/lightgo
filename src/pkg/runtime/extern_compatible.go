// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package runtime


// Mark KeepAlive as noinline so that the current compiler will ensure
// that the argument is alive at the point of the function call.
// If it were inlined, it would disappear, and there would be nothing
// keeping the argument alive. Perhaps a future compiler will recognize
// runtime.KeepAlive specially and do something more efficient.
//go:noinline

// 	@compatible: 该函数在 v1.7 版本初次添加, 具体实现在 c 语言中.
// 	@todo
//
// [golang v1.7 context](https://github.com/golang/go/tree/go1.7/src/runtime/mfinal.go:L464)
//
// KeepAlive marks its argument as currently reachable.
// This ensures that the object is not freed, and its finalizer is not run,
// before the point in the program where KeepAlive is called.
//
// A very simplified example showing where KeepAlive is required:
// 	type File struct { d int }
// 	d, err := syscall.Open("/file/path", syscall.O_RDONLY, 0)
// 	// ... do something if err != nil ...
// 	p := &File{d}
// 	runtime.SetFinalizer(p, func(p *File) { syscall.Close(p.d) })
// 	var buf [10]byte
// 	n, err := syscall.Read(p.d, buf[:])
// 	// Ensure p is not finalized until Read returns.
// 	runtime.KeepAlive(p)
// 	// No more uses of p after this point.
//
// Without the KeepAlive call, the finalizer could run at the start of
// syscall.Read, closing the file descriptor before syscall.Read makes
// the actual system call.
func KeepAlive(interface{}) {}

// 	@compatible: 此方法是为了实现 Fastrand() 函数新增的
//
func fastrand1() uint32
// 	@compatible: 此方法是为了实现 runtime.fastrand() 的妥协
// 	见 src/pkg/os/tempfile_compatible.go -> fastrand()
//
// 原本 fastrand() 是 runtime 中的一个函数, 通过 linkname 直接调用,
// 后来自行暴露了一个 runtime.Fastrand() 函数.
//
// [golang v1.8 fastrand](https://github.com/golang/go/tree/go1.8/src/runtime/asm_amd64.s#L2166)
// [golang v1.9 fastrand](https://github.com/golang/go/tree/go1.9/src/runtime/stubs.go#L97)
//
// 	@implementAt: src/pkg/runtime/runtime.c -> runtime·fastrand1()
func Fastrand() uint32 {
	return fastrand1()
}
