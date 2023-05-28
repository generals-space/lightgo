// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "proc.h"
#include "arch_amd64.h"
#include "zaexperiment.h"
#include "malloc.h"
#include "stack.h"
#include "race.h"
#include "type.h"
#include "../../cmd/ld/textflag.h"

// 表示 runtime 中 p 的**实际数量**, 取值范围在 [1, MaxGomaxprocs(256, 固定值)]
//
// 所有的 p 都存在于 runtime·allp 数组中. 该数组长度为 MaxGomaxprocs, 但实际可能不会占满.
//
// 在 runtime 中, 经常有如下方式遍历 p 列表.
//
// for(i = 0; i < runtime·gomaxprocs; i++) {
//     p = runtime·allp[i];
//     ...省略
// }
//
// 在 runtime·schedinit() 初始化时, 就通过 procresize() 函数设置了该值.
//
// 另外, 由于 golang 提供了 runtime 接口, 所以这个值也可以在程序运行过程中被动态地修改.
int32	runtime·gomaxprocs;
// runtime·ncpu 表示当前物理机实际的 cpu 核数.
//
// 在 src/pkg/runtime/os_linux.c -> runtime·osinit() 函数中被赋值.
int32	runtime·ncpu;

uint32	runtime·needextram;
bool	runtime·iscgo;
G*	runtime·allg; // G对象链表
G*	runtime·lastg;
M*	runtime·allm;
int8*	runtime·goos;

// 看样子是先创建任务对象g, 再启动m去执行ta.
//
// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> _rt0_go() 程序启动时的初始化汇总函数
//
// The bootstrap sequence is:
//
//	call osinit
//	call schedinit
//	make & queue new G
//	call runtime·mstart
//
// The new G calls runtime·main.
void runtime·schedinit(void)
{
	int32 n, procs;
	byte *p;
	Eface i;

	// 最大系统线程数量限制, 参考标准库 runtime/debug.SetMaxThreads
	runtime·sched.maxmcount = 10000;
	runtime·precisestack = haveexperiment("precisestack");

	runtime·mprofinit();
	// 初始化内存分配器
	runtime·mallocinit();
	// 初始化当前M, 即 m0 主线程.
	//
	// m 对象为全局对象(应该是所谓的 m0), 在 src/pkg/runtime/runtime.h 中, 
	// 通过"extern	register	M*	m;"声明.
	mcommoninit(m);

	// Initialize the itable value for newErrorCString,
	// so that the next time it gets called, 
	// possibly in a fault during a garbage collection, 
	// it will not need to allocated memory.
	runtime·newErrorCString(0, &i);

	// 处理命令行参数
	runtime·goargs();
	// 处理环境变量
	runtime·goenvs();
	// 处理 GODEBUG, GOTRACEBACK 调试相关的环境变量设置
	runtime·parsedebugvars();

	// Allocate internal symbol table representation now, 
	// we need it for GC anyway.
	runtime·symtabinit();

	runtime·sched.lastpoll = runtime·nanotime();

	// 默认 GOMAXPROCS = 1, 最多不能超过 256
	procs = 1;
	p = runtime·getenv("GOMAXPROCS");
	if(p != nil && (n = runtime·atoi(p)) > 0) {
		if(n > MaxGomaxprocs) {
			n = MaxGomaxprocs;
		}
		procs = n;
	}
	// 为 P 对象申请空间, 不过这是按照最大值申请的(开发者可以在程序运行期间在代码中动态调整).
	// 然后调用 procresize() 创建指定数量的 p, 并为各 p 对象的本地任务队列申请空间.
	runtime·allp = runtime·malloc((MaxGomaxprocs+1)*sizeof(runtime·allp[0]));
	// 调整 P 数量, 为新建的 p 对象分配 mcache(缓存) 与 gfree(任务队列), 
	// 并置为 idle 状态, 之后可以开始干活了.
	procresize(procs);

	// 正式开启GC. 这里表示 runtime 启动完成.
	// 在 runtime·gc() 中会判断此值, 如果不为1, 就结束操作.
	mstats.enablegc = 1;

	if(raceenabled) {
		g->racectx = runtime·raceinit();
	}
}

extern void main·init(void);
extern void main·main(void);

static FuncVal scavenger = {runtime·MHeap_Scavenger};
static FuncVal initDone = { runtime·unlockOSThread };

// 主协程(g0)启动(从这里开始进入开发者声明的 func main())
//
// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> _rt0_go() 程序启动时的初始化汇总函数.
//     在 runtime·schedinit() 后进入此流程, 此时内存分配器与全局调度器已经初始化完成.
//
// The main goroutine.
void runtime·main(void)
{
	Defer d;

	// Max stack size is 1 GB on 64-bit, 250 MB on 32-bit.
	// Using decimal instead of binary GB and MB 
	// because they look nicer in the stack overflow failure message.
	//
	// 64位系统上栈空间最大为1G, 32位上则是250M.
	// 这里用了10进制而不是2进制(1000而不是1024), 这样在出现栈溢出时, 错误消息比较好看.
	if(sizeof(void*) == 8) {
		runtime·maxstacksize = 1000000000;
	}
	else {
		runtime·maxstacksize = 250000000;
	}

	// 创建监控线程(定期垃圾回收, 以及并发任务调试相关)
	// 使用 ps -efT 可以看到, 此时除了当前 m0 主线程, 又多出了一个线程
	newm(sysmon, nil);

	// Lock the main goroutine onto this, the main OS thread, during initialization. 
	// Most programs won't care, 
	// but a few do require certain calls to be made by the main thread.
	// Those can arrange for main.main to run in the main thread
	// by calling runtime.LockOSThread during initialization to preserve the lock.
	//
	// 调用 runtime·lockOSThread(), 在初始化阶段将主协程绑定在当前 m 对象上.
	// 大部分操作可能并无所谓, 但有一些事情必须要主线程来做.
	// 初始化期间持有线程锁, 然后将 main·main 放在主线程中运行...
	runtime·lockOSThread();

	// Defer unlock so that runtime.Goexit during init does the unlock too.
	//
	// initDone 就在上面定义, 值为 runtime·unlockOSThread() 函数
	d.fn = &initDone;
	d.siz = 0;
	d.link = g->defer;
	d.argp = (void*)-1;
	d.special = true;
	d.free = false;
	g->defer = &d;

	if(m != &runtime·m0) {
		runtime·throw("runtime·main not on m0");
	}
	// 通过 scavenger 变量启动独立的协程运行 runtime·MHeap_Scavenger(), 进行垃圾回收.
	//
	// 注意, 这里是创建协程, 而不是系统线程, 所以 ps -efT 是不会有变化的.
	runtime·newproc1(&scavenger, nil, 0, 0, runtime·main);

	// 这里应该是用户编写的代码中, 出现包引用以及 init() 函数的地方, 由编译器指向.
	main·init();

	if(g->defer != &d || d.fn != &initDone) {
		runtime·throw("runtime: bad defer entry after init");
	}

	g->defer = d.link;
	runtime·unlockOSThread();
	// 这里是用户编写的main()函数, 由编译器指向.
	main·main();
	if(raceenabled) {
		runtime·racefini();
	} 

	// Make racy client program work: if panicking on
	// another goroutine at the same time as main returns,
	// let the other goroutine finish printing the panic trace.
	// Once it does, it will exit. See issue 3934.
	if(runtime·panicking) {
		runtime·park(nil, nil, "panicwait");
	} 

	// runtime·exit() 为汇编代码, 针对不同系统平台各自编写.
	runtime·exit(0);

	// ...这里是清理runtime·main吧? 可是都exit了, 这里为什么还能运行???
	for(;;) {
		*(int32*)runtime·main = 0;
	} 
}

////////////////////////////////////////////////////////////////////////////////
// 拆分 m 相关操作函数到 proc_m.c
////////////////////////////////////////////////////////////////////////////////

// 将处于 syscall 或是 locked 状态的 m 对象绑定的 p 移交给新的 m 对象.
// 如果处于gc过程中, 就不再做这些移交的操作, 置 p 为 Pcstop, 然后尝试唤醒STW.
//
// caller: 
// 	1. ·entersyscallblock() 当 GMP 陷入阻塞的系统调用时, GM一起等待返回,
// 	而P则与M解绑, 寻找其他的 m 对象继续发挥作用.
// 	2. retake() 由 sysmon() 发起, 回收陷入系统调用的 p 对象, 同时也可能抢占运行时间比较长的 g 任务.
// 	3. stoplockedm() gc时, m 被告知需要与当前绑定的 p 解绑.
//
// Hands off P from syscall or locked M.
void handoffp(P *p)
{
	// if it has local work, start it straight away
	//
	// 1. 如果当前 m 的 p 对象的本地任务队列不为空, 则需要为其重新绑定一个 m, 继续ta的任务.
	// 且是非 spinning 状态, 以开始运行.
	if(p->runqhead != p->runqtail || runtime·sched.runqsize) {
		startm(p, false);
		return;
	}
	// no local work, check that there are no spinning/idle M's,
	// otherwise our help is not required
	//
	// 2. 运行到这, 说明 p 的本地队列中没有任务.
	// 先找找有没有处于 spinning 状态的 m, 或是存在其他的空闲 p.
	// 如果有, 那也没有必要再做之后的操作了, 不然早被其他 m 抢到了, 用不着自己让.
	// startm() 会获取一个可用的m来与p绑定的.
	// 不过 spinning 为true? 是有什么目的吗???
	//
	// TODO: fast atomic
	if(runtime·atomicload(&runtime·sched.nmspinning) + runtime·atomicload(&runtime·sched.npidle) == 0 && 
		runtime·cas(&runtime·sched.nmspinning, 0, 1)) {

		startm(p, true);
		return;
	}

	runtime·lock(&runtime·sched);
	// 如果当前正处于 gc 中, 应当一步一步将p停止.
	// 要知道, 在 runtime·stoptheworld() 中, 将处于
	// 空闲的, 或是陷入系统调用的p对象的状态修改为了 Pgcstop.
	// stopwait 的值最初为 gomaxprocs, 每将一个p置为 Pgcstop, 这个值减1.
	// 最后剩下处于 running 状态的p没办法强制停止, 只能在 stopnote 上休眠.
	// 等被 sysmon 监控进程协助调度, p的g被回收, 与m解绑, 就运行到了这里.
	// 当 stopwait 值为0时表示所有p都已经停止了, 就可以唤醒休眠的 stoptheworld 了.
	if(runtime·sched.gcwaiting) {
		p->status = Pgcstop;
		if(--runtime·sched.stopwait == 0) {
			runtime·notewakeup(&runtime·sched.stopnote);
		}
		runtime·unlock(&runtime·sched);
		return;
	}
    // 运行到这, 说明p的本地队列中没有任务, 调度器里又没有其他空闲的p
    // 但是全局队列中仍有任务.
    // ...还是调用 startm()
	if(runtime·sched.runqsize) {
		runtime·unlock(&runtime·sched);
		startm(p, false);
		return;
	}
	// If this is the last running P and nobody is polling network,
	// need to wakeup another M to poll network.
    // 如果只有当前这一个p处于 running 状态
    // ...不过, 是从哪里看到 polling network 相关的东西的???
	if(runtime·sched.npidle == runtime·gomaxprocs-1 && 
		runtime·atomicload64(&runtime·sched.lastpoll) != 0) {
		runtime·unlock(&runtime·sched);
		startm(p, false);
		return;
	}
    // 运行到这里, 说明未处于gc, 也没能绑定一个m,
    // 本地队列和全局队列都没有额外任务需要运行.
    // 将p放到全局调度器的空闲链表中.
	pidleput(p);
	runtime·unlock(&runtime·sched);
}

// 尝试再添加一个 p 对象来执行 g 任务.
//
// caller:
// 	1. runtime·newproc1() go func() 新建 g 任务时被调用.
// 	2. runtime·ready() 在原本阻塞的 g 任务被唤醒时(如 channel 有数据, socket 可读写等情况)被调用
// 	3. resetspinning()
//
// Tries to add one more P to execute G's.
// Called when a G is made runnable (newproc, ready).
void wakep(void)
{
	// 对于spin线程要保守一些.
	// 如果存在 spin 状态的 m, 直接返回; 否则就创建一个新的 m.
	//
	// be conservative about spinning threads
	if(!runtime·cas(&runtime·sched.nmspinning, 0, 1)) {
		return;
	}
	// 这里不指定 p, 获取/新建 p 对象会在 startm() 中完成
	startm(nil, true);
}

// 本来像 channel 发生阻塞这种情况, 使 g 任务暂停, 与其关联的 m 只要与该 g 解绑,
// 就可以寻找其他 g 任务继续工作, 而不必让出关联的 p 对象.
// 但如果 m 与 g 绑定(如开发者显式声明osthread()), 则该 m 必须要原地待该 g 解除阻塞.
// 而 p 对象是很宝贵的, 因为需要调用该方法, 将 p 对象让出来, 让其他 m 执行.
//
// caller:
// 	1. park0()
// 	2. runtime·gosched0()
// 	3. exitsyscall0()
//
// Stops execution of the current m that is locked to a g 
// until the g is runnable again.
// Returns with acquired P.
void stoplockedm(void)
{
	P *p;

	if(m->lockedg == nil || m->lockedg->lockedm != m) {
		runtime·throw("stoplockedm: inconsistent locking");
	}
	if(m->p) {
		// Schedule another M to run this p.
		// 将当前m与其p对象解绑, 然后绑定另外一个m
		p = releasep();
		handoffp(p);
	}
	incidlelocked(1);
	// 此时已经让出了 p, 原地休眠(系统线程级别的真正休眠)
	// Wait until another thread schedules lockedg again.
	runtime·notesleep(&m->park);
	runtime·noteclear(&m->park);
	if(m->lockedg->status != Grunnable) {
		runtime·throw("stoplockedm: not runnable");
	}
	// 为当前 m 绑定下一个p(只是修改了一下m->p的值)...但是就没有下下一个p了.
	// 注意能运行到这里, 说明 m->p 已经是 nil 了
	acquirep(m->nextp);
	m->nextp = nil;
}

// 是在发现 gp->lockedm 时.
// gp已经与某个m绑定, 且不是当前m, 但缺少p所以没办法执行.
// 所以调度者指示当前m让出p给ta们, 让ta们先执行.
//
// caller: 
// 	1. schedule() 只有这一处.
// 	m 在空闲期间在 schedule() 中寻找可执行的 g 任务.
//  当找到一个 g, 却发现这个 g 绑定了另外一个 m, 那就调用此函数,
//  当前 m 会把自己的 p 让出来, 自己去休眠.
//
// Schedules the locked m to run the locked gp.
static void startlockedm(G *gp)
{
	M *mp;
	P *p;

	mp = gp->lockedm;
	// mp 不是当前 m, 否则就没有让不让这一说了.
	if(mp == m) {
		runtime·throw("startlockedm: locked to me");
	}
	if(mp->nextp) {
		runtime·throw("startlockedm: m has p");
	}

	// directly handoff current P to the locked m
	incidlelocked(-1);

	// 将自己的 p 释放, 转交给 mp.
	p = releasep();
	mp->nextp = p;

	// 由于 mp 原来是缺少 p 而无法执行的, 所以一定处于休眠状态.
	// 现在有了 p, 可以唤醒ta了.
	runtime·notewakeup(&mp->park);

	// mp 已经可以被调度了, stopm 操作的是当前的m
	// m 会因为失去了自己的 p 而无法执行, 只能阻塞等待.
	stopm();
}

// STW期间, 停止当前m, 直到starttheworld时再返回.
//
// caller: 
// 	1. findrunnable()
// 	2. schedule()
//
// Stops the current m for stoptheworld.
// Returns when the world is restarted.
void gcstopm(void)
{
	P *p;
	// 只在gc操作过程中执行此函数
	if(!runtime·sched.gcwaiting) {
		runtime·throw("gcstopm: not waiting for gc");
	}
	if(m->spinning) {
		// 如果m处于自旋, 则移除其自旋状态, 并将调度器中的自旋记录减1.
		m->spinning = false;
		runtime·xadd(&runtime·sched.nmspinning, -1);
	}
	// 解除与当前m绑定的p对象并将其返回, 此时p的状态为idle
	p = releasep();

	runtime·lock(&runtime·sched);
	p->status = Pgcstop;
	// stopwait减1, 此时如果其值等于0, 就可以唤醒在stopnote休眠的函数了.
	// 比如runtime·stoptheworld()
	if(--runtime·sched.stopwait == 0) {
		runtime·notewakeup(&runtime·sched.stopnote);
	}
	runtime·unlock(&runtime·sched);

	stopm();
}

// 当前 m 上执行指定的 g 任务(g 对象的 status 必须为 Grunnable).
//
// 无返回值.
//
// caller:
// 	1. schedule()
//
// Schedules gp to run on the current M.
// Never returns.
void execute(G *gp)
{
	int32 hz;

	if(gp->status != Grunnable) {
		runtime·printf("execute: bad g status %d\n", gp->status);
		runtime·throw("execute: bad g status");
	}
	gp->status = Grunning;
	gp->preempt = false;
	gp->stackguard0 = gp->stackguard;

	// g 与 m 相互绑定
	m->p->schedtick++;
	m->curg = gp;
	gp->m = m;

	// Check whether the profiler needs to be turned on or off.
	hz = runtime·sched.profilehz;
	if(m->profilehz != hz) {
		runtime·resetcpuprofiler(hz);
	}

	// 这是一段汇编代码
	runtime·gogo(&gp->sched);
}

// caller:
// 	1. schedule() 只有这一处
static void resetspinning(void)
{
	int32 nmspinning;

	if(m->spinning) {
		// 解除当前m的spin状态
		m->spinning = false;
		nmspinning = runtime·xadd(&runtime·sched.nmspinning, -1);

		if(nmspinning < 0) {
			runtime·throw("findrunnable: negative nmspinning");
		}
	} else {
		nmspinning = runtime·atomicload(&runtime·sched.nmspinning);
	}

	// m对象的wakeup策略是有意保守一些的,
	// 所以这里需要确认是否有必要唤醒另一个p.
	// ...啥意思???
	//
	// M wakeup policy is deliberately somewhat conservative 
	// (see nmspinning handling),
	// so see if we need to wakeup another P here.
	if (nmspinning == 0 && runtime·atomicload(&runtime·sched.npidle) > 0) {
		wakep();
	}
}

// 找到一个待执行的 g 并运行 ta, 即 goroutine 的抢占.
//
// 该函数的执行主体为各 M 对象.
//
// caller: 
// 	1. runtime·mstart() 新建的 m 对象完成并启动后, 调用此函数参与调度循环.
// 	2. park0() 好像只与 channel 有关
// 	3. runtime·gosched0() 开发者在代码中, 或是gc过程中在start the world后, 通知 m0 主动进行重新调度;
// 	4. goexit0() g 对象(如go func()) 执行结束时, 需要为此 m 对象寻找新的 g 任务
// 	5. exitsyscall0()
//
// One round of scheduler: find a runnable goroutine and execute it.
// Never returns.
void schedule(void)
{
	G *gp;
	uint32 tick;

	if(m->locks) {
		runtime·throw("schedule: holding locks");
	}

top:
	// 如果当前 m 处于 gc 等待过程中, 就不要继续向下执行了.
	//
	// stoptheworld/freezetheworld 调用时会将此值设置为1
	if(runtime·sched.gcwaiting) {
		gcstopm();
		goto top;
	}

	gp = nil;
	// Check the global runnable queue once in a while to ensure fairness.
	// Otherwise two goroutines can completely occupy the local runqueue
	// by constantly respawning each other.
	//
	// 定期检测全局待运行队列以保证公平性.
	// 否则两个协程可以不断在本地待运行队列互相重新创建, 这样就没有机会运行其他队列中的任务了.
	tick = m->p->schedtick;
	// This is a fancy way to say tick%61==0,
	// it uses 2 MUL instructions instead of a single DIV 
	// and so is faster on modern processors.
	//
	// 这是一个实现了与 tick%61 == 0 相同功能的, 比较fancy(花哨)的方式.
	// ta使用了2个乘法指令, 代替了1个除法指令, 在现代CPU中, 速度更快.
	if(tick - (((uint64)tick*0x4325c53fu)>>36)*61 == 0 && runtime·sched.runqsize > 0) {
		runtime·lock(&runtime·sched);
		// 1. 从全局待运行任务队列中获取1个任务 g 对象, 并将其放到 m->p 的本地待执行队列中.
		gp = globrunqget(m->p, 1);
		runtime·unlock(&runtime·sched);

		if(gp) {
			resetspinning();
		}
	}

	// 2. 如果未到从全局队列取 g 的时机, 或是没能从全局队列获取到,
	// 那么就尝试从当前 m->p 的本地待执行队列中获取.
	if(gp == nil) {
		gp = runqget(m->p);
		if(gp && m->spinning) {
			runtime·throw("schedule: spinning with local work");
		}
	}
	// 3. 如果全局队列和本地队列都没有取到任务g, 那么调用findrunnable()阻塞直到有新任务出现.
	// ...其实是从其他队列中抢夺(偷取)了
	if(gp == nil) {
		// blocks until work is available
		gp = findrunnable(); 
		resetspinning();
	}
	// 4. 如果发现这个g被锁定在某个m上, 则将自己的p转交给那个锁定的m, 让ta来执行gp任务.
	// 而当前的m本身则需要阻塞以等待另一个p对象
	// ...真...舍己为人啊
	// 而且, gp 是从某个队列获取到的, 说明当前并没有 p 可以让ta执行.
	if(gp->lockedm) {
		// Hands off own p to the locked m,
		// then blocks waiting for a new p.
		// startlockedm 中调用了 stopm, 会阻塞当前m, 等待新任务
		startlockedm(gp);
		goto top;
	}

	// 自定义 runtime 信息打印
	if(runtime·debug.scheddetail >= 3) {
		int64 curg_id = -1;
		if(m->curg != nil) {
			curg_id = m->curg->goid;
		}
		runtime·printf(
			"schedule() called - global g: %D, m: %d, m.g0: %D, m.curg: %D, m.p: %d, target g: %D\n",
			g->goid, m->id, m->g0->goid, curg_id, m->p->id, gp->goid
		);
	}

	// 在当前 m 上运行 g 任务, 此时 g 的状态必须为 runnable
	execute(gp);
}

// 将当前 m 的 g 对象状态修改为 waiting, 并解绑 m 和 g, 让该 g 开始休眠(阻塞), 
// 直到由 runtime·ready() 唤醒(唤醒后也是放到全局队列里, 哪个 m 抢到就哪个 m 执行).
// 用于 channel 的实现.
//
// 与 runtime·gosched() 很像, 不过后者是将 g 放回到全局队列, 让其他 m 有机会执行ta.
//
// 	param unlockf: 是一个函数, 用于解锁. 一般为 runtime·unlock() 或是 nil.
// 	param lock: unlockf 的参数, 表示当前协程是在哪个锁对象上休眠的, 以便之后再在该对象上将其唤醒.
//
// 比如在 chan.c -> runtime·chansend() 中, 参数列表为 (runtime·unlock, chan 对象, "chan send")
//
// caller:
// 	1. src/pkg/runtime/chan.c -> 很多
// 	2. src/pkg/runtime/mgc0.c -> runfinq()
//
// Puts the current goroutine into a waiting state and unlocks the lock.
// The goroutine can be made runnable again by calling runtime·ready(gp).
void runtime·park(void(*unlockf)(Lock*), Lock *lock, int8 *reason)
{
	m->waitlock = lock;
	m->waitunlockf = unlockf;
	g->waitreason = reason;
	runtime·mcall(park0);
}

// 在 g0 上继续执行 park 操作, gp 是上面提到的当前 g 对象.
//
// 	@param gp: 待休眠的 g 对象
//
// caller:
// 	1. runtime·park() 只有这一处
//
// runtime·park continuation on g0.
static void park0(G *gp)
{
	// 自定义 runtime 信息打印
	if(runtime·debug.scheddetail >= 3) {
		int64 curg_id = -1;
		if(m->curg != nil) {
			curg_id = m->curg->goid;
		}
		runtime·printf(
			"park0() called - global g: %D, m: %d, m.g0: %D, m.curg: %D, m.p: %d, source g: %D\n",
			g->goid, m->id, m->g0->goid, curg_id, m->p->id, gp->goid
		);
	}

	gp->status = Gwaiting;
	gp->m = nil;
	// 这里的 m 应该是主调函数 runtime·park() 中所在的 m
	m->curg = nil;
	if(m->waitunlockf) {
		// 这里解开了锁, 目的是什么, 为什么要放在这里 ???
		m->waitunlockf(m->waitlock);
		m->waitunlockf = nil;
		m->waitlock = nil;
	}
	// 如果 m 已经与该 gp 绑定, 则无法去执行其他 g 任务, 只能原地等待被唤醒.
	if(m->lockedg) {
		stoplockedm();
		execute(gp); // Never returns.
	}
	schedule();
}

// 告知 m 主动中断当前 g 任务, g 被放回到全局队列, m 则执行其他任务.
//
// 与 runtime·park() 很像, 不过后者的 g 会一直阻塞, 而不是被放到全局队列, 直到被唤醒.
//
// caller:
// 	1. runtime·Gosched 开发者可以在代码中主动触发
// 	2. src/pkg/runtime/mgc0.c -> runtime·gc() gc过程中, start the world 时可主动触发
//
// Scheduler yield.
void runtime·gosched(void)
{
	runtime·mcall(runtime·gosched0);
}

// 告知 m 主动中断当前 g 任务, g 被放回到全局队列, m 则执行其他任务.
//
// caller:
// 	1. runtime·gosched()
//
// runtime·gosched continuation on g0.
void runtime·gosched0(G *gp)
{
	// 自定义 runtime 信息打印
	if(runtime·debug.scheddetail >= 3) {
		int64 curg_id = -1;
		if(m->curg != nil) {
			curg_id = m->curg->goid;
		}
		runtime·printf(
			"runtime·gosched0() called - global g: %D, m: %d, m.g0: %D, m.curg: %D, m.p: %d, source g: %D\n",
			g->goid, m->id, m->g0->goid, curg_id, m->p->id, gp->goid
		);
	}

	// gp->status 本来应该是 Grunning, 毕竟是处于运行状态中主动中断的.
	gp->status = Grunnable;

	// 解绑
	gp->m = nil;
	m->curg = nil;

	runtime·lock(&runtime·sched);
	// 将 gp 放到全局 runq 队列中...
	globrunqput(gp);
	runtime·unlock(&runtime·sched);

	if(m->lockedg) {
		stoplockedm();
		execute(gp);  // Never returns.
	}

	// m 继续订阅执行其他任务
	schedule();
}

////////////////////////////////////////////////////////////////////////////////
// 拆分 g 相关操作函数到 proc_g.c
////////////////////////////////////////////////////////////////////////////////

// golang原生: runtime.Gosched() 函数.
// 
// 开发者可以在代码中主动触发, 将 g 对象放回全局队列, m 执行其他任务.
//
// 啥时候会用到???
void runtime·Gosched(void)
{
	runtime·gosched();
}

// caller:
// 	1. src/pkg/runtime/runtime1.goc -> GOMAXPROCS()
//
// Implementation of runtime.GOMAXPROCS.
// delete when scheduler is even stronger
// 调整的过程中用到了 STW 操作.
int32 runtime·gomaxprocsfunc(int32 n)
{
	int32 ret;

	if(n > MaxGomaxprocs) {
		n = MaxGomaxprocs;
	} 
	runtime·lock(&runtime·sched);
	ret = runtime·gomaxprocs;
	if(n <= 0 || n == ret) {
		// 不合法的 proc 值或是和原有值相同时直接返回.
		runtime·unlock(&runtime·sched);
		return ret;
	}
	runtime·unlock(&runtime·sched);
	// 运行到这里, 准备调整进程中 proc 的数量, 需要执行STW操作.
	runtime·semacquire(&runtime·worldsema, false);
	// ...STW就STW吧, 还把 gcing 标记设置为1.
	// 应该是某些对象需要在 gc 状态进行一些调整吧.
	m->gcing = 1;
	runtime·stoptheworld();
	// newprocs 是 runtime 空间中的全局变量.
	newprocs = n;
	m->gcing = 0;
	runtime·semrelease(&runtime·worldsema);
	runtime·starttheworld();

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
// 拆分 lockOSThread() 等函数到 proc_osthread.c
////////////////////////////////////////////////////////////////////////////////

int32 runtime·mcount(void)
{
	return runtime·sched.mcount;
}

// ...很简单, 容易理解.
void incidlelocked(int32 v)
{
	runtime·lock(&runtime·sched);
	runtime·sched.nmidlelocked += v;
	if(v > 0) {
		checkdead();
	}
	runtime·unlock(&runtime·sched);
}

// 检测死锁. 注意最后一句的报错信息, 写代码的时候经常看到, 
// 所以 run 的值很大可能是0.
// 需要了解ta的检测机制.
// 1. 某些m状态同时处于 idle, locked 两种状态
// 2. 更普遍的情况是, 
//
// caller:
// 	1. incidlelocked()
// 	2. mput()
//
// Check for deadlock situation.
// The check is based on number of running M's, if 0 -> deadlock.
// incidlelocked(-1) 和 incidlelocked(1) 
void checkdead(void)
{
	G *gp;
	int32 run, grunning, s;

	// run 是有任务正在执行的数量, 有一个m用于执行 sysmon, 需要额外减去.
	//
	// -1 for sysmon
	run = runtime·sched.mcount - runtime·sched.nmidle - runtime·sched.nmidlelocked - 1;
	// 注释掉
	// runtime·printf("checkdead(): mcount=%d nmidle=%d nmidlelocked=%d\n",
	// runtime·sched.mcount, runtime·sched.nmidle, runtime·sched.nmidlelocked);
	if(run > 0) {
		return;
	} 
	// ...这是存在m同时处于 idle 和 locked 两种状态吧, 否则不可能小于0
	if(run < 0) {
		runtime·printf(
			"checkdead: nmidle=%d nmidlelocked=%d mcount=%d\n",
			runtime·sched.nmidle, runtime·sched.nmidlelocked, runtime·sched.mcount
		);
		runtime·throw("checkdead: inconsistent counts");
	}
	// ...运行到这里说明 run 是0了吧
	// grunning 表示...处于 waiting 状态的m的数量? 
	// ...那为啥叫running
	grunning = 0;
	for(gp = runtime·allg; gp; gp = gp->alllink) {
		if(gp->isbackground) {
			continue;
		}
		// 除了下面这些, 还有 Gidle, Gdead
		s = gp->status;
		if(s == Gwaiting) {
			grunning++;
		}
		else if(s == Grunnable || s == Grunning || s == Gsyscall) {
			// 因为 run == 0, 所以目前所有的g任务中不应该存在 
			// 这些状态(因为没有m在运行).
			runtime·printf("checkdead: find g %D in status %d\n", gp->goid, s);
			runtime·throw("checkdead: runnable g");
		}
	}
	// possible if main goroutine calls runtime·Goexit()
	// 如果主协程调用了 runtime·Goexit() 可能出现为0的情况.
	if(grunning == 0) {
		runtime·exit(0);
	}
	// 记得这个报错一般出现在锁没有被释放, 或是通道一直被阻塞的时候.
	// 所以 run 的值很大可能是0.
	//
	// do not dump full stacks
	m->throwing = -1;
	// [anchor] 运行到此处的示例, 请见 010.channel 中的 chan02() 函数.
	runtime·throw("all goroutines are asleep - deadlock!");
}

////////////////////////////////////////////////////////////////////////////////
// 拆分 p 对象操作函数到 proc_p.c
////////////////////////////////////////////////////////////////////////////////

static int8 experiment[] = GOEXPERIMENT; // defined in zaexperiment.h

static bool haveexperiment(int8 *name)
{
	int32 i, j;

	for(i=0; i<sizeof(experiment); i++) {
		if((i == 0 || experiment[i-1] == ',') && experiment[i] == name[0]) {
			for(j=0; name[j]; j++) {
				if(experiment[i+j] != name[j]) {
					goto nomatch;
				}
			}

			if(experiment[i+j] != '\0' && experiment[i+j] != ',') {
				goto nomatch;
			}
			return 1;
		}
	nomatch:;
	}
	return 0;
}
