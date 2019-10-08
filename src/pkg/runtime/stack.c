// Copyright 2013 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "malloc.h"
#include "stack.h"

/* 栈空间布局
这应该是针对单个函数的, g对象中相关成员只是对当前正在运行的函数来说.

stack0       [stackguard0]        stackbase(sp)         stackguard0(上限)      --> High Address
  ↓               ↓                    ↓                       ↓
  +---------------+--------------------+--------------+·········
  | System        | Stack              | Top          |        |
  +---------------+--------------------+--------------+·········

 */

enum
{
	StackDebug = 0,
};

typedef struct StackCacheNode StackCacheNode;
struct StackCacheNode
{
	StackCacheNode *next;
	// StackCacheBatch 在 runtime.h 中定义, 值为16
	void*	batch[StackCacheBatch-1];
};

static StackCacheNode *stackcache;
static Lock stackcachemu;

// stackcacherefill/stackcacherelease implement a global cache of stack segments.
// The cache is required to prevent unlimited growth of per-thread caches.
// refill 与 release 这两个函数实现了全局栈段的缓存.
// ...说是全局, 其实cache是属于m对象的, 用于某个m的栈分配.
// 这种全局栈缓存的存在是为了防止每个线程的缓存的无限增长.
// 此函数被调用, 意味着当前m的 stackcache 数组为空了,
// 调用此函数来补充.
// caller: runtime·stackalloc()
static void
stackcacherefill(void)
{
	StackCacheNode *n;
	int32 i, pos;

	runtime·lock(&stackcachemu);
	n = stackcache;
	if(n) stackcache = n->next;

	runtime·unlock(&stackcachemu);
	if(n == nil) {
		// 向操作系统申请指定大小的空间, 返回被分配空间的起始地址.
		// 这里要把n指向的地址看作是单个对象看待.
		// 这一个对象中包含 StackCacheBatch 个栈
		n = (StackCacheNode*)runtime·SysAlloc(FixedStack*StackCacheBatch, &mstats.stacks_sys);
		if(n == nil) runtime·throw("out of memory (stackcacherefill)");
		// 将 StackCacheBatch 个栈的起始地址存到 batch 数组.
		// 注意, 跳过了第一个栈的起始地址n.
		for(i = 0; i < StackCacheBatch-1; i++)
			n->batch[i] = (byte*)n + (i+1)*FixedStack;
	}

	// 这里从前向后填充.
	// 看这个for循环的样子, 好像是确定 batch 中的成员是满的啊
	// 从0到14遍历...这是不是意味着此函数每次申请的内存都会
	// 被全部转移到 m->stackcache, 所以上面的if一定会成立啊...
	pos = m->stackcachepos;
	for(i = 0; i < StackCacheBatch-1; i++) {
		m->stackcache[pos] = n->batch[i];
		pos = (pos + 1) % StackCacheSize;
	}
	// 这里存储了地址n.
	// 在 n == nil 这个判断中, 跳过了地址n.
	m->stackcache[pos] = n;
	pos = (pos + 1) % StackCacheSize;
	m->stackcachepos = pos;
	m->stackcachecnt += StackCacheBatch;
}

static void
stackcacherelease(void)
{
	StackCacheNode *n;
	uint32 i, pos;

	pos = (m->stackcachepos - m->stackcachecnt) % StackCacheSize;
	n = (StackCacheNode*)m->stackcache[pos];
	pos = (pos + 1) % StackCacheSize;
	for(i = 0; i < StackCacheBatch-1; i++) {
		n->batch[i] = m->stackcache[pos];
		pos = (pos + 1) % StackCacheSize;
	}
	m->stackcachecnt -= StackCacheBatch;
	runtime·lock(&stackcachemu);
	n->next = stackcache;
	stackcache = n;
	runtime·unlock(&stackcachemu);
}

// 分配大小为n的栈空间, 返回这段空间的起始地址.
// caller: stack.c -> runtime·newstack(), 
// proc.c -> runtime·malg()
void*
runtime·stackalloc(uint32 n)
{
	uint32 pos;
	void *v;

	// Stackalloc must be called on scheduler stack, 
	// so that we never try to grow the stack 
	// during the code that stackalloc runs.
	// Doing so would cause a deadlock (issue 1547).
	// 此函数只能在调度栈 g0 上调用, 普通g对象只能调用 mstackalloc()
	if(g != m->g0)
		runtime·throw("stackalloc not on scheduler stack");

	// Stacks are usually allocated with a fixed-size free-list allocator,
	// but if we need a stack of non-standard size, 
	// we fall back on malloc
	// (assuming that inside malloc 
	// and GC all the stack frames are small,
	// so that we do not deadlock).
	// 栈对象通常是由固定大小的空闲链表分配器来分配的,
	// 所以可以直接向缓存的栈对象链表获取.
	// 但是如果 n 值与常规的栈大小不符, 则需要单独为其分配, 
	// 使用 malloc 完成.
	//
	if(n == FixedStack || m->mallocing || m->gcing) {
		// ...这个检查, 呵呵
		if(n != FixedStack) {
			runtime·printf("stackalloc: in malloc, size=%d want %d\n", FixedStack, n);
			runtime·throw("stackalloc");
		}
		if(m->stackcachecnt == 0) stackcacherefill();
		// 注意这里, 使用时是从后向前.
		// 即, 先获取第n的成员, 再获取第n-1个成员
		pos = m->stackcachepos;
		pos = (pos - 1) % StackCacheSize;
		v = m->stackcache[pos];
		m->stackcachepos = pos;
		m->stackcachecnt--;
		m->stackinuse++;
		return v;
	}
	return runtime·mallocgc(n, 0, FlagNoProfiling|FlagNoGC|FlagNoZero|FlagNoInvokeGC);
}

// 调用 runtime·free 直接释放指定空间 v
// caller: runtime·oldstack()
void
runtime·stackfree(void *v, uintptr n)
{
	uint32 pos;

	if(n == FixedStack || m->mallocing || m->gcing) {
		if(m->stackcachecnt == StackCacheSize)
			stackcacherelease();
		pos = m->stackcachepos;
		m->stackcache[pos] = v;
		m->stackcachepos = (pos + 1) % StackCacheSize;
		m->stackcachecnt++;
		m->stackinuse--;
		return;
	}
	// 在 malloc.goc 中定义
	runtime·free(v);
}

// Called from runtime·lessstack when returning from a function 
// which allocated a new stack segment. 
// The function's return value is in m->cret.
// 在函数结束时被调用(函数调用时一般会分配新栈段)
// caller: runtime·lessstack() 只有这一种调用情况
void
runtime·oldstack(void)
{
	Stktop *top;
	uint32 argsize;
	byte *sp, *old;
	uintptr *src, *dst, *dstend;
	G *gp;
	int64 goid;
	int32 oldstatus;

	gp = m->curg;
	top = (Stktop*)gp->stackbase;
	old = (byte*)gp->stackguard - StackGuard;
	// sp用于定位局部变量
	sp = (byte*)top;
	argsize = top->argsize;

	if(StackDebug) {
		runtime·printf("runtime: oldstack gobuf={pc:%p sp:%p lr:%p} cret=%p argsize=%p\n",
			top->gobuf.pc, top->gobuf.sp, top->gobuf.lr, m->cret, (uintptr)argsize);
	}

	// gp->status is usually Grunning, but it could be Gsyscall if a stack split
	// happens during a function call inside entersyscall.
	// gp->status一般是 Grunning, 但如果在 entersyscall 内部的函数调用期间
	// 发生栈分割, 则可能是 Gsyscall
	oldstatus = gp->status;

	gp->sched = top->gobuf;
	gp->sched.ret = m->cret;
	m->cret = 0; // drop reference
	gp->status = Gwaiting;
	gp->waitreason = "stack unsplit";

	if(argsize > 0) {
		sp -= argsize;
		dst = (uintptr*)top->argp;
		dstend = dst + argsize/sizeof(*dst);
		src = (uintptr*)sp;
		while(dst < dstend) *dst++ = *src++;
	}
	goid = top->gobuf.g->goid;	// fault if g is bad, before gogo
	USED(goid);

	gp->stackbase = top->stackbase;
	gp->stackguard = top->stackguard;
	gp->stackguard0 = gp->stackguard;
	gp->panicwrap = top->panicwrap;

	if(top->free != 0) {
		gp->stacksize -= top->free;
		runtime·stackfree(old, top->free);
	}

	gp->status = oldstatus;
	runtime·gogo(&gp->sched);
}

// enough until runtime.main sets it for real
uintptr runtime·maxstacksize = 1<<20; 

// Called from runtime·newstackcall or from runtime·morestack 
// when a new stack segment is needed. 
// Allocate a new stack big enough for m->moreframesize bytes, 
// copy m->moreargsize bytes to the new frame,
// and then act as though runtime·lessstack 
// called the function at m->morepc.
// 分配一个至少 m->moreframesize 字节的栈空间.
// 
// caller: runtime·newstackcall(), runtime·morestack() 
// 两个都是汇编代码.
// 在原来的栈空间不足, 需要新的栈段的时候被调用.
void
runtime·newstack(void)
{
	int32 framesize, argsize, oldstatus;
	Stktop *top, *oldtop;
	byte *stk;
	uintptr sp;
	uintptr *src, *dst, *dstend;
	G *gp;
	Gobuf label;
	// 是否由 runtime·newstackcall() 函数调用.
	bool newstackcall;
	uintptr free;

	if(m->morebuf.g != m->curg) {
		runtime·printf("runtime: newstack called from g=%p\n"
			"\tm=%p m->curg=%p m->g0=%p m->gsignal=%p\n",
			m->morebuf.g, m, m->curg, m->g0, m->gsignal);
		runtime·throw("runtime: wrong goroutine in newstack");
	}

	// gp->status is usually Grunning, 
	// but it could be Gsyscall if a stack split
	// happens during a function call inside entersyscall.
	gp = m->curg;
	oldstatus = gp->status;

	// framesize 表示本次调用需要分配的栈大小 
	framesize = m->moreframesize;
	argsize = m->moreargsize;
	gp->status = Gwaiting;
	gp->waitreason = "stack split";
	newstackcall = framesize==1;

	if(newstackcall) framesize = 0;

	// For newstackcall the context already points to beginning of runtime·newstackcall.
	if(!newstackcall) runtime·rewindmorestack(&gp->sched);

	sp = gp->sched.sp;
	if(thechar == '6' || thechar == '8') {
		// The call to morestack cost a word.
		sp -= sizeof(uintptr);
	}
	if(StackDebug || sp < gp->stackguard - StackGuard) {
		runtime·printf("runtime: newstack framesize=%p argsize=%p sp=%p stack=[%p, %p]\n"
			"\tmorebuf={pc:%p sp:%p lr:%p}\n"
			"\tsched={pc:%p sp:%p lr:%p ctxt:%p}\n",
			(uintptr)framesize, (uintptr)argsize, 
			sp, gp->stackguard - StackGuard, gp->stackbase,
			m->morebuf.pc, m->morebuf.sp, m->morebuf.lr,
			gp->sched.pc, gp->sched.sp, gp->sched.lr, gp->sched.ctxt);
	}
	// 栈溢出检查
	if(sp < gp->stackguard - StackGuard) {
		runtime·printf("runtime: split stack overflow: %p < %p\n", 
			sp, gp->stackguard - StackGuard);
		runtime·throw("runtime: split stack overflow");
	}

	if(argsize % sizeof(uintptr) != 0) {
		runtime·printf("runtime: stack split with misaligned argsize %d\n", 
			argsize);
		runtime·throw("runtime: stack split argsize");
	}
	// g->stackguard0 的唯一一次有效使用...? 其他地方都是赋值操作...
	// if条件成立, 表示当前g对象已被通知抢占, 但还未让出CPU.
	if(gp->stackguard0 == (uintptr)StackPreempt) {
		// g0不可被抢占
		if(gp == m->g0)
			runtime·throw("runtime: preempt g0");
		if(oldstatus == Grunning && m->p == nil && m->locks == 0)
			runtime·throw("runtime: g is running but p is not");
		if(oldstatus == Gsyscall && m->locks == 0)
			runtime·throw("runtime: stack split during syscall");

		// Be conservative about where we preempt.
		// We are interested in preempting user Go code, not runtime code.
		// 抢占时谨慎一点, 最好先抢占开发者的go协程, 保留运行时的代码...没毛病
		if(oldstatus != Grunning || m->locks || m->mallocing || 
			m->gcing || m->p->status != Prunning) {
			// Let the goroutine keep running for now.
			// gp->preempt is set, so it will be preempted next time.
			// 这一轮协程还可以运行, 但是由于 preempt 已经设置,
			// 下一轮调度就会被抢占了.
			gp->stackguard0 = gp->stackguard;
			gp->status = oldstatus;
			runtime·gogo(&gp->sched);	// never return
		}
		// Act like goroutine called runtime.Gosched.
		gp->status = oldstatus;
		runtime·gosched0(gp);	// never return
	}

	if(newstackcall && 
		m->morebuf.sp - sizeof(Stktop) - argsize - 32 > gp->stackguard) {
		// special case: called from runtime.newstackcall (framesize==1)
		// to call code with an arbitrary argument size,
		// and we have enough space on the current stack.
		// the new Stktop* is necessary to unwind, 
		// but we don't need to create a new segment.
		// 特殊情况: 主调函数为 runtime·newstackcall() framesize == 1
		top = (Stktop*)(m->morebuf.sp - sizeof(*top));
		stk = (byte*)gp->stackguard - StackGuard;
		free = 0;
	} else {
		// allocate new segment.
		// 计算实际需要扩张的栈内存大小.
		framesize += argsize;
		// room for more functions, Stktop.
		framesize += StackExtra;
		if(framesize < StackMin) framesize = StackMin;
		framesize += StackSystem;
		gp->stacksize += framesize;
		// 不可超过最大值
		if(gp->stacksize > runtime·maxstacksize) {
			runtime·printf("runtime: goroutine stack exceeds %D-byte limit\n", 
				(uint64)runtime·maxstacksize);
			runtime·throw("stack overflow");
		}
		// 新栈帧的起始地址.
		stk = runtime·stackalloc(framesize);
		top = (Stktop*)(stk+framesize-sizeof(*top));
		// 此次分配的 framesize 空间目前全部是空闲的.
		free = framesize;
	}

	if(StackDebug) {
		runtime·printf("\t-> new stack [%p, %p]\n", stk, top);
	}

	top->stackbase = gp->stackbase;
	top->stackguard = gp->stackguard;
	top->gobuf = m->morebuf;
	top->argp = m->moreargp;
	top->argsize = argsize;
	top->free = free;
	m->moreargp = nil;
	m->morebuf.pc = (uintptr)nil;
	m->morebuf.lr = (uintptr)nil;
	m->morebuf.sp = (uintptr)nil;

	// copy flag from panic
	top->panic = gp->ispanic;
	gp->ispanic = false;
	
	// if this isn't a panic, maybe we're splitting the stack for a panic.
	// if we're splitting in the top frame, 
	// propagate the panic flag
	// forward so that recover will know we're in a panic.
	oldtop = (Stktop*)top->stackbase;
	if(oldtop != nil && oldtop->panic && 
		top->argp == (byte*)oldtop - oldtop->argsize - gp->panicwrap)
		top->panic = true;

	top->panicwrap = gp->panicwrap;
	gp->panicwrap = 0;

	gp->stackbase = (uintptr)top;
	gp->stackguard = (uintptr)stk + StackGuard;
	gp->stackguard0 = gp->stackguard;

	sp = (uintptr)top;
	if(argsize > 0) {
		sp -= argsize;
		dst = (uintptr*)sp;
		dstend = dst + argsize/sizeof(*dst);
		src = (uintptr*)top->argp;
		while(dst < dstend) *dst++ = *src++;
	}
	if(thechar == '5') {
		// caller would have saved its LR below args.
		sp -= sizeof(void*);
		*(void**)sp = nil;
	}

	// Continue as if lessstack had just called m->morepc
	// (the PC that decided to grow the stack).
	runtime·memclr((byte*)&label, sizeof label);
	label.sp = sp;
	label.pc = (uintptr)runtime·lessstack;
	label.g = m->curg;
	if(newstackcall)
		runtime·gostartcallfn(&label, (FuncVal*)m->cret);
	else {
		runtime·gostartcall(&label, (void(*)(void))gp->sched.pc, gp->sched.ctxt);
		gp->sched.ctxt = nil;
	}
	gp->status = oldstatus;
	runtime·gogo(&label);

	// never return
	*(int32*)345 = 123;
}

// adjust Gobuf as if it executed a call to fn
// and then did an immediate gosave.
void
runtime·gostartcallfn(Gobuf *gobuf, FuncVal *fv)
{
	runtime·gostartcall(gobuf, fv->fn, fv);
}

void
runtime∕debug·setMaxStack(intgo in, intgo out)
{
	out = runtime·maxstacksize;
	runtime·maxstacksize = in;
	FLUSH(&out);
}
