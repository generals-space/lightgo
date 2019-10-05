// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build dragonfly freebsd linux

#include "runtime.h"
#include "stack.h"
#include "../../cmd/ld/textflag.h"

// This implementation depends on OS-specific implementations of
//
//	runtime·futexsleep(uint32 *addr, uint32 val, int64 ns)
//		Atomically,
//			if(*addr == val) sleep
//		Might be woken up spuriously; that's allowed.
//		Don't sleep longer than ns; ns < 0 means forever.
//
//	runtime·futexwakeup(uint32 *addr, uint32 cnt)
//		If any procs are sleeping on addr, wake up at most cnt.

enum
{
	MUTEX_UNLOCKED = 0,
	MUTEX_LOCKED = 1,
	MUTEX_SLEEPING = 2,

	ACTIVE_SPIN = 4,
	ACTIVE_SPIN_CNT = 30,
	PASSIVE_SPIN = 1,
};

// Possible lock states are MUTEX_UNLOCKED, MUTEX_LOCKED and MUTEX_SLEEPING.
// MUTEX_SLEEPING means that there is presumably at least one sleeping thread.
// Note that there can be spinning threads during all states - they do not
// affect mutex's state.
void
runtime·lock(Lock *l)
{
	uint32 i, v, wait, spin;

	if(m->locks++ < 0)
		runtime·throw("runtime·lock: lock count");

	// Speculative grab for lock.
	v = runtime·xchg((uint32*)&l->key, MUTEX_LOCKED);
	if(v == MUTEX_UNLOCKED)
		return;

	// wait is either MUTEX_LOCKED or MUTEX_SLEEPING
	// depending on whether there is a thread sleeping
	// on this mutex.  If we ever change l->key from
	// MUTEX_SLEEPING to some other value, we must be
	// careful to change it back to MUTEX_SLEEPING before
	// returning, to ensure that the sleeping thread gets
	// its wakeup call.
	wait = v;

	// On uniprocessor's, no point spinning.
	// On multiprocessors, spin for ACTIVE_SPIN attempts.
	spin = 0;
	if(runtime·ncpu > 1)
		spin = ACTIVE_SPIN;

	for(;;) {
		// Try for lock, spinning.
		for(i = 0; i < spin; i++) {
			while(l->key == MUTEX_UNLOCKED)
				if(runtime·cas((uint32*)&l->key, MUTEX_UNLOCKED, wait))
					return;
			runtime·procyield(ACTIVE_SPIN_CNT);
		}

		// Try for lock, rescheduling.
		for(i=0; i < PASSIVE_SPIN; i++) {
			while(l->key == MUTEX_UNLOCKED)
				if(runtime·cas((uint32*)&l->key, MUTEX_UNLOCKED, wait))
					return;
			runtime·osyield();
		}

		// Sleep.
		v = runtime·xchg((uint32*)&l->key, MUTEX_SLEEPING);
		if(v == MUTEX_UNLOCKED)
			return;
		wait = MUTEX_SLEEPING;
		runtime·futexsleep((uint32*)&l->key, MUTEX_SLEEPING, -1);
	}
}

void
runtime·unlock(Lock *l)
{
	uint32 v;

	v = runtime·xchg((uint32*)&l->key, MUTEX_UNLOCKED);
	if(v == MUTEX_UNLOCKED)
		runtime·throw("unlock of unlocked lock");
	if(v == MUTEX_SLEEPING)
		runtime·futexwakeup((uint32*)&l->key, 1);

	if(--m->locks < 0)
		runtime·throw("runtime·unlock: lock count");
	if(m->locks == 0 && g->preempt)  // restore the preemption request in case we've cleared it in newstack
		g->stackguard0 = StackPreempt;
}

// One-time notifications.
// n->key = 0表示可以在此休眠,
// 在sleep函数中会判断n->key的值, 
// 如果不为0, 则表示被唤醒, 可以返回了.
void
runtime·noteclear(Note *n)
{
	n->key = 0;
}

void
runtime·notewakeup(Note *n)
{
	uint32 old;
	// 将 n->key 赋值为1, old为true/false
	old = runtime·xchg((uint32*)&n->key, 1);
	if(old != 0) {
		runtime·printf("notewakeup - double wakeup (%d)\n", old);
		runtime·throw("notewakeup - double wakeup");
	}
	// 唤醒1个在 n->key 处休眠的进程
	runtime·futexwakeup((uint32*)&n->key, 1);
}

// 休眠直到某个协程调用了wakeup将其唤醒.
// 唤醒的标识就是n->key不等于0...???
void
runtime·notesleep(Note *n)
{
	// 只能在g0上调用.
	if(g != m->g0) runtime·throw("notesleep not on g0");
	while(runtime·atomicload((uint32*)&n->key) == 0)
		runtime·futexsleep((uint32*)&n->key, 0, -1);
}

#pragma textflag NOSPLIT
static bool
notetsleep(Note *n, int64 ns, int64 deadline, int64 now)
{
	// Conceptually, deadline and now are local variables.
	// They are passed as arguments so that the space for them
	// does not count against our nosplit stack sequence.

	if(ns < 0) {
		while(runtime·atomicload((uint32*)&n->key) == 0)
			// -1表示不设超时, 只能由wakeup唤醒
			runtime·futexsleep((uint32*)&n->key, 0, -1);
		// 被唤醒, 返回true
		return true;
	}
	
	// n->key不等于0, 说明已被唤醒???
	if(runtime·atomicload((uint32*)&n->key) != 0) return true;
	// 计时开始
	deadline = runtime·nanotime() + ns;
	for(;;) {
		runtime·futexsleep((uint32*)&n->key, 0, ns);

		if(runtime·atomicload((uint32*)&n->key) != 0) break;

		now = runtime·nanotime();
		
		if(now >= deadline) break;

		ns = deadline - now;
	}
	// deadline时间到
	
	return runtime·atomicload((uint32*)&n->key) != 0;
}

// ns 纳秒时间长度
// 在runtime.h底部介绍了 runtime·notetsleep() 与 runtime·notesleep() 相似
// 但这两个明显实现方式不同...
// caller: runtime·stoptheworld()
bool
runtime·notetsleep(Note *n, int64 ns)
{
	bool res;
	// g0是特殊的g对象, 在m中, g0可以拥有调度堆栈
	if(g != m->g0 && !m->gcing) runtime·throw("notetsleep not on g0");

	res = notetsleep(n, ns, 0, 0);
	return res;
}

// same as runtime·notetsleep, but called on user g (not g0)
// calls only nosplit functions between entersyscallblock/exitsyscall
bool
runtime·notetsleepg(Note *n, int64 ns)
{
	bool res;

	if(g == m->g0) runtime·throw("notetsleepg on g0");

	runtime·entersyscallblock();
	res = notetsleep(n, ns, 0, 0);
	runtime·exitsyscall();
	return res;
}
