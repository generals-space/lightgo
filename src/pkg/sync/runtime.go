// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package sync

import "unsafe"

// defined in package runtime
// 下面的几个函数声明, 定义部署请见 src/pkg/runtime/sema.goc 的同名函数.


// Semacquire操作在*s<=0时会阻塞, >0时会将其减一然后返回.(与类unix系统提供的系统调用行为相似)
// semaphore被设计为用于同步的简单的P操作, golang并不支持开发者直接使用这个机制.
//
// Semacquire waits until *s > 0 and then atomically decrements it.
// It is intended as a simple sleep primitive for use by the synchronization
// library and should not be used directly.
func runtime_Semacquire(s *uint32)

// Semrelease自动递增*s, 并通知一个因为调用Semacquire而阻塞的处于等待状态的协程.
// 与C语言中操作系统提供的信号量机制一致, 但是C语言中的互斥锁是用futex实现的...
// 它是一个简单的唤醒原语, 供同步库使用, 不应直接使用.
// 如果handoff为true, 将计数值直接传递给第一个等待协程.
// ...如果为false, 则随机选一个唤醒???
//
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
