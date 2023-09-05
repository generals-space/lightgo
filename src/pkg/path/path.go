// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// 	@note: path 与 path/filepath 的函数实现基本相同, 其实可以直接将这些函数移除,
// 但为了保证兼容性, 这里改为调用 filepath 的实现.

package path

import "path/filepath"

func Clean(path string) string {
	return filepath.Clean(path)
}

func Split(path string) (dir, file string) {
	return filepath.Split(path)
}

func Join(elem ...string) string {
	return filepath.Join(elem...)
}

func Ext(path string) string {
	return filepath.Ext(path)
}

func Base(path string) string {
	return filepath.Base(path)
}

func IsAbs(path string) bool {
	return filepath.IsAbs(path)
}

func Dir(path string) string {
	return filepath.Dir(path)
}
