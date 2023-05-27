// Copyright 2013 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Futex is only available on Dragonfly, FreeBSD and Linux.
// The race detector emits calls to split stack functions so it breaks the test.
// +build dragonfly freebsd linux
// +build !race

package runtime_test

import (
	. "runtime"
	"testing"
	"time"
)

// 命令行执行可用: go test -v -run ./futex_test.go

func TestFutexsleep(t *testing.T) {
	ch := make(chan bool, 1)
	var dummy uint32
	start := time.Now()
	go func() {
		Entersyscall()
		Futexsleep(&dummy, 0, (1<<31+100)*1e9)
		Exitsyscall()
		ch <- true
	}()
	select {
	case <-ch:
		// 过早醒来, 说明 futex sleep 失败了.
		t.Errorf("futexsleep finished early after %s!", time.Since(start))
	case <-time.After(time.Second):
		// 成功休眠1秒后, 手动唤醒
		Futexwakeup(&dummy, 1)
	}
}
