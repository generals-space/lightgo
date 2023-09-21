// Copyright 2010 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package ioutil

import (
	"os"
	"path/filepath"
	"strconv"
	"sync"
	"time"
)

// Random number state.
// We generate random temporary file names so that there's a good
// chance the file doesn't exist yet - keeps the number of tries in
// TempFile to a minimum.
var rand uint32
var randmu sync.Mutex

func reseed() uint32 {
	return uint32(time.Now().UnixNano() + int64(os.Getpid()))
}

func nextSuffix() string {
	randmu.Lock()
	r := rand
	if r == 0 {
		r = reseed()
	}
	r = r*1664525 + 1013904223 // constants from Numerical Recipes
	rand = r
	randmu.Unlock()
	return strconv.Itoa(int(1e9 + r%1e9))[1:]
}

// TempFile 在目标目录 dir 下创建一个以 prefix 为前缀的临时文件, 将其以读写模式打开并返回.
//
// 	@param dir: 如果为空, 则在"/tmp"目录下创建;
// 	@param prefix: 便于识别的文件名前缀;
//
// 注意: 
// 	1. 调用者可以通过 f.Name() 获取文件的完整路径;
// 	2. 调用本函数所创建的临时文件, 应该由调用者自己删除;
// 
// TempFile creates a new temporary file in the directory dir
// with a name beginning with prefix, opens the file for reading
// and writing, and returns the resulting *os.File.
// If dir is the empty string, TempFile uses the default directory
// for temporary files (see os.TempDir).
// Multiple programs calling TempFile simultaneously
// will not choose the same file.  The caller can use f.Name()
// to find the pathname of the file.  It is the caller's responsibility
// to remove the file when no longer needed.
func TempFile(dir, prefix string) (f *os.File, err error) {
	if dir == "" {
		dir = os.TempDir()
	}

	nconflict := 0
	for i := 0; i < 10000; i++ {
		name := filepath.Join(dir, prefix+nextSuffix())
		f, err = os.OpenFile(name, os.O_RDWR|os.O_CREATE|os.O_EXCL, 0600)
		if os.IsExist(err) {
			if nconflict++; nconflict > 10 {
				rand = reseed()
			}
			continue
		}
		break
	}
	return
}

// TempDir 在目标目录下创建一个临时目录并返回该目录路径.
//
// 	@param dir: 如果为空, 则在"/tmp"目录下创建;
// 	@param prefix: 便于识别的路径前缀;
//
// 操作系统中一般都会提供临时目录, 比如linux下的/tmp目录（通过os.TempDir()可以获取到).
// 有时候, 我们自己需要创建临时目录, 比如Go工具链源码中（src/cmd/go/build.go）,
// 通过 TempDir 创建一个临时目录, 用于存放编译过程的临时文件.
//
// TempDir creates a new temporary directory in the directory dir
// with a name beginning with prefix and returns the path of the
// new directory.  If dir is the empty string, TempDir uses the
// default directory for temporary files (see os.TempDir).
// Multiple programs calling TempDir simultaneously
// will not choose the same directory.  It is the caller's responsibility
// to remove the directory when no longer needed.
func TempDir(dir, prefix string) (name string, err error) {
	if dir == "" {
		dir = os.TempDir()
	}

	nconflict := 0
	for i := 0; i < 10000; i++ {
		try := filepath.Join(dir, prefix+nextSuffix())
		err = os.Mkdir(try, 0700)
		if os.IsExist(err) {
			if nconflict++; nconflict > 10 {
				rand = reseed()
			}
			continue
		}
		if err == nil {
			name = try
		}
		break
	}
	return
}
