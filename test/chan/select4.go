// run

// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file

// Test that a select statement proceeds when a value is ready.

// 这个用例太简单了, 没有阅读的必要.

package main

func f() *int {
	println("BUG: called f")
	return new(int)
}

func main() {
	var x struct {
		a int
	}
	c := make(chan int, 1)
	c1 := make(chan int)
	c <- 42
	select {
	// 将 c1 中的成员赋值给 f() 返回的 int 指针.
	// 不过 c1 是无缓冲 channel, 里面没有成员, 所以根本进不了这个 case.
	case *f() = <-c1:
		// nothing
	case x.a = <-c:
		if x.a != 42 {
			println("BUG:", x.a)
		}
	}
}
