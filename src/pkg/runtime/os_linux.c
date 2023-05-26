// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "defs_GOOS_GOARCH.h"
#include "os_GOOS.h"
#include "signal_unix.h"
#include "stack.h"
#include "../../cmd/ld/textflag.h"

extern SigTab runtime·sigtab[];

static Sigset sigset_none;
static Sigset sigset_all = { ~(uint32)0, ~(uint32)0 };

// Linux futex.
//
//	futexsleep(uint32 *addr, uint32 val)
//	futexwakeup(uint32 *addr)
//
// Futexsleep atomically checks if *addr == val and if so, sleeps on addr.
// Futexwakeup wakes up threads sleeping on addr.
// Futexsleep is allowed to wake up spuriously.
// futexsleep()原子地检查 *addr == val 是否为true, 如果为true, 就在addr上陷入休眠.
// futexwakeup()则是将在addr处休眠的协程唤醒.
// futexsleep()允许被误唤醒...唤醒错了???

// linux的futex系统调用是单个函数, 通过WAIT/WAKE两个标识位实现休眠和唤醒两种不同的操作.
// 这里把对futex的调用封装成了两个函数
enum
{
	FUTEX_WAIT = 0,
	FUTEX_WAKE = 1,
};

// Atomically,
//	if(*addr == val) sleep
// Might be woken up spuriously; that's allowed.
// Don't sleep longer than ns; ns < 0 means forever.
// 原子地检查 *addr == val 是否为true, 如果是, 则继续休眠
// 允许被误唤醒
// 休眠时间最长为 ns 纳秒, 如果 ns < 0, 表示永久休眠.
#pragma textflag NOSPLIT
void runtime·futexsleep(uint32 *addr, uint32 val, int64 ns)
{
	Timespec ts;

	// Some Linux kernels have a bug where futex of
	// FUTEX_WAIT returns an internal error code as an errno. 
	// Libpthread ignores the return value here,
	// and so can we: as it says a few lines up,
	// spurious wakeups are allowed.

	// ns小于0, 则永久休眠, 直到被唤醒.
	if(ns < 0) {
		runtime·futex(addr, FUTEX_WAIT, val, nil, nil, 0);
		return;
	}
	// NOTE: tv_nsec is int64 on amd64, so this assumes a little-endian system.
	ts.tv_nsec = 0;
	ts.tv_sec = runtime·timediv(ns, 1000000000LL, (int32*)&ts.tv_nsec);
	runtime·futex(addr, FUTEX_WAIT, val, &ts, nil, 0);
}

// wakeup可以唤醒在 addr 处休眠的进程, 可能不只一个,
// cnt 可以指定此次操作能够唤醒多少个休眠中的进程.
//
// If any procs are sleeping on addr, wake up at most cnt.
void runtime·futexwakeup(uint32 *addr, uint32 cnt)
{
	int64 ret;

	ret = runtime·futex(addr, FUTEX_WAKE, cnt, nil, nil, 0);

	if(ret >= 0) {
		return;
	} 

	// I don't know that futex wakeup can return EAGAIN or EINTR, 
	// but if it does, it would be safe to loop and call futex again.
	// ...这里很难理解
	runtime·printf("futexwakeup addr=%p returned %D\n", addr, ret);
	*(int32*)0x1006 = 0x1006;
}

// runtime·sched_getaffinity()是汇编代码, 是名为 sched_getaffinity()的系统调用.
extern runtime·sched_getaffinity(uintptr pid, uintptr len, uintptr *buf);

// getproccount 获取物理机实际 cpu 核数
//
// caller:
// 	1. runtime·osinit() 只有这一处, 在 runtime 启动时被调用.
static int32
getproccount(void)
{
	uintptr buf[16], t;
	int32 r, cnt, i;

	cnt = 0;
	r = runtime·sched_getaffinity(0, sizeof(buf), buf);
	if(r > 0) {
		for(i = 0; i < r/sizeof(buf[0]); i++) {
			t = buf[i];
			t = t - ((t >> 1) & 0x5555555555555555ULL);
			t = (t & 0x3333333333333333ULL) + ((t >> 2) & 0x3333333333333333ULL);
			cnt += (int32)((((t + (t >> 4)) & 0xF0F0F0F0F0F0F0FULL) * 0x101010101010101ULL) >> 56);
		}
	}

	return cnt ? cnt : 1;
}

// Clone, the Linux rfork.
enum
{
	CLONE_VM = 0x100,
	CLONE_FS = 0x200,
	CLONE_FILES = 0x400,
	CLONE_SIGHAND = 0x800,
	CLONE_PTRACE = 0x2000,
	CLONE_VFORK = 0x4000,
	CLONE_PARENT = 0x8000,
	CLONE_THREAD = 0x10000,
	CLONE_NEWNS = 0x20000,
	CLONE_SYSVSEM = 0x40000,
	CLONE_SETTLS = 0x80000,
	CLONE_PARENT_SETTID = 0x100000,
	CLONE_CHILD_CLEARTID = 0x200000,
	CLONE_UNTRACED = 0x800000,
	CLONE_CHILD_SETTID = 0x1000000,
	CLONE_STOPPED = 0x2000000,
	CLONE_NEWUTS = 0x4000000,
	CLONE_NEWIPC = 0x8000000,
};

// 调用 clone 系统调用创建新的系统线程, 并绑定到传入的 M 对象, 新线程将执行 runtime·mstart() 方法.
//
// 调用者不一定非得是 m0
//
// clone 系统调用由汇编代码实现.
//
// caller:
// 	1. src/pkg/runtime/proc.c -> newm() 创建 m 对象时, 调用此函数创建系统线程. 只有这一处.
void runtime·newosproc(M *mp, void *stk)
{
	int32 ret;
	int32 flags;
	Sigset oset;

	/*
	 * note: strace gets confused if we use CLONE_PTRACE here.
	 */
	flags = CLONE_VM	/* share memory */
		| CLONE_FS	/* share cwd, etc */
		| CLONE_FILES	/* share fd table */
		| CLONE_SIGHAND	/* share sig handler table */
		| CLONE_THREAD	/* revisit - okay for now */
		;

	// so 386 asm can find it
	mp->tls[0] = mp->id;
	if(0){
		runtime·printf(
			"newosproc stk=%p m=%p g=%p clone=%p id=%d/%d ostk=%p\n",
			stk, mp, mp->g0, runtime·clone, mp->id, (int32)mp->tls[0], &mp
		);
	}

	// Disable signals during clone, so that the new thread starts
	// with signals disabled.  It will enable them in minit.
	runtime·rtsigprocmask(SIG_SETMASK, &sigset_all, &oset, sizeof oset);
	ret = runtime·clone(flags, stk, mp, mp->g0, runtime·mstart);
	runtime·rtsigprocmask(SIG_SETMASK, &oset, nil, sizeof oset);

	if(ret < 0) {
		runtime·printf(
			"runtime: failed to create new OS thread (have %d already; errno=%d)\n", 
			runtime·mcount(), -ret
		);
		runtime·throw("runtime.newosproc");
	}

	// 自定义 runtime 信息打印
	if(runtime·debug.scheddetail >= 3) {
		// 注意, 调用 clone 时, 传入的 mp 对象还只是一个空的结构体, 并没有绑定任何 p 跟 g,
		// 所以这里都是空的, 就不打印ta们的 id 值了.
		runtime·printf(
			"newosproc m=%d g=%p p=%p newproc=%d\n",
			mp->id, mp->curg, mp->p, ret
		);
	}
}

// caller: 
// 	1. src/pkg/runtime/asm_amd64.s -> _rt0_go() 主程序入口
void runtime·osinit(void)
{
	runtime·ncpu = getproccount();
}

// Random bytes initialized at startup.  These come
// from the ELF AT_RANDOM auxiliary vector (vdso_linux_amd64.c).
byte*	runtime·startup_random_data;
uint32	runtime·startup_random_data_len;

void runtime·get_random_data(byte **rnd, int32 *rnd_len)
{
	if(runtime·startup_random_data != nil) {
		*rnd = runtime·startup_random_data;
		*rnd_len = runtime·startup_random_data_len;
	} else {
		#pragma dataflag NOPTR
		static byte urandom_data[HashRandomBytes];
		int32 fd;
		fd = runtime·open("/dev/urandom", 0 /* O_RDONLY */, 0);
		if(runtime·read(fd, urandom_data, HashRandomBytes) == HashRandomBytes) {
			*rnd = urandom_data;
			*rnd_len = HashRandomBytes;
		} else {
			*rnd = nil;
			*rnd_len = 0;
		}
		runtime·close(fd);
	}
}

void runtime·goenvs(void)
{
	runtime·goenvs_unix();
}

// Called to initialize a new m (including the bootstrap m).
// Called on the parent thread (main thread in case of bootstrap), can allocate memory.
void runtime·mpreinit(M *mp)
{
	mp->gsignal = runtime·malg(32*1024);	// OS X wants >=8K, Linux >=2K
}

// Called to initialize a new m (including the bootstrap m).
// Called on the new thread, can not allocate memory.
void runtime·minit(void)
{
	// Initialize signal handling.
	runtime·signalstack((byte*)m->gsignal->stackguard - StackGuard, 32*1024);
	runtime·rtsigprocmask(SIG_SETMASK, &sigset_none, nil, sizeof(Sigset));
}

// Called from dropm to undo the effect of an minit.
void runtime·unminit(void)
{
	runtime·signalstack(nil, 0);
}

void runtime·sigpanic(void)
{
	switch(g->sig) {
	case SIGBUS:
		if(g->sigcode0 == BUS_ADRERR && g->sigcode1 < 0x1000) {
			if(g->sigpc == 0) {
				runtime·panicstring("call of nil func value");
			}
			runtime·panicstring("invalid memory address or nil pointer dereference");
		}
		runtime·printf("unexpected fault address %p\n", g->sigcode1);
		runtime·throw("fault");
	case SIGSEGV:
		if((g->sigcode0 == 0 || 
			g->sigcode0 == SEGV_MAPERR || 
			g->sigcode0 == SEGV_ACCERR) && 
			g->sigcode1 < 0x1000) {
			if(g->sigpc == 0) {
				runtime·panicstring("call of nil func value");
			}
			runtime·panicstring("invalid memory address or nil pointer dereference");
		}
		runtime·printf("unexpected fault address %p\n", g->sigcode1);
		runtime·throw("fault");
	case SIGFPE:
		switch(g->sigcode0) {
		case FPE_INTDIV:
			runtime·panicstring("integer divide by zero");
		case FPE_INTOVF:
			runtime·panicstring("integer overflow");
		}
		runtime·panicstring("floating point error");
	}
	runtime·panicstring(runtime·sigtab[g->sig].name);
}

uintptr runtime·memlimit(void)
{
	Rlimit rl;
	extern byte text[], end[];
	uintptr used;

	if(runtime·getrlimit(RLIMIT_AS, &rl) != 0)
		return 0;
	if(rl.rlim_cur >= 0x7fffffff)
		return 0;

	// Estimate our VM footprint excluding the heap.
	// Not an exact science: use size of binary plus
	// some room for thread stacks.
	used = end - text + (64<<20);
	if(used >= rl.rlim_cur)
		return 0;

	// If there's not at least 16 MB left, we're probably
	// not going to be able to do much.  Treat as no limit.
	rl.rlim_cur -= used;
	if(rl.rlim_cur < (16<<20))
		return 0;

	return rl.rlim_cur - used;
}

#ifdef GOARCH_386
#define sa_handler k_sa_handler
#endif

/*
 * This assembler routine takes the args from registers, puts them on the stack,
 * and calls sighandler().
 */
extern void runtime·sigtramp(void);
extern void runtime·sigreturn(void);	// calls runtime·sigreturn

void
runtime·setsig(int32 i, GoSighandler *fn, bool restart)
{
	Sigaction sa;

	runtime·memclr((byte*)&sa, sizeof sa);
	sa.sa_flags = SA_ONSTACK | SA_SIGINFO | SA_RESTORER;
	if(restart)
		sa.sa_flags |= SA_RESTART;
	sa.sa_mask = ~0ULL;
	// TODO(adonovan): Linux manpage says "sa_restorer element is
	// obsolete and should not be used".  Avoid it here, and test.
	sa.sa_restorer = (void*)runtime·sigreturn;
	if(fn == runtime·sighandler)
		fn = (void*)runtime·sigtramp;
	sa.sa_handler = fn;
	if(runtime·rt_sigaction(i, &sa, nil, sizeof(sa.sa_mask)) != 0)
		runtime·throw("rt_sigaction failure");
}

GoSighandler*
runtime·getsig(int32 i)
{
	Sigaction sa;

	runtime·memclr((byte*)&sa, sizeof sa);
	if(runtime·rt_sigaction(i, nil, &sa, sizeof(sa.sa_mask)) != 0)
		runtime·throw("rt_sigaction read failure");
	if((void*)sa.sa_handler == runtime·sigtramp)
		return runtime·sighandler;
	return (void*)sa.sa_handler;
}

void
runtime·signalstack(byte *p, int32 n)
{
	Sigaltstack st;

	st.ss_sp = p;
	st.ss_size = n;
	st.ss_flags = 0;
	if(p == nil)
		st.ss_flags = SS_DISABLE;
	runtime·sigaltstack(&st, nil);
}
