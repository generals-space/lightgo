/*
 * proc_g.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "race.h"
#include "stack.h"
#include "../../cmd/ld/textflag.h"

int32 runtime·gcount(void)
{
	G *gp;
	int32 n, s;

	n = 0;
	runtime·lock(&runtime·sched);
	// TODO(dvyukov): runtime.NumGoroutine() is O(N).
	// We do not want to increment/decrement centralized counter in newproc/goexit,
	// just to make runtime.NumGoroutine() faster.
	// Compromise solution is to introduce per-P counters of active goroutines.
	for(gp = runtime·allg; gp; gp = gp->alllink) {
		s = gp->status;
		if(s == Grunnable || s == Grunning || s == Gsyscall || s == Gwaiting) {
			n++;
		}
	}
	runtime·unlock(&runtime·sched);
	return n;
}

// 唤醒因 runtime·park() 而陷入休眠的 g 对象.
//
// 将 gp 标记为 Grunnable 状态(gp->status 需要是 Gwaiting 状态),
// 并将 gp 通过 runqput() 放到待执行队列中.
//
// caller:
// 在 proc.c 相关代码中没有被调用过, 主调函数都是应用层阻塞结束的时刻,
// 比如 channel 有数据写入/读取, socket 的数据接收/发送等.
//
// Mark gp ready to run.
void runtime·ready(G *gp)
{
	// Mark runnable.
	// disable preemption because it can be holding p in a local var
	m->locks++;
	if(gp->status != Gwaiting) {
		runtime·printf("goroutine %D has status %d\n", gp->goid, gp->status);
		runtime·throw("bad g->status in ready");
	}
	gp->status = Grunnable;
	// 将 gp 对象放到当前 m 绑定的本地 p 对象的 runq 待执行队列中.
	runqput(m->p, gp);
	// TODO: fast atomic
	if(runtime·atomicload(&runtime·sched.npidle) != 0 && runtime·atomicload(&runtime·sched.nmspinning) == 0){
		wakep();
	}
	m->locks--;
	// restore the preemption request in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) {
		g->stackguard0 = StackPreempt;
	} 
}

// 尝试获取一个待执行的 g 任务(分别从本地队列, 全局队列, 以及其他 p 的本地队列里寻找).
//
// caller: 
// 	1. schedule() 只有这一个调用者, m 线程不断寻找待执行任务, 但是全局队列, 还有 m->p 中的本地队列,
// 		都没有可执行任务时, 调用此函数从其他 p 对象, 全局队列, epoll 中获取.
//
// Finds a runnable goroutine to execute.
// Tries to steal from other P's, get g from global queue, poll network.
G* findrunnable(void)
{
	G *gp;
	P *p;
	int32 i;

top:
	if(runtime·sched.gcwaiting) {
		gcstopm();
		goto top;
	}
	// 1. 尝试从当前 m->p 本地队列中获取一个 g 任务
	// local runq
	gp = runqget(m->p);
	if(gp) {
		return gp;
	} 

	// 2. 尝试从全局任务队列中获取一"批" g, 默认数量 sched.runqsize/gomaxprocs+1
	// global runq
	if(runtime·sched.runqsize) {
		runtime·lock(&runtime·sched);
		gp = globrunqget(m->p, 0);
		runtime·unlock(&runtime·sched);
		if(gp) {
			return gp;
		} 
	}
	// poll network(non-blocking)
	gp = runtime·netpoll(false);
	if(gp) {
		injectglist(gp->schedlink);
		gp->status = Grunnable;
		return gp;
	}
	// If number of spinning M's >= number of busy P's, block.
	// This is necessary to prevent excessive CPU consumption
	// when GOMAXPROCS>>1 but the program parallelism is low.
	//
	// TODO: fast atomic
	if(!m->spinning && 2 * runtime·atomicload(&runtime·sched.nmspinning) >= runtime·gomaxprocs - runtime·atomicload(&runtime·sched.npidle)) {
		goto stop;
	}
	if(!m->spinning) {
		m->spinning = true;
		runtime·xadd(&runtime·sched.nmspinning, 1);
	}
	// 3. 随机从其他 P ⾥获取一个 g 任务
	// random steal from other P's
	for(i = 0; i < 2*runtime·gomaxprocs; i++) {
		if(runtime·sched.gcwaiting) {
			goto top;
		}
		p = runtime·allp[runtime·fastrand1()%runtime·gomaxprocs];
		if(p == m->p){
			// 偷到自己了, 返回的 gp 就还是 nil, 继续循环.
			gp = runqget(p);
		} else {
			// 偷⼀半到⾃⼰的 p 队列，并返回⼀个
			gp = runqsteal(m->p, p);
		}
		if(gp) {
			return gp;
		} 
	}
stop:
	// return P and block
	runtime·lock(&runtime·sched);
	if(runtime·sched.gcwaiting) {
		runtime·unlock(&runtime·sched);
		goto top;
	}
	// 再次尝试全局队列
	if(runtime·sched.runqsize) {
		gp = globrunqget(m->p, 0);
		runtime·unlock(&runtime·sched);
		return gp;
	}
	// 还是找不到, 释放 p
	p = releasep();
	pidleput(p);
	runtime·unlock(&runtime·sched);
	if(m->spinning) {
		m->spinning = false;
		runtime·xadd(&runtime·sched.nmspinning, -1);
	}
	// 再次检查所有的 p 队列.
	// check all runqueues once again
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p && p->runqhead != p->runqtail) {
			runtime·lock(&runtime·sched);
			p = pidleget();
			runtime·unlock(&runtime·sched);
			if(p) {
				acquirep(p); // 将 p 拿回来
				goto top; // 再来一次
			}
			break;
		}
	}
	// poll network
	if(runtime·xchg64(&runtime·sched.lastpoll, 0) != 0) {
		if(m->p) {
			runtime·throw("findrunnable: netpoll with p");
		}
		if(m->spinning) {
			runtime·throw("findrunnable: netpoll with spinning");
		}
		// block until new work is available
		gp = runtime·netpoll(true); 
		runtime·atomicstore64(&runtime·sched.lastpoll, runtime·nanotime());
		if(gp) {
			runtime·lock(&runtime·sched);
			p = pidleget();
			runtime·unlock(&runtime·sched);
			if(p) {
				acquirep(p);
				injectglist(gp->schedlink);
				gp->status = Grunnable;
				return gp;
			}
			injectglist(gp);
		}
	}
	// 折腾半天, 还是什么收获都没有, 只好 stop, 将⾃⼰放⼊ sched.midle 队列.
	stopm();
	goto top;
}

// 将 glist 任务列表放回全局调度器的任务队列, 并将其全部标记为 runnable.
// 同时如果全局调度器中还有处于 idle 状态的 p, 就创建更多的 m 来运行这些任务吧.
// ...可以与GC并行运行??? 怎么可能???
//
// Injects the list of runnable G's into the scheduler.
// Can run concurrently with GC.
void injectglist(G *glist)
{
	int32 n;
	G *gp;

	if(glist == nil) {
		return;
	} 

	runtime·lock(&runtime·sched);
	for(n = 0; glist; n++) {
		gp = glist;
		glist = gp->schedlink;
		gp->status = Grunnable;
		globrunqput(gp);
	}
	runtime·unlock(&runtime·sched);
	// 如果全局队列中还有处于 idle 状态的 p, 就创建更多的m来运行这些任务.
	for(; n && runtime·sched.npidle; n--) {
		startm(nil, false);
	}
}

// 结束运行当前的 goroutine.
// go协程都是运行完成后自然结束的, 且没有返回值.
//
// caller:
// 	1. runtime·newproc1() 此处不是调用者, 而是赋值者.
//     因为 go func() 是通过将 exit 压入栈中, 结束时调用此函数自行退出的.
//
// Finishes execution of the current goroutine.
// Need to mark it as nosplit, because it runs with 
// sp > stackbase (as runtime·lessstack).
// Since it does not return it does not matter. 
// But if it is preempted at the split stack check,
// GC will complain about inconsistent sp.
// 
#pragma textflag NOSPLIT
void runtime·goexit(void)
{
	if(raceenabled) {
		runtime·racegoend();
	}
	runtime·mcall(goexit0);
}

// 由 runtime·goexit() 调用 mcall() 将代码切换到 g0 上执行.
// 
// param gp: 即为当前 m 的 g0 .
//
// caller:
// 	1. runtime·goexit()
//
// runtime·goexit continuation on g0.
static void goexit0(G *gp)
{
	// 自定义 runtime 信息打印
	if(runtime·debug.scheddetail >= 3) {
		int64 curg_id = -1;
		if(m->curg != nil) {
			curg_id = m->curg->goid;
		}
		runtime·printf(
			"goexit0() called - global g: %D, m: %d, m.g0: %D, m.curg: %D, m.p: %d, source g: %D\n",
			g->goid, m->id, m->g0->goid, curg_id, m->p->id, gp->goid
		);
	}
	gp->status = Gdead;
	gp->m = nil;
	gp->lockedm = nil;
	m->curg = nil;
	m->lockedg = nil;
	if(m->locked & ~LockExternal) {
		runtime·printf("invalid m->locked = %d\n", m->locked);
		runtime·throw("internal lockOSThread error");
	}
	m->locked = 0;
	// 释放 g 对象额外分配的栈帧
	runtime·unwindstack(gp, nil);
	// 将 g 放回 p.gfree.
	// 如果空闲数量过多, 就移交⼀部分给 sched.gfree, 以便其他 P 获取.
	gfput(m->p, gp);
	// 当前 g 执行完成并退出, m 则重新进入循环, 继续获取 g 任务, 开始下一轮工作.
	schedule();
}

// 为普通g对象gp分配栈空间.
// 由于是通过 runtime·mcall() 调用, 所以当前g对象其实为g0.
// 如果是为 m->g0 分配栈空间, 则可以不经过此函数, 
// 而是直接调用 runtime·stackalloc()
//
// caller: 
// 	1. runtime·malg() 
//
// Hook used by runtime·malg to call runtime·stackalloc on the scheduler stack. 
// This exists because runtime·stackalloc insists
// on being called on the scheduler stack, 
// to avoid trying to grow the stack while allocating a new stack segment.
static void mstackalloc(G *gp)
{
	gp->param = runtime·stackalloc((uintptr)gp->param);
	runtime·gogo(&gp->sched);
}

// 创建一个 g 对象, 为其分配至少 stacksize 字节(一般为 8K)的栈空间, 并返回.
// 如果 stacksize < 0, 则只创建G对象, 不分配栈空间.
//
// caller:
// 	1. runtime·newproc1(fn) 创建一个用于执行 fn 的 g 对象.
//
// Allocate a new g, with a stack big enough for stacksize bytes.
G* runtime·malg(int32 stacksize)
{
	G *newg;
	byte *stk;

	if(StackTop < sizeof(Stktop)) {
		runtime·printf(
			"runtime: SizeofStktop=%d, should be >=%d\n", 
			(int32)StackTop, (int32)sizeof(Stktop)
		);
		runtime·throw("runtime: bad stack.h");
	}
	// 先申请空间
	newg = runtime·malloc(sizeof(G));
	// runtime·allocm() 中调用时, 为 cgo/win 分配 g0 时, stacksize 会指定为-1.
	if(stacksize >= 0) {
		if(g == m->g0) {
			// stk 是分配的栈空间的起始地址
			//
			// running on scheduler stack already.
			stk = runtime·stackalloc(StackSystem + stacksize);
		} else {
			// have to call stackalloc on scheduler stack.
			g->param = (void*)(StackSystem + stacksize);
			// 通过 runtime·mcall() 调用 mstackalloc, 
			// 其实最终调用的还是 runtime·stackalloc()
			runtime·mcall(mstackalloc);
			stk = g->param;
			g->param = nil;
		}
		// stacksize 为当前分配的栈大小(即上面的 stk 变量的大小)
		newg->stacksize = StackSystem + stacksize;
		// stack0 是当前栈空间的起始地址
		newg->stack0 = (uintptr)stk;
		newg->stackguard0 = newg->stackguard;
		// stackguard 当前栈空间的上限
		newg->stackguard = (uintptr)stk + StackGuard;
		// stackbase 是当前栈顶部top部分的起始地址
		newg->stackbase = (uintptr)stk + StackSystem + stacksize - sizeof(Stktop);
		runtime·memclr((byte*)newg->stackbase, sizeof(Stktop));
	}
	return newg;
}

// golang原生: go func() 函数创建协程.
//
// 创建一个 g 对象, 用来运行 fn, 传入 siz 个字节的参数, 然后将g对象放到 waiting 队列等待执行.
//
// 调用栈不可分割, 因为这里假设传入的参数紧随在 fn 的地址之后, 栈分割发生时就无法被正确获取到.
// ...因为此函数直接通过 siz 参数确定传给 fn 的参数, 栈分割会影响其参数的获取.
//
// Create a new g running fn with siz bytes of arguments.
// Put it on the queue of g's waiting to run.
// The compiler turns a go statement into a call to this.
// Cannot split the stack because it assumes that the arguments
// are available sequentially after &fn; 
// they would not be copied if a stack split occurred. 
// It's OK for this to call functions that split the stack. 这句啥意思???
#pragma textflag NOSPLIT
void runtime·newproc(int32 siz, FuncVal* fn, ...)
{
	byte *argp;

	argp = (byte*)(&fn+1);
	runtime·newproc1(fn, argp, siz, 0, runtime·getcallerpc(&siz));
}

// 创建一个新的 g 对象, 用来执行 fn 函数, 传入的参数起始地址为 argp, 一共 narg bytes,
// 并且返回 nret bytes 的结果.
//
// param argp: 传入参数的地址
// param narg: 传入参数的大小
// param nret: 返回值大小.
// param callerpc: 主调函数的地址, pc即程序计数器, 对 g 切换是有用的.
//
// 新创建的 g 对象会放到 waiting 队列等待执行.
//
// caller:
// 	1. runtime·main() 在 runtime 初始化完成后, 执行用户级别 init 行为之前, 调用此函数创建 gc 协程.
// 	2. runtime·newproc()
//
// Create a new g running fn with narg bytes of arguments starting
// at argp and returning nret bytes of results. 
// callerpc is the address of the go statement that created this. 
// The new g is put on the queue of g's waiting to run.
G* runtime·newproc1(FuncVal *fn, byte *argp, int32 narg, int32 nret, void *callerpc)
{
	byte *sp;
	G *newg;
	int32 siz;

	//runtime·printf("newproc1 %p %p narg=%d nret=%d\n", fn->fn, argp, narg, nret);
	// disable preemption because it can be holding p in a local var
	// 禁止抢占(这里的抢占是指谁抢占谁???)
	m->locks++; 
	siz = narg + nret;
	siz = (siz+7) & ~7;

	// We could instead create a secondary stack frame
	// and make it look like goexit was on the original but
	// the call to the actual goroutine function was split.
	// Not worth it: this is almost always an error.
	// 参数占用空间太大.
	// 虽然可以创建一个2级栈帧, 然后让ta看起来像 goexit 在最初的栈上,
	// 但是这样的话, 对 g 中函数的调用是被拆分过的.
	// 不值得, 这种情况几乎完全是不该出现的错误
	if(siz > StackMin - 1024){
		runtime·throw("runtime.newproc: function arguments too large for new goroutine");
	}

	// 从指定 p (当前 m 绑定的 p)的本地队列中取一个 g 任务对象(返回的 g 对象一定是空闲的)
	if((newg = gfget(m->p)) != nil) {
		if(newg->stackguard - StackGuard != newg->stack0){
			runtime·throw("invalid stack in newg");
		}
	} else {
		// 如果获取失败(比如进程初始启动, 要创建第1个 g 对象), 则新建一个 g.
		newg = runtime·malg(StackMin);
		runtime·lock(&runtime·sched);
		// 如果全局 g 队列为空, 则将新建的 g 对象添加进去, 否则追加到队尾.
		if(runtime·lastg == nil){ 
			runtime·allg = newg;
		} else{
			runtime·lastg->alllink = newg;
		}
		runtime·lastg = newg;
		runtime·unlock(&runtime·sched);
	}

	// 将执行参数入栈.
	// sp 用于定位参数和返回值, 但是用 g 对象执行的函数应该是没有返回值的.
	sp = (byte*)newg->stackbase;
	sp -= siz;
	// 将从 argp 处, narg 大小的空间, 复制到 sp 处.
	runtime·memmove(sp, argp, narg);

	// 初始化 G.sched, 该结构⽤于保存执⾏现场数据
	runtime·memclr((byte*)&newg->sched, sizeof newg->sched);
	newg->sched.sp = (uintptr)sp;
	newg->sched.pc = (uintptr)runtime·goexit;
	newg->sched.g = newg;
	runtime·gostartcallfn(&newg->sched, fn);
	newg->gopc = (uintptr)callerpc;
	newg->status = Grunnable;
	newg->goid = runtime·xadd64(&runtime·sched.goidgen, 1);
	newg->panicwrap = 0;
	if(raceenabled) {
		newg->racectx = runtime·racegostart((void*)callerpc);
	}
	runqput(m->p, newg);

	// 尝试唤醒某个 M 开始执⾏(有空闲 P, 没有处于⾃旋等待的 M)
	// 开发者代码中通过 go func() 创建的协程不一定能立刻开始执行, wakep() 可能没有效果, 可以理解.
	// 不过这里为什么要额外判断空闲 p 和 自旋的 m ???
	if(runtime·atomicload(&runtime·sched.npidle) != 0 && 
		runtime·atomicload(&runtime·sched.nmspinning) == 0 && 
		fn->fn != runtime·main) { // TODO: fast atomic
		wakep();
	}

	m->locks--;
	// 重新开启抢占
	// restore the preemption request in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) {
		g->stackguard0 = StackPreempt;
	}
	return newg;
}

// Put on gfree list.
// If local list is too long, transfer a batch to the global list.
static void gfput(P *p, G *gp)
{
	if(gp->stackguard - StackGuard != gp->stack0) {
		runtime·throw("invalid stack in gfput");
	}
	gp->schedlink = p->gfree;
	p->gfree = gp;
	p->gfreecnt++;
	if(p->gfreecnt >= 64) {
		runtime·lock(&runtime·sched.gflock);
		while(p->gfreecnt >= 32) {
			p->gfreecnt--;
			gp = p->gfree;
			p->gfree = gp->schedlink;
			gp->schedlink = runtime·sched.gfree;
			runtime·sched.gfree = gp;
		}
		runtime·unlock(&runtime·sched.gflock);
	}
}

// 从指定p的本地任务队列中获取一个待执行的 g 任务对象.
// 如果队列为空, 则从全局队列中取一些 g 任务过来.
// 
// caller:
// 	1. runtime·newproc1() 开发者通过 go func() 创建协程时, 调用此方法取得一个 g 对象.
//
// Get from gfree list.
// If local list is empty, grab a batch from global list.
static G* gfget(P *p)
{
	G *gp;

retry:
	// gp 是 p 队列本地的 g 队列
	gp = p->gfree;
	// 如果 p 本地的 g 空闲队列为空, 但全局 g 空闲队列还有值, 则从全局队列中取一些过来.
	if(gp == nil && runtime·sched.gfree) {
		runtime·lock(&runtime·sched.gflock);
		// 如果全局 g 队列的数量足够, 则最多可以取 32 个 g 对象放到本地.
		while(p->gfreecnt < 32 && runtime·sched.gfree) {
			p->gfreecnt++;
			gp = runtime·sched.gfree;
			runtime·sched.gfree = gp->schedlink;
			// 每从全局 g 队列中取出一个成员, 就放到本地 g 队列的链表头.
			gp->schedlink = p->gfree;
			p->gfree = gp;
		}
		runtime·unlock(&runtime·sched.gflock);
		goto retry;
	}
	if(gp) {
		p->gfree = gp->schedlink;
		p->gfreecnt--;
	}
	return gp;
}

// 清理目标 p 缓存的本地g任务列表, 但不是直接删除, 而是转移到全局任务列表(头部).
//
// caller: 
// 	1. procresize() 调整 p 数量时, 调用此函数将多余的 p 对象中的任务空间回收掉.
//
// Purge all cached G's from gfree list to the global list.
void gfpurge(P *p)
{
	G *gp;
	// 注意: p->gfree 是不需要加锁的.
	// 加锁要保护的是 runtime·sched.gfree
	runtime·lock(&runtime·sched.gflock);
	while(p->gfreecnt) {
		p->gfreecnt--;
		gp = p->gfree;
		p->gfree = gp->schedlink;
		gp->schedlink = runtime·sched.gfree;
		runtime·sched.gfree = gp;
	}
	runtime·unlock(&runtime·sched.gflock);
}
