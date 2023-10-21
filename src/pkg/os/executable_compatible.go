// Copyright 2016 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package os

// 	@compatible: 该函数在 v1.8 版本初次添加
//
// Executable returns the path name for the executable that started
// the current process. There is no guarantee that the path is still
// pointing to the correct executable. If a symlink was used to start
// the process, depending on the operating system, the result might
// be the symlink or the path it pointed to.
// If a stable result is needed, path/filepath.EvalSymlinks might help.
//
// Executable returns an absolute path unless an error occurred.
//
// The main use case is finding resources located relative to an executable.
//
// Executable is not supported on nacl or OpenBSD (unless procfs is mounted.)
func Executable() (string, error) {
	return executable()
}

// 	@note: 在原版代码中存在多平台兼容代码, 这里全部删掉了, 只支持 amd64 平台 linux 系统.
//
// We query the executable path at init time to avoid the problem of
// readlink returns a path appended with " (deleted)" when the original
// binary gets deleted.
var executablePath, executablePathErr = func() (string, error) {
	// /proc/self/exe 代表当前程序, 用 readlink() 读取它, 就可以获取当前程序的绝对路径
	var procfn string = "/proc/self/exe"
	return Readlink(procfn)
}()

// 	@compatible: 该函数在 v1.8 版本初次添加
func executable() (string, error) {
	return executablePath, executablePathErr
}
