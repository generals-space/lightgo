// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build !cmd_go_bootstrap

// This code is compiled into the real 'go' binary, but it is not
// compiled into the binary that is built during all.bash, so as
// to avoid needing to build net (and thus use cgo) during the
// bootstrap process.

// 20231005 将 net/http 移除核心标准库时做的变更, 解除对 net/http 的依赖

package main

import "io"

// httpGET returns the data from an HTTP GET request for the given URL.
func httpGET(url string) ([]byte, error) {
	return []byte{}, nil
}

// httpsOrHTTP returns the body of either the importPath's
// https resource or, if unavailable, the http resource.
func httpsOrHTTP(importPath string) (urlStr string, body io.ReadCloser, err error) {
	return "", nil, err
}
