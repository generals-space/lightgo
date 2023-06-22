// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package errors implements functions to manipulate errors.
package errors

// New returns an error that formats as the given text.
func New(text string) error {
	return &errorString{text}
}

// errorString is a trivial implementation of error.
type errorString struct {
	s string
}

// Error 很奇怪, 对任何一个 error.Error() 进行代码跳转, 都跳不过去, 说明 error 是内置类型.
// 我觉得实际的类型声明应该在
// src/pkg/builtin/builtin.go -> error()
// 是一个接口, 应该说, 任何一个实际了 Error() 方法的对象, 都可以被定义为 error 类型?
//
// 而真正的 error 类型, 应该为 src/cmd/gc/go.h -> errortype 类型, 在编译期识别.
func (e *errorString) Error() string {
	return e.s
}
