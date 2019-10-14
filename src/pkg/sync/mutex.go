// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package sync provides basic synchronization primitives such as mutual
// exclusion locks.  Other than the Once and WaitGroup types, most are intended
// for use by low-level library routines.  Higher-level synchronization is
// better done via channels and communication.
//
// Values containing the types defined in this package should not be copied.
package sync

import (
	"sync/atomic"
	"unsafe"
)

// A Mutex is a mutual exclusion lock.
// Mutexes can be created as part of other structures;
// the zero value for a Mutex is an unlocked mutex.
type Mutex struct {
	state int32
	sema  uint32
}

// A Locker represents an object that can be locked and unlocked.
type Locker interface {
	Lock()
	Unlock()
}

const (
	// mutex is locked
	mutexLocked = 1 << iota // 值为 1

	// 当一个已经获得锁的协程在调用 UnLock() 时会设置此标识.
	// 后面抢占到锁的协程会在 `if awoke {}`条件中将此标识重置为0.
	mutexWoken // 值为 2

	// Waiter Shift 用来表示参与抢占当前 mutex 的协程的数量.
	// 如果 state = 0000 0101 说明有1个协程正在参与抢占,
	// 如果 state = 0000 1001 则表示有2个协程(从第3位开始数)
	// 需要注意的是, 当只有一个协程 Lock(), 然后又 UnLock() 期间,
	// 算是没有协程等待, 则waiter 的数量为0,
	// 除非另一个协程调用 Lock() 开始阻塞时, waiter 数量才会加1;
	// 另外, 当持有锁的协程在调用 UnLcok() 时就会判断是否有协程在等待,
	// 如果有, 则直接将 waiter 数量减1,
	// 而不是由之后成功抢占到锁的协程来减...(why???)
	mutexWaiterShift = iota // 值为 2
)

// Lock locks m.
// If the lock is already in use, the calling goroutine
// blocks until the mutex is available.
// 锁定当前 mutex 对象
// 如果某个协程调用此函数时, mutex 已经被其他协程占用了,
// 调用方将一直阻塞直到其他协程将 mutex 释放
func (m *Mutex) Lock() {
	// Fast path: grab unlocked mutex.
	// 原子操作, 将锁的状态修改为 Locked.
	// ...在没有锁的情况下直接使用原子操作代替, 情有可原.
	if atomic.CompareAndSwapInt32(&m.state, 0, mutexLocked) {
		if raceenabled {
			raceAcquire(unsafe.Pointer(m))
		}
		return
	}
	// 运行到这里, 说明 mutex 对象已经被占用, 那么当前协程需要~~阻塞等待~~
	// 不是等待, 是要用for循环无限抢占, 直到成功.
	//
	// 顺利获得 mutex 需要两个条件, 缺一不可:
	// 一个是awoke, 即持有 mutex 的协程调用了 Unlock, 将其释放;
	// 另一个是, 在下面的for抢占循环中, 及时锁定空闲的 mutex;
	awoke := false

	for {
		// 每次for循环都要重新获取 m.state, 因为有可能有其他协程参与进 mutex 的抢占,
		// 从而导致 m.state 被修改.
		// 下面对 new 变量的修改最终是在for循环末尾的 CAS 操作中实施的.
		// 其实就是要将 state 修改为 locked, 然后增加 wait 的协程数量

		// ...现在你又不用原子操作了? 咋不用 Load 呢?
		old := m.state

		// 与 0001 做或操作, 则结果必定是 xxx1
		new := old | mutexLocked

		// 不为0说明本次循环中 lock 状态未解除, 那么将参与竞争的协程数量加1.
		// 第一次调用 Lock 的协程, 由于 old 为 0000, 所以不会有加1的操作.
		// 一旦这里成功, 那接下来的CAS就不会 break(判断条件是相反的).
		// 可以说 acquire 操作与 waiter 加1的操作是同步的.
		if old&mutexLocked != 0 {
			new = old + 1<<mutexWaiterShift
		}

		if awoke {
			// The goroutine has been woken from sleep,
			// so we need to reset the flag in either case.
			// 当前协程已经在上一次for循环通过 Semaacquire 获得了信号量(其它的还在休眠呢)
			// (首次for循环 awoke 肯定为 false, 不会进入这里)
			// 由于 woken 标识位只会在持有锁的协程调用 UnLock() 时设置,
			// 现在当前协程已经被唤醒了, 那么需要把 woken 再修改为0.
			//
			// 但是不需要将 wait 数量减1, 这个操作在上一个持有锁的协程来完成.
			//
			// 下面的操作是其实包含了两步: 先做异或操作, 再用new与其结果做与操作.
			// 会把 woken 标识位抹除, 而其他位上的值则保持不变.
			// 比如, 从上面的流程分析下来, new 的值大概为 xxx1(因为有 locked 标识位),
			// 如果 new = 0001, 结果为 0001, 如果 new = 0011, 结果为 0001
			// 如果 new = 0111, 结果为 0101, 如果 new = 1011, 结果为 1001
			// ...md用&加^操作重置某个位, 这操作简直神了, 666.
			new &^= mutexWoken
		}

		// 注意: 如果这里没成功(就是有其他协程抢先了), 那本次for循环相当于啥也没做, 不会有影响.
		if atomic.CompareAndSwapInt32(&m.state, old, new) {
			// 这是退出for循环的唯一可能, 就是old的 locked 标识位为0
			if old&mutexLocked == 0 {
				break
			}
			// 其实这里才是 mutex 的核心, 其底层借助了信号量 semaphore
			// 检测sema值是否大于0, 如果大于0, 原子减1, goroutine进入ready状态, 继续争用锁.
			// 否则goroutine进入sleep等待唤醒状态(Unlock 有释放sema, 将其加1的语句).
			runtime_Semacquire(&m.sema)
			awoke = true
		}
	}

	if raceenabled {
		raceAcquire(unsafe.Pointer(m))
	}
}

// Unlock unlocks m.
// It is a run-time error if m is not locked on entry to Unlock.
//
// A locked Mutex is not associated with a particular goroutine.
// It is allowed for one goroutine to lock a Mutex and then
// arrange for another goroutine to unlock it.
// 一个已锁定的 mutex 不会与特定的 goroutine 关联(即你无法得知当前 mutex 属于哪一个协程)
//
func (m *Mutex) Unlock() {
	if raceenabled {
		_ = m.state
		raceRelease(unsafe.Pointer(m))
	}

	// Fast path: drop lock bit.
	// 释放 locked 标识, 原子操作, 没毛病.
	// 不过, 为什么像 Lock() 中重置 woken 标识一样, 用&加^操作来完成呢?
	new := atomic.AddInt32(&m.state, -mutexLocked)

	// 这是检测 new 原子操作Add是否成功了
	// 如果 new = xxx0, 结果为 0001
	// 如果 new = xxx1, 结果为 0000
	// 所以如果条件为真, 说明 new 为 xxx1, 而原来的 m.state 为 xxx0,
	// 这说明本来 mutex 就是没有加锁的, 这应该抛出一个异常.
	// 问题是, 为什么验证操作要写的这么麻烦? 不就是验证最后一位是0还是1么?
	if (new+mutexLocked)&mutexLocked == 0 {
		panic("sync: unlock of unlocked mutex")
	}

	old := new
	for {
		// If there are no waiters or a goroutine has already
		// been woken or grabbed the lock, no need to wake anyone.
		// 前者说明没有因为等待信号量而休眠的协程,
		// 后者 old & 0011 != 0, 说明有其他协程已经重新将 mutex 锁定,
		// 或是锁定后又释放然后设置了 woken 标识位.
		// (不过 woken 是在下面的语句中设置的, 这是有多快? 后发先至啊...)
		// 那就不用释放 sema 了, 因为只有一个协程的时候没有 acquire 过 sema...
		// 第一种情况好说, 第二种情况比较难想像啊...
		if old>>mutexWaiterShift == 0 || old&(mutexLocked|mutexWoken) != 0 {
			return
		}

		// Grab the right to wake someone.
		// 运行到这里, 说明有协程在等待 sema, 需要释放(释放后休眠的协程会自然唤醒开始抢占).
		// 将 waiter 数量减1, 并且设置 woken 标识位.
		// ~~...为什么 waiter 要减1?~~ 下面减得是2, 双数.
		//
		// 可以说是在这里提前(在等待的协程实际获得锁之前)把这个值减了...
		// 假设当前仍有1个协程在等待, 那么此时, old 为0100, 下面 new 会变为 0010
		new = (old - 1<<mutexWaiterShift) | mutexWoken
		if atomic.CompareAndSwapInt32(&m.state, old, new) {
			runtime_Semrelease(&m.sema)
			return
		}
		old = m.state
	}
}
