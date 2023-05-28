/*
 * proc_osthread.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "../../cmd/ld/textflag.h"

// 调用此函数时需要禁止抢占, 否则这里的 m 对象与调用者的 m 就不相同了.
//
// 嗯...没有看到禁止抢占的代码.
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

////////////////////////////////////////////////////////////////////////////////

// caller: none
void runtime·testSchedLocalQueue(void)
{
	P p;
	G gs[1000];
	int32 i, j;

	runtime·memclr((byte*)&p, sizeof(p));
	p.runqsize = 1;
	p.runqhead = 0;
	p.runqtail = 0;
	p.runq = runtime·malloc(p.runqsize*sizeof(*p.runq));

	for(i = 0; i < nelem(gs); i++) {
		if(runqget(&p) != nil) {
			runtime·throw("runq is not empty initially");
		}
		for(j = 0; j < i; j++) {
			runqput(&p, &gs[i]);
		}
		for(j = 0; j < i; j++) {
			if(runqget(&p) != &gs[i]) {
				runtime·printf("bad element at iter %d/%d\n", i, j);
				runtime·throw("bad element");
			}
		}
		if(runqget(&p) != nil) {
			runtime·throw("runq is not empty afterwards");
		}
	}
}

// caller: none
void runtime·testSchedLocalQueueSteal(void)
{
	P p1, p2;
	G gs[1000], *gp;
	int32 i, j, s;

	runtime·memclr((byte*)&p1, sizeof(p1));
	p1.runqsize = 1;
	p1.runqhead = 0;
	p1.runqtail = 0;
	p1.runq = runtime·malloc(p1.runqsize*sizeof(*p1.runq));

	runtime·memclr((byte*)&p2, sizeof(p2));
	p2.runqsize = nelem(gs);
	p2.runqhead = 0;
	p2.runqtail = 0;
	p2.runq = runtime·malloc(p2.runqsize*sizeof(*p2.runq));

	for(i = 0; i < nelem(gs); i++) {
		for(j = 0; j < i; j++) {
			gs[j].sig = 0;
			runqput(&p1, &gs[j]);
		}
		gp = runqsteal(&p2, &p1);
		s = 0;
		if(gp) {
			s++;
			gp->sig++;
		}
		while(gp = runqget(&p2)) {
			s++;
			gp->sig++;
		}
		while(gp = runqget(&p1)) {
			gp->sig++;
		}
		for(j = 0; j < i; j++) {
			if(gs[j].sig != 1) {
				runtime·printf("bad element %d(%d) at iter %d\n", j, gs[j].sig, i);
				runtime·throw("bad element");
			}
		}
		if(s != i/2 && s != i/2+1) {
			runtime·printf("bad steal %d, want %d or %d, iter %d\n",
				s, i/2, i/2+1, i);
			runtime·throw("bad steal");
		}
	}
}
