// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package build

// const goosList = "darwin dragonfly freebsd linux netbsd openbsd plan9 windows "
// const goarchList = "386 amd64 arm "

// List of past, present, and future known GOOS and GOARCH values.
// Do not remove from this list, as these are used for go/build filename matching.

// 	@compatible: 此列表取自 v1.16 版本同名文件
// 
// 按首字母排序
//
// [golang v1.16 syslist.go](https://github.com/golang/go/tree/go1.16/src/go/build/syslist.go)
const goosList = "aix android darwin dragonfly freebsd hurd illumos ios js linux nacl netbsd openbsd plan9 solaris windows zos "
const goarchList = "386 amd64 amd64p32 arm armbe arm64 arm64be ppc64 ppc64le mips mipsle mips64 mips64le mips64p32 mips64p32le ppc riscv riscv64 s390 s390x sparc sparc64 wasm "
