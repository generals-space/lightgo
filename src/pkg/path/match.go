// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// 	@note: path 与 path/filepath 的 Match() 函数实现相同, 其实可以直接将
// path.Match() 移除, 但为了保证兼容性, 这里改为调用 filepath 的实现.

package path

import "path/filepath"

var ErrBadPattern = filepath.ErrBadPattern

func Match(pattern, name string) (matched bool, err error) {
	return filepath.Match(pattern, name)
}
