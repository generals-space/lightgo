// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package sync

import "unsafe"

// defined in package runtime
// 下面的几个函数声明, 定义部署请见 src/pkg/runtime/sema.goc 的同名函数.

// Semacquire waits until *s > 0 and then atomically decrements it.
// It is intended as a simple sleep primitive for use by the synchronization
// library and should not be used directly.
func runtime_Semacquire(s *uint32)

// Semrelease atomically increments *s and notifies a waiting goroutine
// if one is blocked in Semacquire.
// It is intended as a simple wakeup primitive for use by the synchronization
// library and should not be used directly.
func runtime_Semrelease(s *uint32)

// Opaque representation of SyncSema in runtime/sema.goc.
type syncSema [3]uintptr

// Syncsemacquire waits for a pairing Syncsemrelease on the same semaphore s.
func runtime_Syncsemacquire(s *syncSema)

// Syncsemrelease waits for n pairing Syncsemacquire on the same semaphore s.
func runtime_Syncsemrelease(s *syncSema, n uint32)

// Ensure that sync and runtime agree on size of syncSema.
func runtime_Syncsemcheck(size uintptr)
func init() {
	var s syncSema
	runtime_Syncsemcheck(unsafe.Sizeof(s))
}
