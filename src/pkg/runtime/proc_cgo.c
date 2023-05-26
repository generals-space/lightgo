/*
 * proc_cgo.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "race.h"
#include "../../cmd/ld/textflag.h"

M*	runtime·extram; // 这个m链表貌似和 cgo 调用相关.

static M* lockextra(bool nilokay);
static void unlockextra(M*);

// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> runtime·cgocallback()
//
// needm is called when a cgo callback happens on a
// thread without an m (a thread not created by Go).
// In this case, needm is expected to find an m to use
// and return with m, g initialized correctly.
// Since m and g are not set now (likely nil, but see below)
// needm is limited in what routines it can call. In particular
// it can only call nosplit functions (textflag 7) and cannot
// do any scheduling that requires an m.
//
// In order to avoid needing heavy lifting here, we adopt
// the following strategy: there is a stack of available m's
// that can be stolen. Using compare-and-swap
// to pop from the stack has ABA races, so we simulate
// a lock by doing an exchange (via casp) to steal the stack
// head and replace the top pointer with MLOCKED (1).
// This serves as a simple spin lock that we can use even
// without an m. The thread that locks the stack in this way
// unlocks the stack by storing a valid stack head pointer.
//
// In order to make sure that there is always an m structure
// available to be stolen, we maintain the invariant that there
// is always one more than needed. At the beginning of the
// program (if cgo is in use) the list is seeded with a single m.
// If needm finds that it has taken the last m off the list, its job
// is - once it has installed its own m so that it can do things like
// allocate memory - to create a spare m and put it on the list.
//
// Each of these extra m's also has a g0 and a curg that are
// pressed into service as the scheduling stack and current
// goroutine for the duration of the cgo callback.
//
// When the callback is done with the m, it calls dropm to
// put the m back on the list.
#pragma textflag NOSPLIT
void runtime·needm(byte x)
{
	M *mp;

	if(runtime·needextram) {
		// Can happen if C/C++ code calls Go from a global ctor.
		// Can not throw, because scheduler is not initialized yet.
		runtime·write(2, "fatal error: cgo callback before cgo call\n",
			sizeof("fatal error: cgo callback before cgo call\n")-1);
		runtime·exit(1);
	}

	// Lock extra list, take head, unlock popped list.
	// nilokay=false is safe here because of the invariant above,
	// that the extra list always contains or will soon contain
	// at least one m.
	mp = lockextra(false);

	// Set needextram when we've just emptied the list,
	// so that the eventual call into cgocallbackg will
	// allocate a new m for the extra list. We delay the
	// allocation until then so that it can be done
	// after exitsyscall makes sure it is okay to be
	// running at all (that is, there's no garbage collection
	// running right now).
	mp->needextram = mp->schedlink == nil;
	unlockextra(mp->schedlink);

	// Install m and g (= m->g0) and set the stack bounds
	// to match the current stack. We don't actually know
	// how big the stack is, like we don't know how big any
	// scheduling stack is, but we assume there's at least 32 kB,
	// which is more than enough for us.
	runtime·setmg(mp, mp->g0);
	g->stackbase = (uintptr)(&x + 1024);
	g->stackguard = (uintptr)(&x - 32*1024);
	g->stackguard0 = g->stackguard;

#ifdef GOOS_windows
#ifdef GOARCH_386
	// On windows/386, we need to put an SEH frame (two words)
	// somewhere on the current stack. We are called from cgocallback_gofunc
	// and we know that it will leave two unused words below m->curg->sched.sp.
	// Use those.
	m->seh = (SEH*)((uintptr*)&x + 1);
#endif
#endif

	// Initialize this thread to use the m.
	runtime·asminit();
	runtime·minit();
}

// caller: 
// 	1. src/pkg/runtime/cgocall.c -> runtime·cgocall()
// 	2. src/pkg/runtime/cgocall.c -> runtime·cgocallbackg()
// 而且在函数内部有一次递归调用.
// 应该可以说这就是为了 cgo 调用而存在的吧?
//
// newextram allocates an m and puts it on the extra list.
// It is called with a working local m, 
// so that it can do things like call schedlock and allocate.
void runtime·newextram(void)
{
	M *mp, *mnext;
	G *gp;

	// Create extra goroutine locked to extra m.
	// The goroutine is the context in which the cgo callback will run.
	// The sched.pc will never be returned to, but setting it to
	// runtime.goexit makes clear to the traceback routines where
	// the goroutine stack ends.
	// 创建 extra 的g和m对象, 并将其双向绑定.
	mp = runtime·allocm(nil);
	gp = runtime·malg(4096);
	gp->sched.pc = (uintptr)runtime·goexit;
	gp->sched.sp = gp->stackbase;
	gp->sched.lr = 0;
	gp->sched.g = gp;
	gp->syscallpc = gp->sched.pc;
	gp->syscallsp = gp->sched.sp;
	gp->syscallstack = gp->stackbase;
	gp->syscallguard = gp->stackguard;
	gp->status = Gsyscall;
	mp->curg = gp;
	mp->locked = LockInternal;
	mp->lockedg = gp;
	gp->lockedm = mp;
	gp->goid = runtime·xadd64(&runtime·sched.goidgen, 1);
	if(raceenabled)
		gp->racectx = runtime·racegostart(runtime·newextram);
	// put on allg for garbage collector
	runtime·lock(&runtime·sched);
	if(runtime·lastg == nil)
		runtime·allg = gp;
	else
		runtime·lastg->alllink = gp;
	runtime·lastg = gp;
	runtime·unlock(&runtime·sched);

	// Add m to the extra list.
	mnext = lockextra(true);
	mp->schedlink = mnext;
	unlockextra(mp);
}

// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> runtime·cgocallback_gofunc()
//
// dropm is called when a cgo callback has called needm but is now
// done with the callback and returning back into the non-Go thread.
// It puts the current m back onto the extra list.
//
// The main expense here is the call to signalstack to release the
// m's signal stack, and then the call to needm on the next callback
// from this thread. It is tempting to try to save the m for next time,
// which would eliminate both these costs, but there might not be
// a next time: the current thread (which Go does not control) might exit.
// If we saved the m for that thread, there would be an m leak each time
// such a thread exited. Instead, we acquire and release an m on each
// call. These should typically not be scheduling operations, just a few
// atomics, so the cost should be small.
//
// TODO(rsc): An alternative would be to allocate a dummy pthread per-thread
// variable using pthread_key_create. Unlike the pthread keys we already use
// on OS X, this dummy key would never be read by Go code. It would exist
// only so that we could register at thread-exit-time destructor.
// That destructor would put the m back onto the extra list.
// This is purely a performance optimization. The current version,
// in which dropm happens on each cgo call, is still correct too.
// We may have to keep the current version on systems with cgo
// but without pthreads, like Windows.
void runtime·dropm(void)
{
	M *mp, *mnext;

	// Undo whatever initialization minit did during needm.
	runtime·unminit();

	// Clear m and g, and return m to the extra list.
	// After the call to setmg we can only call nosplit functions.
	mp = m;
	runtime·setmg(nil, nil);

	mnext = lockextra(true);
	mp->schedlink = mnext;
	unlockextra(mp);
}

#define MLOCKED ((M*)1)

// lockextra locks the extra list and returns the list head.
// The caller must unlock the list by storing a new list head
// to runtime.extram. If nilokay is true, then lockextra will
// return a nil list head if that's what it finds. If nilokay is false,
// lockextra will keep waiting until the list head is no longer nil.
#pragma textflag NOSPLIT
static M* lockextra(bool nilokay)
{
	M *mp;
	void (*yield)(void);

	for(;;) {
		// mp 为 extram 链表头
		mp = runtime·atomicloadp(&runtime·extram);
		if(mp == MLOCKED) {
			yield = runtime·osyield;
			yield();
			continue;
		}
		if(mp == nil && !nilokay) {
			runtime·usleep(1);
			continue;
		}
		if(!runtime·casp(&runtime·extram, mp, MLOCKED)) {
			yield = runtime·osyield;
			yield();
			continue;
		}
		break;
	}
	return mp;
}

#pragma textflag NOSPLIT
static void unlockextra(M *mp)
{
	runtime·atomicstorep(&runtime·extram, mp);
}
