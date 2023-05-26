/*
 * proc_osthread.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "../../cmd/ld/textflag.h"

// 调用此函数时需要禁止抢占, 否则这里的m对象与调用者的m就不相同了.
// 所以禁用抢占是为了防止m切换???
// 这个函数看起来就是将m和g绑定啊...
//
// caller: 
// 	1. runtime·LockOSThread() 就在下面
// 	2. runtime·lockOSThread() 就在下面
//
// lockOSThread is called by runtime.LockOSThread 
// and runtime.lockOSThread below after they modify m->locked. 
// Do not allow preemption during this call,
// or else the m might be different in this function than in the caller.
#pragma textflag NOSPLIT
static void lockOSThread(void)
{
	m->lockedg = g;
	g->lockedm = m;
}

void runtime·LockOSThread(void)
{
	m->locked |= LockExternal;
	lockOSThread();
}

void runtime·lockOSThread(void)
{
	m->locked += LockInternal;
	lockOSThread();
}

// caller: 
// 	1. runtime·UnlockOSThread()
// 	2. runtime·unlockOSThread()
//
// 与 lockOSThread() 相同, 调用此函数也需要禁止抢占.
//
// unlockOSThread is called by runtime.UnlockOSThread
// and runtime.unlockOSThread below after they update m->locked. 
// Do not allow preemption during this call,
// or else the m might be in different in this function than in the caller.
#pragma textflag NOSPLIT
static void unlockOSThread(void)
{
	if(m->locked != 0) {
		return;
	}
	m->lockedg = nil;
	g->lockedm = nil;
}

void runtime·UnlockOSThread(void)
{
	m->locked &= ~LockExternal;
	unlockOSThread();
}

void runtime·unlockOSThread(void)
{
	if(m->locked < LockInternal) {
		runtime·throw("runtime: internal error: misuse of lockOSThread/unlockOSThread");
	}
	m->locked -= LockInternal;
	unlockOSThread();
}

bool runtime·lockedOSThread(void)
{
	return g->lockedm != nil && m->lockedg != nil;
}

// for testing of callbacks
void runtime·golockedOSThread(bool ret)
{
	ret = runtime·lockedOSThread();
	FLUSH(&ret);
}

void runtime·NumGoroutine(intgo ret)
{
	ret = runtime·gcount();
	FLUSH(&ret);
}

void runtime·badmcall(void (*fn)(G*))  // called from assembly
{
	USED(fn); // TODO: print fn?
	runtime·throw("runtime: mcall called on m->g0 stack");
}

void runtime·badmcall2(void (*fn)(G*))  // called from assembly
{
	USED(fn);
	runtime·throw("runtime: mcall function returned");
}

void runtime·badreflectcall(void) // called from assembly
{
	runtime·panicstring("runtime: arg size to reflect.call more than 1GB");
}

void runtime·Breakpoint(void)
{
	runtime·breakpoint();
}
