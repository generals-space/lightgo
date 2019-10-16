// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build darwin netbsd openbsd plan9 windows

#include "runtime.h"
#include "stack.h"
#include "../../cmd/ld/textflag.h"

// This implementation depends on OS-specific implementations of
// 本文件中的函数实现依赖于特定平台的 semaphore 休眠与唤醒的机制, mac/windows实现了这种机制.
//	uintptr runtime·semacreate(void)
//		Create a semaphore, which will be assigned to m->waitsema.
//		The zero value is treated as absence of any semaphore,
//		so be sure to return a non-zero value.
//
//	int32 runtime·semasleep(int64 ns)
//		If ns < 0, acquire m->waitsema and return 0.
//		If ns >= 0, try to acquire m->waitsema for at most ns nanoseconds.
//		Return 0 if the semaphore was acquired, -1 if interrupted or timed out.
//
//	int32 runtime·semawakeup(M *mp)
//		Wake up mp, which is or will soon be sleeping on mp->waitsema.
//

enum
{
	LOCKED = 1,

	ACTIVE_SPIN = 4,
	ACTIVE_SPIN_CNT = 30,
	PASSIVE_SPIN = 1,
};

void
runtime·lock(Lock *l)
{
	uintptr v;
	uint32 i, spin;

	if(m->locks++ < 0) runtime·throw("runtime·lock: lock count");

	// Speculative grab for lock.
	// 尝试争用锁, 看这情况像是为第一个调用 Lock() 的协程准备的,
	// 此时没有任何人和ta争抢.
	if(runtime·casp((void**)&l->key, nil, (void*)LOCKED)) return;

	// lock_sema.c 中的 lock/unlock 机制基于 semaphore, 所以判断并执行初始化.
	if(m->waitsema == 0) m->waitsema = runtime·semacreate();

	// On uniprocessor's, no point spinning.
	// On multiprocessors, spin for ACTIVE_SPIN attempts.
	spin = 0;
	if(runtime·ncpu > 1) spin = ACTIVE_SPIN;

	// 这里的循环和 lock_futex.c 中的不太一样, 可以对比分析.
	for(i=0;; i++) {
		v = (uintptr)runtime·atomicloadp((void**)&l->key);
		// 如果 l->key 没有设置 locked 标识位, 则进入此if.
		// 由于关于 spin 条件的前两个if除了 procyield/osyield 没有其他任何操作,
		// 只有第3个else块可能会修改 l->key 的值, 
		// 由于一旦进入此if块, 就会重置变量i
		if((v & LOCKED) == 0) {
unlocked:
			// 加锁成功并返回, 有两种情况能够正常 return.
			// 1. 第一个调用 Lock() 并顺利获得 mutex, 不需要运行到下面的 semasleep();
			// 2. 运行到 semasleep() 休眠后被唤醒, 重置i=0重新开始for循环, 
			// 		由于被唤醒时 lock 标识位一定为0, 所以也有可能进入到这里, 然后从
			//      Lock() 调用中返回.
			if(runtime·casp((void**)&l->key, (void*)v, (void*)(v | LOCKED)))
				return;
			i = 0;
		}
		// 运行到这里, 说明v已经被加上了 locked 标记, l对象已经被其他线程占用.
		// 没办法, 当前协程必须等待, 等待的过程分为以下几个阶段.
		// 在这个大循环里, 可以认为, 
		// 第[0, ACTIVE_SPIN)次区间内进行 procyield() 操作, cpu空转;
		// 第[ACTIVE_SPIN, ACTIVE_SPIN + PASSIVE_SPIN)次区间内进行 osyield() 操作, 
		// 		在os层的让出cpu使用权;
		// 第[ACTIVE_SPIN + PASSIVE_SPIN, )次区间内
		if(i < spin)
			runtime·procyield(ACTIVE_SPIN_CNT);
		else if(i < spin + PASSIVE_SPIN)
			runtime·osyield();
		else {
			// Someone else has it.
			// l->waitm points to a linked list of M's waiting for this lock, 
			// chained through m->nextwaitm.
			// Queue this M.
			// 当前锁对象l已经被其他协程占用, 自旋了这么多次, 仍然没等到释放.
			// 那么将当前m添加到
			// l->key 可以被解释为 waitm, 指向一个 M* 类型的链表, 表示在当前锁对象l上等待的队列, 
			// 各个等待着的 M* 成员间, 通过 nextwaitm 进行连接.
			for(;;) {
				m->nextwaitm = (void*)(v &~ LOCKED); // 移除v的 locked 标识位.
				// 注意这里对目标 l->key 的取值, 把m本身的地址也算上了.
				if(runtime·casp((void**)&l->key, (void*)v, (void*)((uintptr)m | LOCKED)))
					break;
				v = (uintptr)runtime·atomicloadp((void**)&l->key);
				// 运行到这里, 说明抢占失败.
				// 看看这里的判断条件与 unlocked 标签前的 if 条件相同.
				// 注意, 这样跳回去, 会将变量i重置为0.
				if((v & LOCKED) == 0) goto unlocked;
			}
			// 经过上面的for循环, 这里的if条件几乎一定会成立.
			if(v & LOCKED) {
				// Queued. Wait.
				// 休眠, 被唤醒后继续执行. 依赖win平台的信号量实现.
				runtime·semasleep(-1);
				// i被重置后进入下一次循环, 被唤醒时 l->key 应该是没有锁的,
				// 那么在新的循环中就有可能 return.
				i = 0;
			}
		}
	}
}

void
runtime·unlock(Lock *l)
{
	uintptr v;
	M *mp;

	// 这个无限循环, 不成功就不停的做法, 感觉比 lock_futex.c 中的实现要靠谱很多啊
	for(;;) {
		v = (uintptr)runtime·atomicloadp((void**)&l->key);
		// ~~这个事先判断原来的值是否为 locked 状态, 避免在一个 unlocked 的锁上~~
		// ~~再执行 unlock() 操作发生的异常, 这个也比 lock_futex.c 的实现靠谱.~~
		//
		// 好吧我错了, 下面的 if..else.. 目的根本不是这个.
		// 等待一个锁的m不只一个, 所以ta们会以链表的形式存储, 通过 m->nextwaitm 属性指向下一个成员.
		// 该链表的头就是 l->key, 当然需要去除最后一位 locked 的影响, 就可以得到一个m对象的地址.
		//
		// ok, 有了上面的铺垫, 我们可以知道 v 的值其实是 m | LOCKED, 
		// 常规情况下不可能 == LOCKED, 除非这是等待队列中最后一个m.
		if(v == LOCKED) {
			// 如果当前没有其他协程在争用此 mutex, 可以把 l->key 设置为 nil
			if(runtime·casp((void**)&l->key, (void*)LOCKED, nil))
				break;
		} else {
			// Other M's are waiting for the lock.
			// Dequeue an M.
			// 如果还有其他的协程在争用此 mutex (这些协程此时应该在休眠).
			// 话说, 为什么是m??? 不应该是g吗?
			mp = (void*)(v &~ LOCKED); // 移除v的 locked 标识位.
			// 唤醒 mp, 然后把 mp->nextwaitm 这下一个正在等待的m的地址赋值给 l->key, 
			// 相当于从队列头取出一个元素.
			// ~~这个链表就像是栈结构一样, 后进先出...感觉这样很不公平啊~~
			// 错了, 虽然这些m按照后入先出的顺序调用唤醒函数, 但是在 semawakeup() 函数内部,
			// 比如win平台的 WaitForXXX/SetEvent 的PV操作, 目标其实是 mp->waitsema,
			// 而在这个等待队列中, mp->waitsema 的值是同一个.
			// 且在系统层面, waitsema 的唤醒顺序仍然是先进先出.
			if(runtime·casp((void**)&l->key, (void*)v, mp->nextwaitm)) {
				// Dequeued an M. Wake it.
				runtime·semawakeup(mp);
				break;
			}
		}
	}

	// 不管是 futex 还是 sema 实现, 下面的操作都是不可省略的.
	if(--m->locks < 0) runtime·throw("runtime·unlock: lock count");
	// restore the preemption request in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) g->stackguard0 = StackPreempt;
}

// One-time notifications.
void
runtime·noteclear(Note *n)
{
	n->key = 0;
}

void
runtime·notewakeup(Note *n)
{
	M *mp;

	do
		mp = runtime·atomicloadp((void**)&n->key);
	while(!runtime·casp((void**)&n->key, mp, (void*)LOCKED));

	// Successfully set waitm to LOCKED.
	// What was it before?
	if(mp == nil) {
		// Nothing was waiting.  Done.
	} else if(mp == (M*)LOCKED) {
		// Two notewakeups!  Not allowed.
		runtime·throw("notewakeup - double wakeup");
	} else {
		// Must be the waiting m.  Wake it up.
		runtime·semawakeup(mp);
	}
}

void
runtime·notesleep(Note *n)
{
	if(g != m->g0) runtime·throw("notesleep not on g0");

	if(m->waitsema == 0)
		m->waitsema = runtime·semacreate();
	if(!runtime·casp((void**)&n->key, nil, m)) {  // must be LOCKED (got wakeup)
		if(n->key != LOCKED)
			runtime·throw("notesleep - waitm out of sync");
		return;
	}
	// Queued.  Sleep.
	runtime·semasleep(-1);
}

#pragma textflag NOSPLIT
static bool
notetsleep(Note *n, int64 ns, int64 deadline, M *mp)
{
	// Conceptually, deadline and mp are local variables.
	// They are passed as arguments so that the space for them
	// does not count against our nosplit stack sequence.

	// Register for wakeup on n->waitm.
	if(!runtime·casp((void**)&n->key, nil, m)) {  // must be LOCKED (got wakeup already)
		if(n->key != LOCKED) runtime·throw("notetsleep - waitm out of sync");
		return true;
	}

	if(ns < 0) {
		// Queued.  Sleep.
		runtime·semasleep(-1);
		return true;
	}
	// 计时开始
	deadline = runtime·nanotime() + ns;
	for(;;) {
		// Registered. Sleep.
		if(runtime·semasleep(ns) >= 0) {
			// Acquired semaphore, semawakeup unregistered us.
			// Done.
			return true;
		}

		// Interrupted or timed out.  Still registered.  Semaphore not acquired.
		ns = deadline - runtime·nanotime();
		// 如果deadline时间已到, 则braek
		if(ns <= 0) break;
		// Deadline hasn't arrived. Keep sleeping.
		// 如果未到, 则继续沉睡.
	}

	// Deadline arrived. Still registered. Semaphore not acquired.
	// Want to give up and return, but have to unregister first,
	// so that any notewakeup racing with the return does not
	// try to grant us the semaphore when we don't expect it.
	// deadline时间到, 仍在注册状态, 且现在未持有信号量.
	// 现在想放弃并返回, 但首先需要先注销, 
	// 这样任何
	for(;;) {
		// 获取n对象锁定的目标地址
		mp = runtime·atomicloadp((void**)&n->key);
		if(mp == m) {
			// No wakeup yet; unregister if possible.
			// 原子cas操作, 如果n对象锁定的是当前m(即mp), 尝试将其设置为nil, 视为解锁.
			if(runtime·casp((void**)&n->key, mp, nil)) return false;
		} else if(mp == (M*)LOCKED) {
			// Wakeup happened so semaphore is available.
			// Grab it to avoid getting out of sync.
			// wakeup唤醒事件发生, 此时信号量可用, 尝试获取ta以防止同步异常
			if(runtime·semasleep(-1) < 0)
				runtime·throw("runtime: unable to acquire - semaphore out of sync");
			return true;
		} else
			runtime·throw("runtime: unexpected waitm - semaphore out of sync");
	}
}

// ns 纳秒时间长度
// caller: runtime·stoptheworld() 只有这一处.
// 只有g0对象可调用.
bool
runtime·notetsleep(Note *n, int64 ns)
{
	bool res;
	// 只有g0, 且只在m处于gc操作时才可调用.
	if(g != m->g0 && !m->gcing) runtime·throw("notetsleep not on g0");

	if(m->waitsema == 0) m->waitsema = runtime·semacreate();

	res = notetsleep(n, ns, 0, nil);
	return res;
}

// same as runtime·notetsleep, but called on user g (not g0)
// calls only nosplit functions between entersyscallblock/exitsyscall
bool
runtime·notetsleepg(Note *n, int64 ns)
{
	bool res;

	if(g == m->g0) runtime·throw("notetsleepg on g0");

	if(m->waitsema == 0) m->waitsema = runtime·semacreate();

	runtime·entersyscallblock();
	res = notetsleep(n, ns, 0, nil);
	runtime·exitsyscall();
	return res;
}
