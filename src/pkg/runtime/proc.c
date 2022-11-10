// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "arch_amd64.h"
#include "zaexperiment.h"
#include "malloc.h"
#include "stack.h"
#include "race.h"
#include "type.h"
#include "../../cmd/ld/textflag.h"

// Goroutine scheduler
// The scheduler's job is to distribute ready-to-run goroutines over worker threads.
//
// The main concepts are:
// G - goroutine.
// M - worker thread, or machine.
// P - processor, a resource that is required to execute Go code.
//     M must have an associated P to execute Go code, however it can be
//     blocked or in a syscall w/o an associated P.
//
// Design doc at http://golang.org/s/go11sched.

typedef struct Sched Sched;
struct Sched {
	Lock;

	// 全局最新的g对象的gid, 每创建一个g, 此值加1.
	uint64	goidgen; 

	M*	midle;	 // idle m's waiting for work
	int32	nmidle;	 // number of idle m's waiting for work
	// number of locked m's waiting for work
	// 只有在 incidlelocked(v) 函数中才会在对这个字段进行加减操作
	// m 有 idle 的状态, 也有 locked 的状态, 不冲突, 可共存
	int32	nmidlelocked; 
	int32	mcount;	 // number of m's that have been created
	// maximum number of m's allowed (or die)
	// 可创建的m的最大数量, 在 runtime·schedinit()中声明, 值为1w
	int32	maxmcount;

	P*	pidle;  // idle P's
	// 空闲 p 的数量
	uint32	npidle;
	// 处于自旋状态(m.spinning == true)的 m 的数量
	uint32	nmspinning;

	// 全局任务队列
	//
	// Global runnable queue.
	G*	runqhead;
	G*	runqtail;
	// runq 队列的长度, 表示挂在全局待运行队列的 g 对象的数量
	// 在 procresize() 中, 此字段的初始值为 128.
	int32	runqsize;

	// Global cache of dead G's.
	// gflock 是较小粒度的锁, 用于保护 gfree
	Lock	gflock;
	// 全局空闲g队列的链表头
	G*	gfree;

    // 取值0或1, 在gc操作前后的 runtime·stoptheworld(),
    // 及 runtime·starttheworld()
    // 还有一个 runtime·freezetheworld() 函数会设置这个值.
    // 一般在 stoptheworld/freezetheworld 时会将其设置为1,
    // starttheworld 则会设置为0.
	uint32	gcwaiting;
	// 关于 stopwait 的解释见下面的 stopnote.
	int32	stopwait;
	// 只在 runtime·stoptheworld() 函数中有过 sleep 休眠的操作,
	// 依次停止处于 stop/syscall..等状态的p对象,
	// 但仍然有可能存在占用CPU的p, stoptheworld 在发出抢占请求后,
	// 并不能立刻停止这些p, 只能等到下一次调度再停止.
	// 
	// 在 stoptheworld 中, 将 stopwait 赋值为 runtime·gomaxprocs
	// 表示进程中p对象的数量, 大概在[1-GOMAXPROCS]吧,
	// 每将一个p对象设置为 gcstop, 就将 stopwait 减1, 
	// 会有一个for无限循环, 判断 stopwait 是否等于0
	Note	stopnote;
	uint32	sysmonwait;
	Note	sysmonnote;
	uint64	lastpoll; // 这个值是一个时间值, 纳秒nanotime()

	// cpu profiling rate
	// cpu数据采集的速率...hz是表示赫兹吗...
	int32	profilehz;
};

// 通过GOMAXPROCS可设置的P的最大值, 超过这个值不会报错, 而是降低到这个值.
//
// The max value of GOMAXPROCS.
// There are no fundamental restrictions on the value.
enum { MaxGomaxprocs = 1<<8 };

Sched	runtime·sched;
// 这个值并不是golang限制的最大P数量(见上面的 MaxGomaxprocs)
// 而是golang经过将用户通过 GOMAXPROCS 设置的值与runtime预设的 
// MaxGomaxprocs值, 及物理CPU数量衡量后得到的最大值.
// 但由于golang提供了runtime接口, 所以这个值仍然可以在运行过程中被修改.
// 在 procresize(n) 函数中, 会动态地调整p的数量, 同时修改这个值为n.
int32	runtime·gomaxprocs;
uint32	runtime·needextram;
bool	runtime·iscgo;
M	runtime·m0;
G	runtime·g0;	 // idle goroutine for m0
G*	runtime·allg; // G对象链表
G*	runtime·lastg;
M*	runtime·allm;
M*	runtime·extram; // 这个m链表貌似和 cgo 调用相关.
int8*	runtime·goos;
int32	runtime·ncpu;
// 在 runtime·gomaxprocsfunc(n)中设置为n
// 一般用来全局存储 procresize(n) 中设置的n值.
static int32	newprocs;

void runtime·mstart(void);
static void runqput(P*, G*);
static G* runqget(P*);
static void runqgrow(P*);
static G* runqsteal(P*, P*);
static void mput(M*);
static M* mget(void);
static void mcommoninit(M*);
static void schedule(void);
static void procresize(int32);
static void acquirep(P*);
static P* releasep(void);
static void newm(void(*)(void), P*);
static void stopm(void);
static void startm(P*, bool);
static void handoffp(P*);
static void wakep(void);
static void stoplockedm(void);
static void startlockedm(G*);
static void sysmon(void);
static uint32 retake(int64);
static void incidlelocked(int32);
static void checkdead(void);
static void exitsyscall0(G*);
static void park0(G*);
static void goexit0(G*);
static void gfput(P*, G*);
static G* gfget(P*);
static void gfpurge(P*);
static void globrunqput(G*);
static G* globrunqget(P*, int32);
static P* pidleget(void);
static void pidleput(P*);
static void injectglist(G*);
static bool preemptall(void);
static bool preemptone(P*);
static bool exitsyscallfast(void);
static bool haveexperiment(int8*);

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
void
runtime·schedinit(void)
{
	int32 n, procs;
	byte *p;
	Eface i;

	// 最大系统线程数量限制, 参考标准库 runtime/debug.SetMaxThreads
	runtime·sched.maxmcount = 10000;
	runtime·precisestack = haveexperiment("precisestack");

	runtime·mprofinit();
	runtime·mallocinit(); 	// 初始化内存分配器
	// m 对象为全局对象(应该是所谓的 m0), 在 src/pkg/runtime/runtime.h 中, 
	// 通过"extern	register	G*	g;"声明,
	// 在 src/pkg/runtime/asm_amd64.s -> _rt0_go() 初始化过程被赋值,
	// (在 runtime·osinit 和 runtime·schedinit 之前)
	mcommoninit(m); 		// 初始化当前M, 即 m0

	// Initialize the itable value for newErrorCString,
	// so that the next time it gets called, 
	// possibly in a fault during a garbage collection, 
	// it will not need to allocated memory.
	runtime·newErrorCString(0, &i);

	runtime·goargs(); 			// 处理命令行参数
	runtime·goenvs(); 			// 处理环境变量
	runtime·parsedebugvars(); 	// 处理 GODEBUG, GOTRACEBACK 调试相关的环境变量设置

	// Allocate internal symbol table representation now, 
	// we need it for GC anyway.
	runtime·symtabinit();

	runtime·sched.lastpoll = runtime·nanotime();

	// 默认 GOMAXPROCS = 1, 最多不能超过 256
	procs = 1;
	p = runtime·getenv("GOMAXPROCS");
	if(p != nil && (n = runtime·atoi(p)) > 0) {
		if(n > MaxGomaxprocs) n = MaxGomaxprocs;
		procs = n;
	}
	// 为 P 对象申请空间...不过这是按照最大值申请的啊...
	// 然后调用 procresize() 创建指定数量的 p, 并为各p对象的本地任务队列申请空间.
	runtime·allp = runtime·malloc((MaxGomaxprocs+1)*sizeof(runtime·allp[0]));
	// 调整 P 数量, 为新建的 p 对象分配 mcache(缓存) 与 gfree(任务队列), 
	// 并置为 idle 状态, 之后可以开始干活了.
	procresize(procs);

	// 正式开启GC. 这里表示 runtime 启动完成.
	// 在 runtime·gc() 中会判断此值, 如果不为1, 就结束操作.
	mstats.enablegc = 1;

	if(raceenabled) g->racectx = runtime·raceinit();
}

extern void main·init(void);
extern void main·main(void);

static FuncVal scavenger = {runtime·MHeap_Scavenger};

// 设置initDone为runtime·unlockOSThread()
static FuncVal initDone = { runtime·unlockOSThread };

// 主协程(g0)启动.
// 
// 从这里开始进入开发者声明的 func main()
//
// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> _rt0_go() 程序启动时的初始化汇总函数.
//     在 runtime·schedinit() 后进入此流程, 此时内存分配器与全局调度器已经初始化完成.
//
// The main goroutine.
void
runtime·main(void)
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
	if(raceenabled) runtime·racefini();

	// Make racy client program work: if panicking on
	// another goroutine at the same time as main returns,
	// let the other goroutine finish printing the panic trace.
	// Once it does, it will exit. See issue 3934.
	if(runtime·panicking) runtime·park(nil, nil, "panicwait");

	// runtime·exit() 为汇编代码, 针对不同系统平台各自编写.
	runtime·exit(0);

	// ...这里是清理runtime·main吧? 可是都exit了, 这里为什么还能运行???
	for(;;) *(int32*)runtime·main = 0;
}

void
runtime·goroutineheader(G *gp)
{
	int8 *status;

	switch(gp->status) {
	case Gidle:
		status = "idle";
		break;
	case Grunnable:
		status = "runnable";
		break;
	case Grunning:
		status = "running";
		break;
	case Gsyscall:
		status = "syscall";
		break;
	case Gwaiting:
		if(gp->waitreason)
			status = gp->waitreason;
		else
			status = "waiting";
		break;
	default:
		status = "???";
		break;
	}
	runtime·printf("goroutine %D [%s]:\n", gp->goid, status);
}

void
runtime·tracebackothers(G *me)
{
	G *gp;
	int32 traceback;

	traceback = runtime·gotraceback(nil);
	
	// Show the current goroutine first, if we haven't already.
	if((gp = m->curg) != nil && gp != me) {
		runtime·printf("\n");
		runtime·goroutineheader(gp);
		runtime·traceback(gp->sched.pc, gp->sched.sp, gp->sched.lr, gp);
	}

	for(gp = runtime·allg; gp != nil; gp = gp->alllink) {
		if(gp == me || gp == m->curg || gp->status == Gdead)
			continue;
		if(gp->issystem && traceback < 2)
			continue;
		runtime·printf("\n");
		runtime·goroutineheader(gp);
		if(gp->status == Grunning) {
			runtime·printf("\tgoroutine running on other thread; stack unavailable\n");
			runtime·printcreatedby(gp);
		} else
			runtime·traceback(gp->sched.pc, gp->sched.sp, gp->sched.lr, gp);
	}
}

// 检查 m 总数量(默认10000), 超出会引发进程崩溃.
// caller:
// 	1. mcommoninit()
static void
checkmcount(void)
{
	// sched lock is held
	if(runtime·sched.mcount > runtime·sched.maxmcount) {
		runtime·printf(
			"runtime: program exceeds %d-thread limit\n", 
			runtime·sched.maxmcount
		);
		runtime·throw("thread exhaustion");
	}
}

// caller: 
// 	1. runtime·schedinit() 进程启动, runtime 运行时初始化时被调用.
// 	2. runtime·allocm()
static void
mcommoninit(M *mp)
{
	// If there is no mcache runtime·callers() will crash,
	// and we are most likely in sysmon thread 
	// so the stack is senseless anyway.
	if(m->mcache) {
		runtime·callers(1, mp->createstack, nelem(mp->createstack));
	}

	mp->fastrand = 0x49f6428aUL + mp->id + runtime·cputicks();

	runtime·lock(&runtime·sched);
	mp->id = runtime·sched.mcount++;
	checkmcount();
	runtime·mpreinit(mp);

	// Add to runtime·allm so garbage collector doesn't free m
	// when it is just in a register or thread-local storage.
	mp->alllink = runtime·allm;
	// runtime·NumCgoCall() iterates over allm w/o schedlock,
	// so we need to publish it safely.
	runtime·atomicstorep(&runtime·allm, mp);
	runtime·unlock(&runtime·sched);
}

// 唤醒因 runtime·park() 而陷入休眠的 g 对象.
//
// 将 gp 标记为 Grunnable 状态(gp->status 需要是 Gwaiting 状态),
// 并将 gp 通过 runqput() 放到待执行队列中.
//
// Mark gp ready to run.
void
runtime·ready(G *gp)
{
	// Mark runnable.
	// disable preemption because it can be holding p in a local var
	m->locks++;
	if(gp->status != Gwaiting) {
		runtime·printf("goroutine %D has status %d\n", gp->goid, gp->status);
		runtime·throw("bad g->status in ready");
	}
	gp->status = Grunnable;
	// 将gp对象放到当前m绑定的p对象的runq待执行队列中.
	runqput(m->p, gp);
	// TODO: fast atomic
	if(runtime·atomicload(&runtime·sched.npidle) != 0 && runtime·atomicload(&runtime·sched.nmspinning) == 0){
		wakep();
	}
	m->locks--;
	// restore the preemption request in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) g->stackguard0 = StackPreempt;
}

// 确定并⾏回收的 goroutine 数量 = min(GOMAXPROCS, ncpu, MaxGcproc)
//
// caller: 
// 	1. mgc0.c -> gc() 只有这一处
int32
runtime·gcprocs(void)
{
	int32 n;

	// Figure out how many CPUs to use during GC.
	// Limited by gomaxprocs, number of actual CPUs, and MaxGcproc.
	// 确定可用于GC操作的CPU数量, 三者中取最小值.
	runtime·lock(&runtime·sched);

	n = runtime·gomaxprocs;
	if(n > runtime·ncpu) n = runtime·ncpu;
	if(n > MaxGcproc) n = MaxGcproc;

	// one M is currently running
	// 在被调用时, 只有1个m处于运行状态, 其他都是 midle
	// 如果 n > nmidle+1, 其实是超出了当前创建了的m的总量.
	// 最多是让所有m都去执行gc嘛.
	if(n > runtime·sched.nmidle+1) n = runtime·sched.nmidle+1;
	runtime·unlock(&runtime·sched);
	return n;
}

// 是否需要增加新的 proc 以执行gc
// 标准就是判断当前处于 midle 的m是否大于0
// 因为只要大于0
// 由于只有一处调用, 可以认为被调用时只有一个m在运行,
// 其他都在休眠.
// caller: runtime·starttheworld() 只有这一处
static bool
needaddgcproc(void)
{
	int32 n;

	runtime·lock(&runtime·sched);
	
	n = runtime·gomaxprocs;
	if(n > runtime·ncpu) n = runtime·ncpu;
	if(n > MaxGcproc) n = MaxGcproc;
	
	// one M is currently running
	// 在被调用时, 只有1个m处于运行状态, 其他都是 midle
	// 将这个 m 移除再比较
	n -= runtime·sched.nmidle+1; 
	runtime·unlock(&runtime·sched);
	return n > 0;
}

// 当确定执行gc操作的协程数量大于1时调用此函数.
//
// param nproc: 执行 gc 的协程数量.
//
// caller: 
// 	1. mgc0.c -> gc()
void
runtime·helpgc(int32 nproc)
{
	M *mp;
	int32 n, pos;

	runtime·lock(&runtime·sched);
	pos = 0;
	// one M is currently running
	for(n = 1; n < nproc; n++) { 
		// m 中的 mcache 用的其实是ta绑定的 p 的 mcache
		// 如果这个if条件为true, 说明遍历到了当前m的p对象, 需要跳过.
		if(runtime·allp[pos]->mcache == m->mcache) {
			pos++;
		}

		// 从sched获取一个空闲的m对象, 设置其helpgc序号,
		// 并且与 allp[pos] 绑定.
		mp = mget();
		// 这里报的错是什么意思?
		// 由于 nproc = min(GOMAXPROCS, ncpu, MaxGcproc), 
		// 而m的数量一定是比p多的, 所以???
		if(mp == nil) {
			runtime·throw("runtime·gcprocs inconsistency");
		}
		// 这是设置序号吧?
		mp->helpgc = n;
		// 为什么只绑定 mcache? 不使用 acquirep() 完成?
		mp->mcache = runtime·allp[pos]->mcache;
		pos++;
		runtime·notewakeup(&mp->park);
	}
	runtime·unlock(&runtime·sched);
}

// Similar to stoptheworld but best-effort 
// and can be called several times.
// There is no reverse operation, used during crashing.
// This function must not lock any mutexes.
void
runtime·freezetheworld(void)
{
	int32 i;

	if(runtime·gomaxprocs == 1) {
		return;
	}
	// stopwait and preemption requests can be lost
	// due to races with concurrently executing threads,
	// so try several times
	for(i = 0; i < 5; i++) {
		// this should tell the scheduler to not start any new goroutines
		runtime·sched.stopwait = 0x7fffffff;
		// 这里用CAS原子操作, 而在runtime·starttheworld()中,
		// 由于只有一个协程在运行, 就直接赋值为0了.
		runtime·atomicstore((uint32*)&runtime·sched.gcwaiting, 1);
		// this should stop running goroutines
		// no running goroutines
		if(!preemptall()) {
			break; 
		}
		runtime·usleep(1000);
	}
	// to be sure
	runtime·usleep(1000);
	preemptall();
	runtime·usleep(1000);
}

// caller: proc.c -> runtime·gomaxprocsfunc(), 
//			mgc0.c -> runtime·gc(), 
//			mgc0.c -> runtime·ReadMemStats()
void
runtime·stoptheworld(void)
{
	int32 i;
	uint32 s;
	P *p;
	bool wait;

	runtime·lock(&runtime·sched); // 加锁
	// stopwait 表示当前go程序启动的P的数量
	// 经过 schedinit, 这个值应该就在1-GOMAXPROCS之间
	runtime·sched.stopwait = runtime·gomaxprocs;
	// 这里用CAS原子操作, 而在runtime·starttheworld()中
	// 由于只有一个协程在运行, 就直接赋值为0了.
	runtime·atomicstore((uint32*)&runtime·sched.gcwaiting, 1);
	// 发起抢占请求, 尝试停止所有正在运行的goroutine
	preemptall();

	// stop current P
	// 1. 停止当前的P, 将其状态转换成Pgcstop
	m->p->status = Pgcstop;
	runtime·sched.stopwait--;

	// try to retake all P's in Psyscall status
	// 2. 将陷入系统调用的P也转换成Pgcstop状态
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		s = p->status;
		if(s == Psyscall && runtime·cas(&p->status, s, Pgcstop)) {
			runtime·sched.stopwait--;
		}
	}
	// stop idle P's
	// 3. 将空闲的P转换成Pgcstop
	while(p = pidleget()) {
		p->status = Pgcstop;
		runtime·sched.stopwait--;
	}
	wait = runtime·sched.stopwait > 0;
	runtime·unlock(&runtime·sched); // 解锁

	// wait for remaining P's to stop voluntarily
    // 如果wait为true, 表示仍有其他状态的p对象, 比如正在使用CPU计算.
    // 就算通过 preemptall() 抢占了g对象, 也不是立刻就停止执行,
    // 而是需要等待下一次调度, 使p放弃g, 当发生这样的操作时,
    // 所处的函数会判断 stopwait 的值是否为0, 如果是, 则发起唤醒操作,
    // 这样 stoptheworld 才能继续下去.
	if(wait) {
		for(;;) {
			// wait for 100us, then try to re-preempt in case of any races
			// 每次等待100us, 然后尝试抢占.
			// if条件为true, 说明某个协程调用了 runtime·notewakeup() 而被唤醒
			// 为false则说明100us时间到, 超时结束.
			if(runtime·notetsleep(&runtime·sched.stopnote, 100*1000)) {
				runtime·noteclear(&runtime·sched.stopnote);
				break;
			}
			// 发起抢占请求, 尝试停止所有正在运行的goroutine
			preemptall();
		}
	}
	// stopwait还未清零, STW操作失败
	if(runtime·sched.stopwait) runtime·throw("stoptheworld: not stopped");
	// 或者p链表中还有状态未成为 Pgcstop 的, 也表示失败.
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p->status != Pgcstop) {
			runtime·throw("stoptheworld: not stopped");
		}
	}
}

// caller: runtime·starttheworld() 只有这一处调用.
static void
mhelpgc(void)
{
	m->helpgc = -1;
}

// ##runtime·starttheworld
void
runtime·starttheworld(void)
{
	P *p, *p1;
	M *mp;
	G *gp;
	bool add;

	// disable preemption because it can be holding p in a local var
	m->locks++; 

	// non-blocking
	gp = runtime·netpoll(false); 
	// 将gp放到全局任务队列
	injectglist(gp);
	add = needaddgcproc();
	runtime·lock(&runtime·sched);
	if(newprocs) {
		procresize(newprocs);
		newprocs = 0;
	} else
		procresize(runtime·gomaxprocs);
	runtime·sched.gcwaiting = 0;

	p1 = nil;
	// 遍历全局任务队列, 将拥有本地任务的p队列取出组成链表并分别绑定空闲的m.
	// 注意排列的顺序, while循环中先取出的p对象将会放在这个链表的尾部, 
	// 最后一个拥有本地任务队列的P对象(由p1指向)将作为该链表头, 
	// 而最终p变量将表示第一个没有本地任务的对象, 没有在此链表中.
	while(p = pidleget()) {
		// procresize() puts p's with work at the beginning of the list.
		// Once we reach a p without a run queue, 
		// the rest don't have one either.
		// procresize() 把本地队列有任务的 p 都放在了链表的前面, 没任务的都在后面.
		// 当我们遍历到一个没有本地任务的 p 时, 说明剩下的 p 也都是没有的.
		if(p->runqhead == p->runqtail) {
			pidleput(p);
			break;
		}
		// 如果p对象本地任务队列不为空, 
		// 那么从全局调度器的 midle 链表中取一个m, 并与该p绑定.
		// 注意: mget()有可能返回nil.
		p->m = mget();
		p->link = p1;
		p1 = p;
	}
	// 经过上面的while循环, p1对象表示链表的头, 由link指向下一个p对象, 最后一个为nil.

	if(runtime·sched.sysmonwait) {
		// runtime.h 中有声明, 将true和false定义为1和0
		runtime·sched.sysmonwait = false;
		runtime·notewakeup(&runtime·sched.sysmonnote);
	}
	runtime·unlock(&runtime·sched);

	// 遍历上面的while循环构建的链表, 其中的每个p成员都拥有本地任务队列.
	while(p1) {
		p = p1;
		p1 = p1->link;
		if(p->m) {
			// 虽然该p绑定了某个m, 但好像还不如绑定一个nil呢.
			// 因为绑定的目标m->nextp指向的p可能并不是当前的p对象,
			// 这样就可能造成一种 m <-> p 相互指向不一致的问题, 
			// 此时需要抛出异常.
			mp = p->m;
			// 这里不是解绑了吗? 而且后面好像也没有用到,
			// 所以我觉得上面绑定p和m的目的只是通过 mget() 获取m对象而已,
			// 实际的目的是设置 mp->nextp, 即在该m在完成当前p任务后, 
			// 下一个执行的才是我们现在指定的p.
			p->m = nil;
			if(mp->nextp) runtime·throw("starttheworld: inconsistent mp->nextp");
			mp->nextp = p;
			// 在 stoplockedm(), stopm() 两处被调用 notesleep().
			// 在这里唤醒这个 m.
			// 需要知道的是, 在两处 sleep() 的地方, 被唤醒后都立刻
			// 绑定了 nextp 成员, 放弃了原来的p.
			runtime·notewakeup(&mp->park);
		} else {
			// Start M to run P. Do not start another M below.
			// p对象没有可用的m, 就启动新的m.
			// 注意这个地方可能会执行不只一次...吧???
			// 一旦这里启动了新的m, 就设置add为false, 不再执行下面的if块.
			newm(nil, p);
			add = false;
		}
	}

	if(add) {
		// If GC could have used another helper proc, start one now,
		// in the hope that it will be available next time.
		// 希望能在下一轮gc期间可用.
		// It would have been even better to start it before the collection,
		// but doing so requires allocating memory, 
		// so it's tricky to coordinate. 
		// 可能在开始gc前就启动会好一点, 但这需要事先分配内存, 很难协调.
		// This lazy approach works out in practice:
		// we don't mind if the first couple gc rounds don't have quite
		// the maximum number of procs.
		// 这种惰性的方法在实践中是有用的, 因为我们不在乎前两轮gc执行的procs数量
		// 没有达到最大值.
		newm(mhelpgc, nil);
	}
	m->locks--;
	// restore the preemption request in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) g->stackguard0 = StackPreempt;
}

//
// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> _rt0_go() 
//     程序启动入口, 开始运行主协程.
// 	2. src/pkg/runtime/os_linux.c -> runtime·newosproc()
//     创建新 m 对象时, 底层关联系统线程, 并通过 clone 系统调用执行本函数.
//
// Called to start an M.
void
runtime·mstart(void)
{
#ifdef GOOS_windows
#ifdef GOARCH_386
	// It is used by windows-386 only. 
	// Unfortunately, seh needs to be located on os stack, 
	// and mstart runs on os stack for both m0 and m.
	SEH seh;
#endif
#endif

	if(g != m->g0) runtime·throw("bad runtime·mstart");

	// Record top of stack for use by mcall.
	// Once we call schedule we're never coming back,
	// so other calls can reuse this stack space.
	runtime·gosave(&m->g0->sched);
	// make sure it is never used
	m->g0->sched.pc = (uintptr)-1; 
	// cgo sets only stackguard0, copy it to stackguard
	m->g0->stackguard = m->g0->stackguard0; 

#ifdef GOOS_windows
#ifdef GOARCH_386
	m->seh = &seh;
#endif
#endif

	runtime·asminit();
	runtime·minit();

	// Install signal handlers; after minit so that minit can
	// prepare the thread to be able to handle the signals.
	// 装载信号处理函数
	if(m == &runtime·m0) runtime·initsig();
	// mstart 被 newm 调用时可能会提供 mstartfn 函数
	// 执行完成目标 fn 后, 该 m 会加入到 g 的抢夺队列(schedule).
	if(m->mstartfn) m->mstartfn();

	if(m->helpgc) {
		m->helpgc = 0;
		stopm();
	} else if(m != &runtime·m0) {
		acquirep(m->nextp);
		m->nextp = nil;
	}
	// 调度器核心执行函数
	schedule();

	// TODO(brainman): This point is never reached, 
	// because scheduler does not release os threads at the moment. 
	// But once this path is enabled, we must remove our seh here.
}

// When running with cgo, we call _cgo_thread_start to start threads for us
// so that we can play nicely with foreign code.
void (*_cgo_thread_start)(void*);

typedef struct CgoThreadStart CgoThreadStart;
struct CgoThreadStart
{
	M *m;
	G *g;
	void (*fn)(void);
};

// 创建一个全新的 m 对象并返回(暂不与任何系统线程绑定).
// 期间会检测 m 数量是否超过限制(默认10000), 超出会引发进程崩溃.
//
// caller: 
// 	1. newm() 创建新的 m 对象, 之后会与一个系统线程绑定.
// 	2. runtime·newextram()
//
// Allocate a new m unassociated with any thread.
// Can use p for allocation context if needed.
M*
runtime·allocm(P *p)
{
	M *mp;
	// The Go type M
	static Type *mtype; 

	// disable GC because it can be called from sysmon
	m->locks++; 
	// temporarily borrow p for mallocs in this function
	if(m->p == nil) acquirep(p); 

	if(mtype == nil) {
		Eface e;
		// 这个函数在 mgc0.go 中定义, 作用就是把e变量看作m指针, 并将其内容清空.
		// 相当于 memset 了.
		runtime·gc_m_ptr(&e);
		mtype = ((PtrType*)e.type)->elem;
	}

	mp = runtime·cnew(mtype);
	mcommoninit(mp);

	// In case of cgo, pthread_create will make us a stack.
	// Windows will layout sched stack on OS stack.
	//
	// 该函数用于 cgo 时(runtime·newextram 应该就是专门用于 cgo 调用的)
	// 或是 win 平台下, 不分配栈空间.
	// 如果是在 linux 平台下, 则分配 8kb(即`ulimit -s`的值)
	if(runtime·iscgo || Windows){
		mp->g0 = runtime·malg(-1);
	} else {
		mp->g0 = runtime·malg(8192);
	}

	if(p == m->p) releasep();

	m->locks--;
	// restore the preemption request 
	// in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) g->stackguard0 = StackPreempt;

	return mp;
}

static M* lockextra(bool nilokay);
static void unlockextra(M*);

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
void
runtime·needm(byte x)
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

// newextram allocates an m and puts it on the extra list.
// It is called with a working local m, 
// so that it can do things like call schedlock and allocate.
// caller: runtime·cgocall() runtime·cgocallbackg()
// 而且在函数内部有一次递归调用.
// 应该可以说这就是为了 cgo 调用而存在的吧?
void
runtime·newextram(void)
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
void
runtime·dropm(void)
{
	M *mp, *mnext;

	// Undo whatever initialization minit did during needm.
	runtime·unminit();

#ifdef GOOS_windows
#ifdef GOARCH_386
	m->seh = nil;  // reset dangling typed pointer
#endif
#endif

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
static M*
lockextra(bool nilokay)
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
static void
unlockextra(M *mp)
{
	runtime·atomicstorep(&runtime·extram, mp);
}


// 创建一个 m 对象, 绑定目标 p 对象, 并执行 fn.
//
// caller:
// 	1. runtime·main() runtime 初始化完成后, 启动主协程 g0, 
//     并由 g0 调用此函数创建监控线程, 运⾏调度器监控.
// 	2. startm()
// Create a new m. 
// It will start off with a call to fn, or else the scheduler.
// 
static void
newm(void(*fn)(void), P *p)
{
	M *mp;

	mp = runtime·allocm(p);
	mp->nextp = p;
	mp->mstartfn = fn;

	if(runtime·iscgo) {
		CgoThreadStart ts;

		if(_cgo_thread_start == nil) runtime·throw("_cgo_thread_start missing");
		ts.m = mp;
		ts.g = mp->g0;
		ts.fn = runtime·mstart;
		runtime·asmcgocall(_cgo_thread_start, &ts);
		return;
	}
	// 创建系统线程
	runtime·newosproc(mp, (byte*)mp->g0->stackbase);
}

// 结束当前 m , 直到有新任务需要执行(就是陷入休眠).
// ...没有返回值, 不过会将当前
//
// caller: 
// 	1. runtime·mstart()
// 	2. startlockedm() 当前 m 将自己的 p 对象转让给另一个 m, 然后调用此方法自行休眠.
// 	3. gcstopm()
// 	4. findrunnable() 某个 m 找了半天没找到可执行的 g 任务, 调用此方法放入空闲队列
// 	5. exitsyscall0()
//
// Stops execution of the current m until new work is available.
// Returns with acquired P.
static void
stopm(void)
{
	if(m->locks) runtime·throw("stopm holding locks");
	if(m->p) runtime·throw("stopm holding p");
	if(m->spinning) {
		// 如果当前m处于自旋, 那么结束自旋, 
		// 同时将全局调度器的 nmspinning 字段减1
		m->spinning = false;
		runtime·xadd(&runtime·sched.nmspinning, -1);
	}

retry:
	runtime·lock(&runtime·sched);
	// 将m放到全局调度器的空闲队列 runtime·sched.midle 中
	mput(m); 
	runtime·unlock(&runtime·sched);
	// 开始休眠, 待唤醒.
	runtime·notesleep(&m->park);
	runtime·noteclear(&m->park);
	// 被唤醒的地方有4处, 可能性还是有很多种的.
	// 如果被唤醒后发现 m->helpgc > 0, 
	// 说明此m被选为参与执行gc了.
	if(m->helpgc) {
		runtime·gchelper();
		// 辅助执行完毕, 立刻回归原来的角色.
		m->helpgc = 0;
		m->mcache = nil;
		goto retry;
	}
	// 为当前m绑定下一个p(只是修改了一下m->p的值)...但是就没有下下一个p了.
	// 注意能运行到这里, 说明 m->p 已经是 nil 了
	acquirep(m->nextp);
	m->nextp = nil;
}

// caller: 
// 	1. startm()
static void
mspinning(void)
{
	m->spinning = true;
}

// 调度一些 m 对象以运行目标 p, 如果 m 不足则创建.
// 如果 p == nil, 尝试获取一个空闲的 p 对象,
// 如果获取不到, 则返回 false...这个函数好像没有返回值吧
//
// caller:
// 	1. wakep()
//
// Schedules some M to run the p (creates an M if necessary).
// If p==nil, tries to get an idle P, if no idle P's returns false.
static void
startm(P *p, bool spinning)
{
	M *mp;
	void (*fn)(void);

	runtime·lock(&runtime·sched);
	// m 要工作必须要有一个 p 对象.
	if(p == nil) {
		p = pidleget();
		if(p == nil) {
			// 这里是没获取到空闲状态的 p
			runtime·unlock(&runtime·sched);
			// ...但是这句是啥意思, 明明没有绑定 m 和 p, 而且还是减1操作???
			if(spinning) runtime·xadd(&runtime·sched.nmspinning, -1);
			return;
		}
	}

	mp = mget();
	runtime·unlock(&runtime·sched);

	// 如果没能获取到 m, 则创建新的 m 以运行目标 p
	if(mp == nil) {
		fn = nil;
		// mspinning() 的定义就在上面
		// 只是简单地把 m->spinning 的值赋为 true
		// 不过...m是哪个m? 是怎么确定的?
		if(spinning) fn = mspinning;
		newm(fn, p);
		return;
	}
	// 从全局调度器中获取到了一个 m
	if(mp->spinning) runtime·throw("startm: m is spinning");
	if(mp->nextp) runtime·throw("startm: m has p");
	mp->spinning = spinning;
	mp->nextp = p;
	runtime·notewakeup(&mp->park);
}

// 将处于 syscall 或是 locked 状态的 m 对象绑定的 p 移交给新的 m 对象.
// 如果处于gc过程中, 就不再做这些移交的操作, 置 p 为 Pcstop, 然后尝试唤醒STW.
//
// caller: 
// 	1. stoplockedm()
// 	2. ·entersyscallblock()
// 	3. retake()
//
// Hands off P from syscall or locked M.
static void
handoffp(P *p)
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
// 	1. runtime·newproc1()
//
// Tries to add one more P to execute G's.
// Called when a G is made runnable (newproc, ready).
static void
wakep(void)
{
	// 对于spin线程要保守一些.
	// 如果存在 spin 状态的 m, 直接返回; 否则就创建一个新的 m.
	//
	// be conservative about spinning threads
	if(!runtime·cas(&runtime·sched.nmspinning, 0, 1)) return;
	// ...不是说添加 p 对象吗, 怎么 startm 了
	startm(nil, true);
}

// Stops execution of the current m that is locked to a g 
// until the g is runnable again.
// Returns with acquired P.
// 
static void
stoplockedm(void)
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
	// Wait until another thread schedules lockedg again.
	runtime·notesleep(&m->park);
	runtime·noteclear(&m->park);
	if(m->lockedg->status != Grunnable) {
		runtime·throw("stoplockedm: not runnable");
	}
	// 为当前m绑定下一个p(只是修改了一下m->p的值)...但是就没有下下一个p了.
	// 注意能运行到这里, 说明 m->p 已经是 nil 了
	acquirep(m->nextp);
	m->nextp = nil;
}

// 是在发现 gp->lockedm 时.
// gp已经与某个m绑定, 且不是当前m, 但缺少p所以没办法执行.
// 所以调度者指示当前m让出p给ta们, 让ta们先执行.
//
// caller: 
// 	1. schedule() m 在空闲期间在 schedule() 中寻找可执行的 g 任务.
//     当找到一个 g, 却发现这个 g 绑定了另外一个 m, 那就调用此函数,
//     当前 m 会把自己的 p 让出来, 自己去休眠.
//
// Schedules the locked m to run the locked gp.
static void
startlockedm(G *gp)
{
	M *mp;
	P *p;

	mp = gp->lockedm;
	// mp 不是当前 m, 否则就没有让不让这一说了.
	if(mp == m) runtime·throw("startlockedm: locked to me");
	if(mp->nextp) runtime·throw("startlockedm: m has p");

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

// Stops the current m for stoptheworld.
// Returns when the world is restarted.
// STW期间, 停止当前m, 直到starttheworld时再返回.
// caller: findrunnable(), schedule()
static void
gcstopm(void)
{
	P *p;
	// 只在gc操作过程中执行此函数
	if(!runtime·sched.gcwaiting) runtime·throw("gcstopm: not waiting for gc");
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
static void
execute(G *gp)
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
	if(m->profilehz != hz) runtime·resetcpuprofiler(hz);

	// 这是一段汇编代码
	runtime·gogo(&gp->sched);
}

// 尝试获取一个待执行的 g 任务(分别从本地队列, 全局队列, 以及其他 p 的本地队列里寻找).
//
// caller: 
// 	1. schedule() 只有这一个调用者, m 线程不断寻找待执行任务.
//     但是全局队列, 还和 m->p 中的本地队列, 都没有可执行任务时, 调用此函数从其他 p 对象中获取.
//
// Finds a runnable goroutine to execute.
// Tries to steal from other P's, get g from global queue, poll network.
static G*
findrunnable(void)
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
	if(gp) return gp;

	// 2. 尝试从全局任务队列中获取一"批" g, 默认数量 sched.runqsize/gomaxprocs+1
	// global runq
	if(runtime·sched.runqsize) {
		runtime·lock(&runtime·sched);
		gp = globrunqget(m->p, 0);
		runtime·unlock(&runtime·sched);
		if(gp) return gp;
	}
	// poll network
	gp = runtime·netpoll(false);  // non-blocking
	if(gp) {
		injectglist(gp->schedlink);
		gp->status = Grunnable;
		return gp;
	}
	// If number of spinning M's >= number of busy P's, block.
	// This is necessary to prevent excessive CPU consumption
	// when GOMAXPROCS>>1 but the program parallelism is low.
	if(!m->spinning && 2 * runtime·atomicload(&runtime·sched.nmspinning) >= runtime·gomaxprocs - runtime·atomicload(&runtime·sched.npidle))  // TODO: fast atomic
		goto stop;
	if(!m->spinning) {
		m->spinning = true;
		runtime·xadd(&runtime·sched.nmspinning, 1);
	}
	// 3. 随机从其他 P ⾥获取一个 g 任务
	// random steal from other P's
	for(i = 0; i < 2*runtime·gomaxprocs; i++) {
		if(runtime·sched.gcwaiting)
			goto top;
		p = runtime·allp[runtime·fastrand1()%runtime·gomaxprocs];
		if(p == m->p){
			// 偷到自己了, 返回的 gp 就还是 nil, 继续循环.
			gp = runqget(p);
		} else {
			// 偷⼀半到⾃⼰的 p 队列，并返回⼀个
			gp = runqsteal(m->p, p);
		}
		if(gp) return gp;
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
		if(m->p)
			runtime·throw("findrunnable: netpoll with p");
		if(m->spinning)
			runtime·throw("findrunnable: netpoll with spinning");
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

static void
resetspinning(void)
{
	int32 nmspinning;

	if(m->spinning) {
		// 解除当前m的spin状态
		m->spinning = false;
		nmspinning = runtime·xadd(&runtime·sched.nmspinning, -1);

		if(nmspinning < 0) runtime·throw("findrunnable: negative nmspinning");
	} else
		nmspinning = runtime·atomicload(&runtime·sched.nmspinning);

	// M wakeup policy is deliberately somewhat conservative 
	// (see nmspinning handling),
	// so see if we need to wakeup another P here.
	// m对象的wakeup策略是有意保守一些的,
	// 所以这里需要确认是否有必要唤醒另一个p.
	// ...啥意思???
	if (nmspinning == 0 && runtime·atomicload(&runtime·sched.npidle) > 0)
		wakep();
}

// Injects the list of runnable G's into the scheduler.
// Can run concurrently with GC.
// 将 glist 任务列表放入全局调度器的任务队列, 将其全部标记为 runnable,
// 同时如果全局调度器中还有处于 idle 状态的p, 就创建更多的m来运行这些任务吧.
// 最后部分, 如果存在空闲的p, 就多创建一些m来运行这些任务.
// ...可以与GC并行运行??? 怎么可能???
static void
injectglist(G *glist)
{
	int32 n;
	G *gp;

	if(glist == nil) return;

	runtime·lock(&runtime·sched);
	for(n = 0; glist; n++) {
		gp = glist;
		glist = gp->schedlink;
		gp->status = Grunnable;
		globrunqput(gp);
	}
	runtime·unlock(&runtime·sched);
	// 如果全局队列中还有处于 idle 状态的p, 就创建更多的m来运行这些任务.
	for(; n && runtime·sched.npidle; n--) {
		startm(nil, false);
	}
}

// 找到一个待执行的 g 并运行ta
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
static void
schedule(void)
{
	G *gp;
	uint32 tick;

	if(m->locks) runtime·throw("schedule: holding locks");

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

		if(gp) resetspinning();
	}

	// 2. 如果未到从全局队列取 g 的时机, 或是没能从全局队列获取到,
	// 那么就尝试从当前 m->p 的本地待执行队列中获取.
	if(gp == nil) {
		gp = runqget(m->p);
		if(gp && m->spinning) runtime·throw("schedule: spinning with local work");
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
void
runtime·park(void(*unlockf)(Lock*), Lock *lock, int8 *reason)
{
	m->waitlock = lock;
	m->waitunlockf = unlockf;
	g->waitreason = reason;
	runtime·mcall(park0);
}

// 在 g0 上继续执行 park 操作, gp 是上面提到的当前 g 对象.
//
// caller:
// 	1. runtime·park() 只有这一处
//
// runtime·park continuation on g0.
static void
park0(G *gp)
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
	m->curg = nil;
	// 这是让该m仍然能正常工作吧, 不受g对象的影响.
	if(m->waitunlockf) {
		m->waitunlockf(m->waitlock);
		m->waitunlockf = nil;
		m->waitlock = nil;
	}
	// 休眠了 gp 对象, 但 m 不能闲着, 重新开始调度.
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
void
runtime·gosched(void)
{
	runtime·mcall(runtime·gosched0);
}

// 告知 m 主动中断当前 g 任务, g 被放回到全局队列, m 则执行其他任务.
//
// caller:
// 	1. runtime·gosched()
//
// runtime·gosched continuation on g0.
void
runtime·gosched0(G *gp)
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
void
runtime·goexit(void)
{
	if(raceenabled) runtime·racegoend();
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
static void
goexit0(G *gp)
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

// 设置当前 g 对象的 g->sched 信息, 即当前正在执行的函数的主调函数 pc 和 sp 等信息.
//
// caller: 
// 	1. ·entersyscall() m 在执行 g 任务时, 陷入系统调用前会发所察觉, 调用此函数保存现场信息,
//     之后 m 会切换执行其他 g 任务(切换的是 p, 还是同一个 p 的其他本地任务).
// 	2. ·entersyscallblock()
//
// 主要还是用于从系统调用返回时重新唤醒主调函数的吧, sp 用于定位局部变量
// 注意: 这里的sched存放的主调函数信息其实是进行syscall的函数, 用于返回的.
#pragma textflag NOSPLIT
static void
save(void *pc, uintptr sp)
{
	g->sched.pc = (uintptr)pc;
	g->sched.sp = sp; 
	g->sched.lr = 0;
	g->sched.ret = 0;
	g->sched.ctxt = 0;
	g->sched.g = g;
}

// The goroutine g is about to enter a system call.
// Record that it's not using the cpu anymore.
// This is called only from the go syscall library and cgocall,
// not from the low-level system calls used by the runtime.
//
// 协程对象 g 将要陷入系统调用, 这里记录ta将不再占用cpu.
// 此函数只由 golang 的 syscall 库和 cgocall 函数调用.
// 
// Entersyscall cannot split the stack: the runtime·gosave must
// make g->sched refer to the caller's stack segment, 
// because entersyscall is going to return immediately after.
//
// entersyscall 不能被分割栈, runtime·gosave() 必定会将
// g->sched 赋值为调用方的栈段, 因为 entersyscall 将会马上返回.
//
// param dummy: 应该是主调函数的函数名称所在地址, 这是一个不确定的值.
// 主要就设置了一个 p 的状态为 Psyscall, 解绑 m与p, 然后标记为当前 g 对象被抢占, 好像就没了?
//
// caller: 
// 	1. runtime·cgocall()
// 	2. runtime·cgocallbackg()
// 	3. 还有其他的一些调用方, 大都是汇编的调用, 主要是各平台的 Syscall(SB) 函数.
#pragma textflag NOSPLIT
void
·entersyscall(int32 dummy)
{
	// Disable preemption because during this function 
	// g is in Gsyscall status,
	// but can have inconsistent g->sched, 
	// do not let GC observe it.
	// 禁止抢占. 因为在此函数中 g 将处于(或者说被修改为) Gsyscall 状态,
	// 但可能会出现 g->sched 不一致的情况, 别让gc线程发现这里.
	m->locks++;

	// Leave SP around for GC and traceback.
	// 保留SP用于GC和回溯
	save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));

	g->syscallsp = g->sched.sp;
	g->syscallpc = g->sched.pc;
	g->syscallstack = g->stackbase;
	g->syscallguard = g->stackguard;
	g->status = Gsyscall;

	// 这里 if 语句的第一个条件是判断栈溢出的, 可参考 stack.c -> runtime·newstack() 
	// 中的 overflow 检查.
	// 至于第二个条件, 由于 g->stackbase 本来就表示
	if(g->syscallsp < g->syscallguard-StackGuard || 
		g->syscallstack < g->syscallsp) {
		// runtime·printf("entersyscall inconsistent %p [%p,%p]\n",
		//	g->syscallsp, g->syscallguard-StackGuard, g->syscallstack);
		runtime·throw("entersyscall");
	}

	// TODO: fast atomic
	// 只有 sysmon() 这一个地方将 sysmonwait 设置为1, 
	// 然后在 sysmonnote 陷入休眠(当没有任务需要监控的时候)
	// 现在任务来了, 唤醒ta吧.
	// ...sysmon 还能监控 syscall 的??? 没注意
	if(runtime·atomicload(&runtime·sched.sysmonwait)) { 
		runtime·lock(&runtime·sched);
		if(runtime·atomicload(&runtime·sched.sysmonwait)) {
			runtime·atomicstore(&runtime·sched.sysmonwait, 0);
			runtime·notewakeup(&runtime·sched.sysmonnote);
		}
		runtime·unlock(&runtime·sched);
		// 这里又来一句这个是啥意思???
		save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));
	}

	// 这里的目的是? 
	// 进入 syscall 应该是解绑m与g的关系吧, 解绑这俩做什么? 也没解绑 m->p 为 nil啊...
	// 另外, 此时 m 的状态是?
	m->mcache = nil;
	m->p->m = nil;
	runtime·atomicstore(&m->p->status, Psyscall);
	// 一般在gc操作前的 runtime·stoptheworld() 会将 gcwaiting 设置为1.
	// if条件为true, 那么应该是处于gc前的STW过程中, 
	// 而且很大可能 stopwait 不为0, 正在 stopnote 处休眠.
	// 那此时就不应该进入 syscall 了, 先执行gc流程吧.
	// 看样子经过下面这句就只能等下一次调度了.
	if(runtime·sched.gcwaiting) {
		runtime·lock(&runtime·sched);
		if (runtime·sched.stopwait > 0 && 
			runtime·cas(&m->p->status, Psyscall, Pgcstop)) {
			if(--runtime·sched.stopwait == 0)
				runtime·notewakeup(&runtime·sched.stopnote);
		}
		runtime·unlock(&runtime·sched);
		// ...这又是啥意思???
		save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));
	}

	// Goroutines must not split stacks in Gsyscall status 
	// (it would corrupt g->sched).
	// We set stackguard to StackPreempt 
	// so that first split stack check calls morestack.
	// Morestack detects this case and throws.
	// 处于 syscall 状态的g不可分割栈(会导致 g->sched 混乱).
	// 这里我们设置 stackguard0 为 StackPreempt, 
	// 以便第一次栈分隔检查可以调用 morestack(),
	// 而 morestack() 将检查分割栈的情况并抛出异常.
	// 
	// 其他地方在设置 stackguard0 为 StackPreempt 时, 通常都会判断 g->preempt 是否为1.
	// 而且通常是在 locks--后,
	// 而这里没有判断, 还在 locks-- 之前, 说明此次操作不需要的前提, 
	// 设置 stackguard0 是直接目的.
	g->stackguard0 = StackPreempt;
	m->locks--;
}

// The same as runtime·entersyscall(), 
// but with a hint that the syscall is blocking.
// 与 runtime·entersyscall() 相同, 但这里的系统调用是阻塞的.
// 没懂什么意思...??? handoff()这一行是阻塞的吗? 直到 syscall 完成?
//
// caller: 
// 	1. lock_sema.c/lock_futex.c -> runtime·notetsleepg() 只有这一处.
#pragma textflag NOSPLIT
void
·entersyscallblock(int32 dummy)
{
	P *p;

	// see comment in entersyscall
	m->locks++; 

	// Leave SP around for GC and traceback.
	// 保留SP用于GC和回溯
	save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));

	g->syscallsp = g->sched.sp;
	g->syscallpc = g->sched.pc;
	g->syscallstack = g->stackbase;
	g->syscallguard = g->stackguard;
	g->status = Gsyscall;

	if(g->syscallsp < g->syscallguard-StackGuard || 
		g->syscallstack < g->syscallsp) {
		// runtime·printf("entersyscall inconsistent %p [%p,%p]\n",
		//	g->syscallsp, g->syscallguard-StackGuard, g->syscallstack);
		runtime·throw("entersyscallblock");
	}
	// 直到这里, 仍与 ·entersyscall 保持一致, 区别在后半部分.

	p = releasep();
	// 把 p 移交到新的 m
	handoffp(p);
	// do not consider blocked scavenger for deadlock detection
	if(g->isbackground) incidlelocked(1);

	// Resave for traceback during blocked call.
	save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));

	// see comment in entersyscall
	g->stackguard0 = StackPreempt; 
	m->locks--;
}

// 协程 g 从ta的系统调用中返回, 安排ta在某个cpu上继续运行.
//
// caller:
// 	1. src/pkg/syscall/asm_linux_amd64.s -> ·Syscall()
// 	2. src/pkg/syscall/asm_linux_amd64.s -> ·Syscall6()
//     与 runtime·entersyscall() 成对存在.
//
//
// The goroutine g exited its system call.
// Arrange for it to run on a cpu again.
// This is called only from the go syscall library, not
// from the low-level system calls used by the runtime.
#pragma textflag NOSPLIT
void
runtime·exitsyscall(void)
{
	m->locks++;  // see comment in entersyscall

	// do not consider blocked scavenger for deadlock detection
	if(g->isbackground) incidlelocked(-1);

	// exitsyscallfast 为 m 重新绑定 p 对象, 绑定成功则为true.
	// 注意, 进入此if块后必定会 return
	if(exitsyscallfast()) {
		// There's a cpu for us, so we can run.
		// 运行到这里, 表示 m 获取到了一个p(cpu)对象
		m->p->syscalltick++;
		g->status = Grunning;
		// Garbage collector isn't running (since we are),
		// so okay to clear gcstack and gcsp.
		// ...这tm写叉了吧, 清理的明明是syscall的东西
		g->syscallstack = (uintptr)nil;
		g->syscallsp = (uintptr)nil;
		m->locks--;
		// 一般在判断g的 preempt 时都会先判断一个m的 locks 值是否为0
		// 但这里没有, 是能保证此时 locks 值一定为0吗???
		if(g->preempt) {
			// restore the preemption request 
			// in case we've cleared it in newstack
			g->stackguard0 = StackPreempt;
		} else {
			// otherwise restore the real stackguard, 
			// we've spoiled it in entersyscall/entersyscallblock
			g->stackguard0 = g->stackguard;
		}
		return;
	}

	m->locks--;

	// 运行到这里, 说明当前 m 并没有成功绑定到一个p(毕竟 p 是有限且相对固定的)
	// Call the scheduler.
	runtime·mcall(exitsyscall0);

	// Scheduler returned, so we're allowed to run now.
	// Delete the gcstack information that we left for
	// the garbage collector during the system call.
	// Must wait until now because until gosched returns
	// we don't know for sure that the garbage collector
	// is not running.
	// 调度器返回, 那么可以开始执行了.
	// 删除...这里的注释应该也是错的.
	// 删除 syscall 栈信息
	g->syscallstack = (uintptr)nil;
	g->syscallsp = (uintptr)nil;
	m->p->syscalltick++;
}

// 从系统调用中返回时, 为当前 m 重新绑定 p.
// 可以是上一个绑定过的 p, 也可以从调度器中重新获取另一个空闲的 p.
//
// caller: 
// 	1. runtime·exitsyscall() 系统调用完成后, 调用此函数为 m 重新绑定 p 对象.
// 
// return: 绑定成功返回true, 否则返回false.
#pragma textflag NOSPLIT
static bool
exitsyscallfast(void)
{
	P *p;

	// Freezetheworld sets stopwait but does not retake P's.
	// 这是啥意思??? 
	if(runtime·sched.stopwait) {
		m->p = nil;
		return false;
	}

	// Try to re-acquire the last P.
	// 1. 尝试绑定上一个p(即 m->p)
	// 在 m 处于系统调用期间, m->p 可能会被 sysmon 线程拿走, 
	// 此时 status 状态会从 Psyscall 变成其他的.
	if(m->p && m->p->status == Psyscall && 
		runtime·cas(&m->p->status, Psyscall, Prunning)) {
		// There's a cpu for us, so we can run.
		// 真的找回了这个cpu(就是 p 对象), 那么重新绑定
		m->mcache = m->p->mcache;
		m->p->m = m;
		return true;
	}

	// Try to get any other idle P.
	// 2. m->p 被 sysmon 线程拿走了, 则尝试重新获取一个空闲的 p.
	m->p = nil;
	if(runtime·sched.pidle) {
		runtime·lock(&runtime·sched);
		p = pidleget();
		// sysmonwait 只有sysmon()这一个地方被设置为1, 然后陷入休眠.
		// 既然p有任务来了, sysmon 也没必要再休眠了.
		if(p && runtime·atomicload(&runtime·sched.sysmonwait)) {
			runtime·atomicstore(&runtime·sched.sysmonwait, 0);
			runtime·notewakeup(&runtime·sched.sysmonnote);
		}
		runtime·unlock(&runtime·sched);
		if(p) {
			// 把p与当前的m绑定
			acquirep(p);
			return true;
		}
	}
	return false;
}

// m 在系统调用完成后, 调用 exitsyscallfast() 发现自己的 p 被 sysmon 线程拿走了, 
// 自己没有 p 对象, 无法再继续 g 任务中系统调用之后的操作了.
// 但是 g 不能不运行, 于是将自己的 g 对象放回到全局队列, 自己则陷入休眠.
//
// caller: 
// 	1. runtime·exitsyscall() 只有这一处
//
// runtime·exitsyscall slow path on g0.
// Failed to acquire P, enqueue gp as runnable.
static void
exitsyscall0(G *gp)
{
	// 自定义 runtime 信息打印
	if(runtime·debug.scheddetail >= 3) {
		int64 curg_id = -1;
		if(m->curg != nil) {
			curg_id = m->curg->goid;
		}
		runtime·printf(
			"exitsyscall0() called - global g: %D, m: %d, m.g0: %D, m.curg: %D, m.p: %d, source g: %D\n",
			g->goid, m->id, m->g0->goid, curg_id, m->p->id, gp->goid
		);
	}

	P *p;

	gp->status = Grunnable;
	gp->m = nil;
	m->curg = nil;
	runtime·lock(&runtime·sched);
	// 最后一次尝试获取 p 对象.
	p = pidleget();
	if(p == nil) {
		// 获取失败, 将 g 放入到全局队列.
		globrunqput(gp);
	} else if(runtime·atomicload(&runtime·sched.sysmonwait)) {
		runtime·atomicstore(&runtime·sched.sysmonwait, 0);
		runtime·notewakeup(&runtime·sched.sysmonnote);
	}
	runtime·unlock(&runtime·sched);

	// 获取成功, 关联 p, 继续执行.
	if(p) {
		// 把p与当前的m绑定
		acquirep(p);
		execute(gp);  // Never returns.
	}
	// 此处表示没能获取p
	if(m->lockedg) {
		// Wait until another thread schedules gp and so m again.
		stoplockedm();
		execute(gp);  // Never returns.
	}
	stopm();
	schedule();  // Never returns.
}

// Called from syscall package before fork.
void
syscall·runtime_BeforeFork(void)
{
	// Fork can hang if preempted with signals frequently enough (see issue 5517).
	// Ensure that we stay on the same M where we disable profiling.
	m->locks++;
	if(m->profilehz != 0) runtime·resetcpuprofiler(0);
}

// Called from syscall package after fork in parent.
void
syscall·runtime_AfterFork(void)
{
	int32 hz;

	hz = runtime·sched.profilehz;
	if(hz != 0) runtime·resetcpuprofiler(hz);
	m->locks--;
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
static void
mstackalloc(G *gp)
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
G*
runtime·malg(int32 stacksize)
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
			// running on scheduler stack already.
			// stk 是分配的栈空间的起始地址
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
void
runtime·newproc(int32 siz, FuncVal* fn, ...)
{
	byte *argp;

	if(thechar == '5'){
		argp = (byte*)(&fn+2);  // skip caller's saved LR
	} else {
		argp = (byte*)(&fn+1);
	}
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
G*
runtime·newproc1(FuncVal *fn, byte *argp, int32 narg, int32 nret, void *callerpc)
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
	if(thechar == '5') {
		// caller's LR
		sp -= sizeof(void*);
		*(void**)sp = nil;
	}

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
	if(raceenabled) newg->racectx = runtime·racegostart((void*)callerpc);
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
	if(m->locks == 0 && g->preempt) g->stackguard0 = StackPreempt;
	return newg;
}

// Put on gfree list.
// If local list is too long, transfer a batch to the global list.
static void
gfput(P *p, G *gp)
{
	if(gp->stackguard - StackGuard != gp->stack0)
		runtime·throw("invalid stack in gfput");
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
static G*
gfget(P *p)
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
static void
gfpurge(P *p)
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

void
runtime·Breakpoint(void)
{
	runtime·breakpoint();
}

// golang原生: runtime.Gosched() 函数.
// 
// 开发者可以在代码中主动触发, 将 g 对象放回全局队列, m 执行其他任务.
//
// 啥时候会用到???
void
runtime·Gosched(void)
{
	runtime·gosched();
}

// Implementation of runtime.GOMAXPROCS.
// delete when scheduler is even stronger
// 调整的过程中用到了 STW 操作.
int32
runtime·gomaxprocsfunc(int32 n)
{
	int32 ret;

	if(n > MaxGomaxprocs) n = MaxGomaxprocs;
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

// lockOSThread is called by runtime.LockOSThread 
// and runtime.lockOSThread below after they modify m->locked. 
// Do not allow preemption during this call,
// or else the m might be different in this function than in the caller.
// caller: runtime·LockOSThread(), runtime·lockOSThread() 就在下面
// 调用此函数时需要禁止抢占, 否则这里的m对象与调用者的m就不相同了.
// 所以禁用抢占是为了防止m切换???
// 这个函数看起来就是将m和g绑定啊...
#pragma textflag NOSPLIT
static void
lockOSThread(void)
{
	m->lockedg = g;
	g->lockedm = m;
}

void
runtime·LockOSThread(void)
{
	m->locked |= LockExternal;
	lockOSThread();
}

void
runtime·lockOSThread(void)
{
	m->locked += LockInternal;
	lockOSThread();
}


// unlockOSThread is called by runtime.UnlockOSThread
// and runtime.unlockOSThread below after they update m->locked. 
// Do not allow preemption during this call,
// or else the m might be in different in this function than in the caller.
// caller: runtime·UnlockOSThread(), runtime·unlockOSThread()
// 与lockOSThread()相同, 调用此函数也需要禁止抢占.
#pragma textflag NOSPLIT
static void
unlockOSThread(void)
{
	if(m->locked != 0) return;
	m->lockedg = nil;
	g->lockedm = nil;
}

void
runtime·UnlockOSThread(void)
{
	m->locked &= ~LockExternal;
	unlockOSThread();
}

void
runtime·unlockOSThread(void)
{
	if(m->locked < LockInternal)
		runtime·throw("runtime: internal error: misuse of lockOSThread/unlockOSThread");
	m->locked -= LockInternal;
	unlockOSThread();
}

bool
runtime·lockedOSThread(void)
{
	return g->lockedm != nil && m->lockedg != nil;
}

// for testing of callbacks
void
runtime·golockedOSThread(bool ret)
{
	ret = runtime·lockedOSThread();
	FLUSH(&ret);
}

void
runtime·NumGoroutine(intgo ret)
{
	ret = runtime·gcount();
	FLUSH(&ret);
}

int32
runtime·gcount(void)
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
		if(s == Grunnable || s == Grunning || s == Gsyscall || s == Gwaiting)
			n++;
	}
	runtime·unlock(&runtime·sched);
	return n;
}

int32
runtime·mcount(void)
{
	return runtime·sched.mcount;
}

void
runtime·badmcall(void (*fn)(G*))  // called from assembly
{
	USED(fn); // TODO: print fn?
	runtime·throw("runtime: mcall called on m->g0 stack");
}

void
runtime·badmcall2(void (*fn)(G*))  // called from assembly
{
	USED(fn);
	runtime·throw("runtime: mcall function returned");
}

void
runtime·badreflectcall(void) // called from assembly
{
	runtime·panicstring("runtime: arg size to reflect.call more than 1GB");
}

static struct {
	Lock;
	void (*fn)(uintptr*, int32);
	int32 hz;
	uintptr pcbuf[100];
} prof;

static void
System(void)
{
}

// Called if we receive a SIGPROF signal.
void
runtime·sigprof(uint8 *pc, uint8 *sp, uint8 *lr, G *gp)
{
	int32 n;
	bool traceback;

	if(prof.fn == nil || prof.hz == 0) return;
	traceback = true;

	// Windows does profiling in a dedicated thread w/o m.
	if(!Windows && (m == nil || m->mcache == nil)) {
		traceback = false;
	}
	
	// Define that a "user g" is a user-created goroutine, and a "system g"
	// is one that is m->g0 or m->gsignal. We've only made sure that we
	// can unwind user g's, so exclude the system g's.
	//
	// It is not quite as easy as testing gp == m->curg (the current user g)
	// because we might be interrupted for profiling halfway through a
	// goroutine switch. The switch involves updating three (or four) values:
	// g, PC, SP, and (on arm) LR. The PC must be the last to be updated,
	// because once it gets updated the new g is running.
	//
	// When switching from a user g to a system g, LR is not considered live,
	// so the update only affects g, SP, and PC. Since PC must be last, there
	// the possible partial transitions in ordinary execution are (1) g alone is updated,
	// (2) both g and SP are updated, and (3) SP alone is updated.
	// If g is updated, we'll see a system g and not look closer.
	// If SP alone is updated, we can detect the partial transition by checking
	// whether the SP is within g's stack bounds. (We could also require that SP
	// be changed only after g, but the stack bounds check is needed by other
	// cases, so there is no need to impose an additional requirement.)
	//
	// There is one exceptional transition to a system g, not in ordinary execution.
	// When a signal arrives, the operating system starts the signal handler running
	// with an updated PC and SP. The g is updated last, at the beginning of the
	// handler. There are two reasons this is okay. First, until g is updated the
	// g and SP do not match, so the stack bounds check detects the partial transition.
	// Second, signal handlers currently run with signals disabled, so a profiling
	// signal cannot arrive during the handler.
	//
	// When switching from a system g to a user g, there are three possibilities.
	//
	// First, it may be that the g switch has no PC update, because the SP
	// either corresponds to a user g throughout (as in runtime.asmcgocall)
	// or because it has been arranged to look like a user g frame
	// (as in runtime.cgocallback_gofunc). In this case, since the entire
	// transition is a g+SP update, a partial transition updating just one of 
	// those will be detected by the stack bounds check.
	//
	// Second, when returning from a signal handler, the PC and SP updates
	// are performed by the operating system in an atomic update, so the g
	// update must be done before them. The stack bounds check detects
	// the partial transition here, and (again) signal handlers run with signals
	// disabled, so a profiling signal cannot arrive then anyway.
	//
	// Third, the common case: it may be that the switch updates g, SP, and PC
	// separately, as in runtime.gogo.
	//
	// Because runtime.gogo is the only instance, we check whether the PC lies
	// within that function, and if so, not ask for a traceback. This approach
	// requires knowing the size of the runtime.gogo function, which we
	// record in arch_*.h and check in runtime_test.go.
	//
	// There is another apparently viable approach, recorded here in case
	// the "PC within runtime.gogo" check turns out not to be usable.
	// It would be possible to delay the update of either g or SP until immediately
	// before the PC update instruction. Then, because of the stack bounds check,
	// the only problematic interrupt point is just before that PC update instruction,
	// and the sigprof handler can detect that instruction and simulate stepping past
	// it in order to reach a consistent state. On ARM, the update of g must be made
	// in two places (in R10 and also in a TLS slot), so the delayed update would
	// need to be the SP update. The sigprof handler must read the instruction at
	// the current PC and if it was the known instruction (for example, JMP BX or 
	// MOV R2, PC), use that other register in place of the PC value.
	// The biggest drawback to this solution is that it requires that we can tell
	// whether it's safe to read from the memory pointed at by PC.
	// In a correct program, we can test PC == nil and otherwise read,
	// but if a profiling signal happens at the instant that a program executes
	// a bad jump (before the program manages to handle the resulting fault)
	// the profiling handler could fault trying to read nonexistent memory.
	//
	// To recap, there are no constraints on the assembly being used for the
	// transition. We simply require that g and SP match and that the PC is not
	// in runtime.gogo.
	//
	// On Windows, one m is sending reports about all the g's, so gp == m->curg
	// is not a useful comparison. The profilem function in os_windows.c has
	// already checked that gp is a user g.
	if(gp == nil ||
	   (!Windows && gp != m->curg) ||
	   (uintptr)sp < gp->stackguard - StackGuard || gp->stackbase < (uintptr)sp ||
	   ((uint8*)runtime·gogo <= pc && pc < (uint8*)runtime·gogo + RuntimeGogoBytes))
		traceback = false;

	// Race detector calls asmcgocall w/o entersyscall/exitsyscall,
	// we can not currently unwind through asmcgocall.
	if(m != nil && m->racecall) traceback = false;

	runtime·lock(&prof);
	if(prof.fn == nil) {
		runtime·unlock(&prof);
		return;
	}
	n = 0;
	if(traceback) {
		n = runtime·gentraceback(
			(uintptr)pc, (uintptr)sp, (uintptr)lr, gp, 0, 
			prof.pcbuf, nelem(prof.pcbuf), nil, nil, false
		);
	}
	if(!traceback || n <= 0) {
		n = 2;
		prof.pcbuf[0] = (uintptr)pc;
		prof.pcbuf[1] = (uintptr)System + 1;
	}
	prof.fn(prof.pcbuf, n);
	runtime·unlock(&prof);
}

// Arrange to call fn with a traceback hz times a second.
void
runtime·setcpuprofilerate(void (*fn)(uintptr*, int32), int32 hz)
{
	// Force sane arguments.
	if(hz < 0) hz = 0;
	if(hz == 0) fn = nil;
	if(fn == nil) hz = 0;

	// Disable preemption, otherwise we can be rescheduled to another thread
	// that has profiling enabled.
	m->locks++;

	// Stop profiler on this thread so that it is safe to lock prof.
	// if a profiling signal came in while we had prof locked,
	// it would deadlock.
	runtime·resetcpuprofiler(0);

	runtime·lock(&prof);
	prof.fn = fn;
	prof.hz = hz;
	runtime·unlock(&prof);
	runtime·lock(&runtime·sched);
	runtime·sched.profilehz = hz;
	runtime·unlock(&runtime·sched);

	if(hz != 0) runtime·resetcpuprofiler(hz);

	m->locks--;
}

// 修改 p 对象的数量为 new, 不足的补上, 多余的丢掉(主要是修改 runtime·gomaxprocs 的值).
//
// caller: 
// 	1. runtime·schedinit() 为 P 链表申请完空间后, 调用此函数将 P 数量调整到合适值.
// 	2. runtime·starttheworld()
//
// 话说, 按照这两个调用函数的时机, 似乎不会存在动态调整p数量的情况?
// 还是说每次调整p数量, 是要在本轮gc结束后的 starttheworld() ?
//
// Change number of processors. 
// The world is stopped, sched is locked.
static void
procresize(int32 new)
{
	int32 i, old;
	G *gp;
	P *p;
	// 获取此次调用之前的 runtime·gomaxprocs 的值
	old = runtime·gomaxprocs;
	if(old < 0 || old > MaxGomaxprocs || new <= 0 || new >MaxGomaxprocs) {
		runtime·throw("procresize: invalid arg");
	}

	// 初始化新的 P 对象(少补), 同时为 mcache(缓存空间) 与 runq(任务队列) 成员分配空间.
	// 
	// 注意 for 循环是遍历 new 进行累加(allp 是数组, 所以中间不会存在空缺).
	//
	// initialize new P's
	for(i = 0; i < new; i++) {
		p = runtime·allp[i];
		if(p == nil) {
			// 创建的新的 p 对象, 标记其为不通过GC回收
			p = (P*)runtime·mallocgc(sizeof(*p), 0, FlagNoInvokeGC);
			p->id = i;
			p->status = Pgcstop;
			runtime·atomicstorep(&runtime·allp[i], p);
		}
		// runtime·mallocgc() 应该不会自动为 p 分配 mcache 吧
		// 我觉得在 runtime·schedinit() 调用时, 此if条件总是成立的.
		if(p->mcache == nil) {
			if(old==0 && i==0){
				p->mcache = m->mcache;  // bootstrap
			}
			else {
				p->mcache = runtime·allocmcache();
			}
		}
		// p 的本地队列
		if(p->runq == nil) {
			p->runqsize = 128;
			p->runq = (G**)runtime·mallocgc(p->runqsize*sizeof(G*), 0, FlagNoInvokeGC);
		}
	}

	// 均匀地分发可执行的 g 任务
	// ...不过看起来好像是把原来所有 p 中的所有 g 任务, 都先取出来放到全局任务队列了.
	// 下面一个for循环就是将全局队列中的任务平均分配到 runtime·allp 中各 p 对象的本地队列中.
	//
	// redistribute runnable G's evenly
	for(i = 0; i < old; i++) {
		p = runtime·allp[i];
		while(gp = runqget(p)) globrunqput(gp);
	}

	// 将 G 从全局队列重新分配到 P.
	//
	// 从1开始, 是因为当前M(m0)已经运行了一个g任务, 而且要占用 allp[0](在下面有定义),
	// 所以如果我们有空闲的 g 对象, 要从 allp[1] 开始分配.
	//
	// start at 1 because current M already executes some G 
	// and will acquire allp[0] below,
	// so if we have a spare G we want to put it into allp[1].
	for(i = 1; runtime·sched.runqhead; i++) {
		// runqhead 指针后移
		gp = runtime·sched.runqhead;
		runtime·sched.runqhead = gp->schedlink;
		runqput(runtime·allp[i%new], gp);
	}

	// 上面的for循环结束, 表示 runqhead 为 nil, 配合下面两句, 即是将全局任务队列清空了.
	runtime·sched.runqtail = nil;
	runtime·sched.runqsize = 0;

	// 释放多余的 p(多退). 从 allp[new] 开始, 包括 mcache 与 gfree 空间.
	//
	// 值得一提的是, gfree 其实并没有被直接释放, 而是转移到了全局调度器的 gfree 成员中.
	//
	// free unused P's
	for(i = new; i < old; i++) {
		p = runtime·allp[i];
		runtime·freemcache(p->mcache);
		p->mcache = nil;
		gfpurge(p);
		p->status = Pdead;
		// 这里只将p的状态修改为了 Pdead, 不能直接释放其空间,
		// 因为ta可能被某个陷入系统调用的m引用.
		//
		// can't free P itself because it can be referenced by an M in syscall
	}

	if(m->p) m->p->m = nil;

	m->p = nil;
	m->mcache = nil;
	p = runtime·allp[0];
	p->m = nil;
	p->status = Pidle;
	// 将位于 allp[0] 的 p 绑定在当前 m 上.
	// 如果主调函数为 runtime·schedinit(), 则当前 m 应该指的是 m0.
	acquirep(p);
	// 将 (1, new-1) 的 p 对象置为 idle, 并将ta们放入全局空闲队列, 之后可以开始干活了...
	for(i = new-1; i > 0; i--) {
		p = runtime·allp[i];
		p->status = Pidle;
		// 调用此函数需要将全局调度器加锁, 这里为什么没加??? 
		// 是因为在两个可能的调用者中可以保证此时是单线程吗?
		pidleput(p);
	}

	// 注意这里, 修改了 runtime·gomaxprocs 的值.
	runtime·atomicstore((uint32*)&runtime·gomaxprocs, new);
}

// 将目标 p 对象与当前 m 关联(只是修改了一下m->p的指向), p 的状态需要是 idle
//
// caller:
// 	1. procresize()
//
// Associate p and the current m.
static void
acquirep(P *p)
{
	if(m->p || m->mcache) runtime·throw("acquirep: already in go");
	if(p->m || p->status != Pidle) {
		runtime·printf(
			"acquirep: p->m=%p(%d) p->status=%d\n", 
			p->m, p->m ? p->m->id : 0, p->status
		);
		runtime·throw("acquirep: invalid p state");
	}
	// 原来 m->mcache 与其绑定的 p 的mcache是同一个.
	m->mcache = p->mcache;
	m->p = p;
	p->m = m;
	p->status = Prunning;
}

// 将当前 m 与其 p 对象解绑, 将这个 p 标记为 idle 状态并返回.
//
// 与 acquirep 正好相反.
//
// Disassociate p and the current m.
static P*
releasep(void)
{
	P *p;

	if(m->p == nil || m->mcache == nil)
		runtime·throw("releasep: invalid arg");
	p = m->p;
	if(p->m != m || p->mcache != m->mcache || p->status != Prunning) {
		runtime·printf("releasep: m=%p m->p=%p p->m=%p m->mcache=%p p->mcache=%p p->status=%d\n",
			m, m->p, p->m, m->mcache, p->mcache, p->status);
		runtime·throw("releasep: invalid p state");
	}
	m->p = nil;
	m->mcache = nil;
	p->m = nil;
	p->status = Pidle;
	return p;
}

// ...很简单, 容易理解.
static void
incidlelocked(int32 v)
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
static void
checkdead(void)
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

// 监控线程.
//
// 1. 将 netpoll 结果放到全局队列
// 2. 收回长时间处于 Psyscall 状态的 P
// 3. 向长时间运行的 g 发出抢占通知(其实除了 sysmon, 调度器在很多地方都会设置抢占标志)
//
// 监控线程也有可能休眠, 比如在 gc 时, 或是所有 p 都空闲的时候, 因为这些时候没有监控的必要.
//
// caller: 
// 	1. runtime·main() 在执行用户代码前, 启动独立线程执行此函数.
//
// 注意: 此函数并不是以 g 的形式执行(因此也不需要 p), 而是由 m 直接执行, 直接绑定一个系统线程.
static void
sysmon(void)
{
	uint32 idle, delay;
	int64 now, lastpoll, lasttrace;
	G *gp;

	lasttrace = 0;
	// how many cycles in succession we had not wokeup somebody
	idle = 0; 
	delay = 0;
	// 无限循环, 无 break, 无 continue.
	for(;;) {
		if(idle == 0) {
			// start with 20us sleep...
			// 首次启动, 设置 delay 为 20
			delay = 20; 
		} else if(idle > 50) {
			// start doubling the sleep after 1ms...
			// idle 每次循环就加1, 即 20us * 50 = 1ms 后,
			// delay 的值每次循环就乘以2, 不过有上限
			delay *= 2;
		}

		// up to 10ms
		// delay 最大为 10ms
		if(delay > 10*1000) {
			delay = 10*1000;
		} 
		// runtime·usleep 是汇编代码, 实际是 select 的系统调用. 
		runtime·usleep(delay);

		// 1. schedtrace <= 0, 不过目前并没有看见哪里有设置这个值
		// 2. 当前进程处于gc流程, 或是所有的p都处于 idle 状态
		// 这种情况下, 也没有必要监控了, 反正大家都没在干活...
		// 于是会陷入休眠, 直接其他地方获取了一个空闲的p, 重新开始参与任务的执行.
		if(runtime·debug.schedtrace <= 0 &&
			(runtime·sched.gcwaiting || 
				runtime·atomicload(&runtime·sched.npidle) == runtime·gomaxprocs)) { 
			// TODO: fast atomic
			runtime·lock(&runtime·sched);
			// ...再次检查, 而且这次还是原子地获取 gcwaiting 的值
			if(runtime·atomicload(&runtime·sched.gcwaiting) || 
				runtime·atomicload(&runtime·sched.npidle) == runtime·gomaxprocs) {
				// 只有这一个地方将 sysmonwait 设置为 1
				runtime·atomicstore(&runtime·sched.sysmonwait, 1);
				runtime·unlock(&runtime·sched);
				// 陷入休眠, 等待被唤醒, 因为目前没有监控的必要.
				// 在 starttheworld, exitsyscallfast 中会为空闲的p
				// 赋予新的任务, 于是被唤醒, 重新开始监控流程.
				// 当然, 每一次重开监控, idle 和 delay 的值都会归零重新计算.
				runtime·notesleep(&runtime·sched.sysmonnote);
				runtime·noteclear(&runtime·sched.sysmonnote);
				// 一次休眠然后被唤醒后, 重置 idle 和 delay
				idle = 0;
				delay = 20;
			} else{
				// 这里相当于什么都没做, 就是说如果有p正在做事, 
				runtime·unlock(&runtime·sched);
			}
		}
		// poll network if not polled for more than 10ms
		// 如果距上次循环超过了 10ms
		lastpoll = runtime·atomicload64(&runtime·sched.lastpoll);
		now = runtime·nanotime();
		if(lastpoll != 0 && lastpoll + 10*1000*1000 < now) {
			runtime·cas64(&runtime·sched.lastpoll, lastpoll, now);
			// non-blocking
			gp = runtime·netpoll(false); 
			if(gp) {
				// Need to decrement number of idle locked M's
				// (pretending that one more is running) before injectglist.
				// Otherwise it can lead to the following situation:
				// injectglist grabs all P's but before it starts M's to run the P's,
				// another M returns from syscall, finishes running its G,
				// observes that there is no work to do and no other running M's
				// and reports deadlock.
				// 先将处于 locked 状态的m的数量减1(假设m的数量大于1)
				// 否则可能会出现这样的情况: 
				// injectglist 将gp任务链表放入全局调度器的任务队列(标记ta们为 runnable ),
				// 在末尾会启动额外的m来运行这些任务(如果存在 idle 的p).
				// 但在启动新m之前, 对全局调度器解锁的瞬间, 有可能一些m对象从 syscall 中返回,
				// 或是已经运行完了ta的g任务, 发现已经没有其他任务了, 也没有其他正在 running 的m, 
				// 就会报 deadlock.
				incidlelocked(-1);
				injectglist(gp);
				incidlelocked(1);
			}
		}
		// retake P's blocked in syscalls and preempt long running G's
		// retake 回收陷入系统调用的p对象, 同时也抢占运行时间比较长的g任务.
		if(retake(now)){
			idle = 0;
		} else {
			idle++;
		}

		if(runtime·debug.schedtrace > 0 && lasttrace + runtime·debug.schedtrace*1000000ll <= now) {
			lasttrace = now;
			runtime·schedtrace(runtime·debug.scheddetail);
		}
	}
}

typedef struct Pdesc Pdesc;
struct Pdesc
{
	uint32	schedtick;
	int64	schedwhen;
	uint32	syscalltick;
	int64	syscallwhen;
};
// pdesc 没有地方初始化, 所以第一次使用时需要判断各字段的空值情况
static Pdesc pdesc[MaxGomaxprocs];

// retake 回收陷入系统调用的 p 对象, 同时也抢占运行时间比较长的g任务.
// (防止某一个任务占用CPU太久影响其他任务的执行)
//
// 	param now: 为绝对时间 nanotime
//
// 返回值 n 为
// 注意对 running 状态的 p 的 g 任务的抢占并不计在 n 中, 因为抢占操作只是一个通知,
// 实际当 p 不再运行 g, 需要等待到下一次调度.
//
// caller: 
// 	1. sysmon() 只有这一处
static uint32
retake(int64 now)
{
	uint32 i, s, n;
	int64 t;
	P *p;
	Pdesc *pd;

	n = 0;
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p==nil) continue;
		pd = &pdesc[i];
		s = p->status;
		if(s == Psyscall) {
			// Retake P from syscall if it's there for more than 1 sysmon tick (20us).
			// But only if there is other work to do.
			t = p->syscalltick;
			// if 条件为真, 说明是这个p对应的 pd 第一次被调用, 就当作初始化了
			if(pd->syscalltick != t) {
				pd->syscalltick = t;
				pd->syscallwhen = now;
				continue;
			}
			// 如果该p的本地任务队列只有一个成员,
			// 且有m对象处于 spinning (没活干) 或是p对象处于 idle 状态
			// 总之, 没必要抢占当前的p, 把任务分配给那些空闲的worker更好.
			if(p->runqhead == p->runqtail &&
				runtime·atomicload(&runtime·sched.nmspinning) + runtime·atomicload(&runtime·sched.npidle) > 0) {
				continue;
			}

			// Need to decrement number of idle locked M's
			// (pretending that one more is running) before the CAS.
			// Otherwise the M from which we retake can exit the syscall,
			// increment nmidle and report deadlock.
			incidlelocked(-1);
			if(runtime·cas(&p->status, s, Pidle)) {
				n++;
				handoffp(p);
			}
			incidlelocked(1);
		} else if(s == Prunning) {
			// Preempt G if it's running for more than 10ms.
			// 如果该p被调度处于 running 状态超过了 10ms
			// 就抢占ta的g对象, 以免对单个任务陷入长时间运行,
			// 而造成其他任务没有机会执行的问题.

			// 注意: 对 running 状态的p的抢占并不会影响本次 retake() 执行的结果
			// 因为此函数的返回值为 n, 这一部分的代码并不会对 n 有修改.
			t = p->schedtick;
			// if 条件为真, 说明是这个p对应的 pd 第一次被调用, 就当作初始化了
			if(pd->schedtick != t) {
				pd->schedtick = t;
				pd->schedwhen = now;
				continue;
			}
			if(pd->schedwhen + 10*1000*1000 > now) {
				continue;
			}
			preemptone(p);
		}
	}
	return n;
}

// 告诉所有的正在运行的goroutines(其实遍历的是p对象链表), ta们被抢占了, 需要停止.
// 只在 gc STW 时会用到.
//
// ta只是用for循环对每个处于running状态的p对象调用 preemptone(p),
// 然后返回结果, 没有额外操作.
//
// 1. 这个函数纯粹就是尽最大努力, 并不保证结果.
// 当一个 p 刚开始运行一个goroutine时, 这个函数对这个 g 的通知可能会失败.
// 2. 无需持有锁.
// 3. 只要有一个goroutine被通知到了, 结果就是true.
//
// caller: 
// 	1. runtime·freezetheworld()
// 	2. runtime·stoptheworld()
//
// Tell all goroutines that they have been preempted and they should stop.
// This function is purely best-effort. 
// It can fail to inform a goroutine if a processor just started running it.
// No locks need to be held.
// Returns true if preemption request was issued to at least one goroutine.
static bool
preemptall(void)
{
	P *p;
	int32 i;
	bool res;

	res = false;
	// 遍历 p 队列
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p == nil || p->status != Prunning) {
			continue;
		}
		res |= preemptone(p);
	}
	return res;
}

// preemptone 告知目标 p 上正在运行的 goroutine 停止(被抢占)
//
// 1. ta同样只是尽最大努力, 并不保证结果.
//    1. 很可能错误的没有通知到.
//    2. 可能通知到别的 g (协程切换的时候吧..)
//    3. 即使通知到了, 目标 g 也可能会忽略, 比如ta正在执行 runtime·newstack() 操作.
// 2. 无需持有锁
// 3. 如果抢占请求被通知到了, 就返回true.
//
// 那么问题来了, 这里顶多只设置了g对象的标识位就返回了, 
// g 应该并不是立刻停止运行吧? 从哪一时刻真正停止呢???
//
// caller:
// 	1. retake() 由 sysmon() 线程回收陷入系统调用的 p 对象, 同时也抢占运行时间比较长的g任务.
// 	2. preemptall() 对所有 p 循环调用此函数.
//
// Tell the goroutine running on processor P to stop.
// This function is purely best-effort. 
// It can incorrectly fail to inform the goroutine. 
// It can send inform the wrong goroutine. 
// Even if it informs the correct goroutine, 
// that goroutine might ignore the request if it is
// simultaneously executing runtime·newstack.
// No lock needs to be held. 
// Returns true if preemption request was issued.
//
static bool
preemptone(P *p)
{
	M *mp;
	G *gp;

	// 既然是抢占, 当然是我抢占别人啊.
	// m就是调用者本身所在的M对象吧, 肯定不能抢啊.
	mp = p->m;
	if(mp == nil || mp == m) {
		return false;
	} 

	gp = mp->curg;
	if(gp == nil || gp == mp->g0) {
		return false;
	}
	// 设置一个p->m->g对象的状态标记就可以了.
	gp->preempt = true;
	gp->stackguard0 = StackPreempt;
	return true;
}

// GODEBUG=schedtrace=1000 时生效的方法, 打印出当前GMP调度信息.
//
// param detailed: 添加有 GODEBUG=scheddetail=1 参数时为 true, 否则默认为 false.
//
// 注意: GODEBUG=gctrace=1 在 src/pkg/runtime/mgc0.c -> gc() 方法中实现
//
// caller:
// 	1. sysmon()
// 	2. src/pkg/runtime/panic.c -> runtime·startpanic()
void
runtime·schedtrace(bool detailed)
{
	static int64 starttime;
	int64 now;
	int64 id1, id2, id3;
	int32 i, q, t, h, s;
	int8 *fmt;
	M *mp, *lockedm;
	G *gp, *lockedg;
	P *p;

	now = runtime·nanotime();
	if(starttime == 0) {
		starttime = now;
	}

	runtime·lock(&runtime·sched);
	runtime·printf(
		"SCHED %Dms: gomaxprocs=%d idleprocs=%d threads=%d idlethreads=%d runqueue=%d",
		(now-starttime)/1000000, runtime·gomaxprocs, runtime·sched.npidle, runtime·sched.mcount,
		runtime·sched.nmidle, runtime·sched.runqsize
	);
	if(detailed) {
		runtime·printf(
			" gcwaiting=%d nmidlelocked=%d nmspinning=%d stopwait=%d sysmonwait=%d\n",
			runtime·sched.gcwaiting, runtime·sched.nmidlelocked, runtime·sched.nmspinning,
			runtime·sched.stopwait, runtime·sched.sysmonwait
		);
	}
	// We must be careful while reading data from P's, M's and G's.
	// Even if we hold schedlock, most data can be changed concurrently.
	// E.g. (p->m ? p->m->id : -1) can crash if p->m changes from non-nil to nil.
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p == nil) continue;
		mp = p->m;
		t = p->runqtail;
		h = p->runqhead;
		s = p->runqsize;
		q = t - h;
		if(q < 0) {
			q += s;
		}
		if(detailed) {
			runtime·printf(
				"  P%d: status=%d schedtick=%d syscalltick=%d m=%d runqsize=%d/%d gfreecnt=%d\n",
				i, p->status, p->schedtick, p->syscalltick, mp ? mp->id : -1, q, s, p->gfreecnt
			);
		}
		else {
			// In non-detailed mode format lengths of per-P run queues as:
			// [len1 len2 len3 len4]
			fmt = " %d";
			if(runtime·gomaxprocs == 1) {
				fmt = " [%d]\n";
			}
			else if(i == 0) {
				fmt = " [%d";
			}
			else if(i == runtime·gomaxprocs-1) {
				fmt = " %d]\n";
			}
			runtime·printf(fmt, q);
		}
	}
	if(!detailed) {
		runtime·unlock(&runtime·sched);
		return;
	}
	for(mp = runtime·allm; mp; mp = mp->alllink) {
		p = mp->p;
		gp = mp->curg;
		lockedg = mp->lockedg;
		id1 = -1;
		if(p) {
			id1 = p->id;
		}
		id2 = -1;
		if(gp) {
			id2 = gp->goid;
		}
		id3 = -1;
		if(lockedg) {
			id3 = lockedg->goid;
		}
		runtime·printf(
			"  M%d: p=%D curg=%D mallocing=%d throwing=%d gcing=%d"
			" locks=%d dying=%d helpgc=%d spinning=%d lockedg=%D\n",
			mp->id, id1, id2, mp->mallocing, mp->throwing, mp->gcing, 
			mp->locks, mp->dying, mp->helpgc, mp->spinning, id3
		);
	}
	for(gp = runtime·allg; gp; gp = gp->alllink) {
		mp = gp->m;
		lockedm = gp->lockedm;
		runtime·printf(
			"  G%D: status=%d(%s) m=%d lockedm=%d\n",
			gp->goid, gp->status, gp->waitreason, mp ? mp->id : -1, lockedm ? lockedm->id : -1
		);
	}
	runtime·unlock(&runtime·sched);
}

// 将 mp 放到全局调度器的 midle 链表头部, 并将其标记为 idle.
//
// 调用此函数时必须对sched全局调度器加锁
//
// Put mp on midle list.
// Sched must be locked.
static void
mput(M *mp)
{
	mp->schedlink = runtime·sched.midle;
	runtime·sched.midle = mp;
	runtime·sched.nmidle++;
	checkdead();
}

// 从全局调度器的 midle 链表中取得一个空闲的 m.
//
// 注意: 有可能返回nil
//
// caller:
// 	1. startm()
//
// Try to get an m from midle list.
// Sched must be locked.
static M*
mget(void)
{
	M *mp;

	if((mp = runtime·sched.midle) != nil){
		runtime·sched.midle = mp->schedlink;
		runtime·sched.nmidle--;
	}
	return mp;
}

// Put gp on the global runnable queue.
// Sched must be locked.
static void
globrunqput(G *gp)
{
	gp->schedlink = nil;
	if(runtime·sched.runqtail)
		runtime·sched.runqtail->schedlink = gp;
	else
		runtime·sched.runqhead = gp;
	runtime·sched.runqtail = gp;
	runtime·sched.runqsize++;
}

// 尝试从全局待运行队列获取一组 g 对象链表, 链表中 g 对象的数量最多为max.
// 调用者必须对sched对象加锁
//
// caller:
// 	1. schedule()
//
// Try get a batch of G's from the global runnable queue.
// Sched must be locked.
static G*
globrunqget(P *p, int32 max)
{
	G *gp, *gp1;
	int32 n;

	// ...全局待运行队列为0
	if(runtime·sched.runqsize == 0) return nil;
	// ...要把runq中的任务平均分给每个p吗
	n = runtime·sched.runqsize/runtime·gomaxprocs+1;
	// 下面这种情况应该是gomaxprocs = 1时吧
	if(n > runtime·sched.runqsize) n = runtime·sched.runqsize;

	if(max > 0 && n > max) n = max;
	runtime·sched.runqsize -= n;

	if(runtime·sched.runqsize == 0) runtime·sched.runqtail = nil;
	// 要返回的p链表, 从sched的runqhead开始.
	// 同时将这个链表中的g成员通过qunqput添加到p的本地待执行队列中.
	gp = runtime·sched.runqhead;
	runtime·sched.runqhead = gp->schedlink;
	n--;
	while(n--) {
		gp1 = runtime·sched.runqhead;
		runtime·sched.runqhead = gp1->schedlink;
		runqput(p, gp1);
	}
	return gp;
}

// 将目标 p 对象放入全局调度器的 pidle 空闲链表.
// 
// caller:
// 	1. procresize() p 数量调整完成后, 
//     将 runtime·allp 队列中的 (1, new-1) 的 p 对象(此时都为 idle 状态)放入全局空闲队列中.
//
// Put p to on pidle list.
// Sched must be locked.
static void
pidleput(P *p)
{
	p->link = runtime·sched.pidle;
	runtime·sched.pidle = p;
	// TODO: fast atomic
	runtime·xadd(&runtime·sched.npidle, 1); 
}

// Try get a p from pidle list.
// Sched must be locked.
// 从全局任务队列中获取并返回一个p对象
static P*
pidleget(void)
{
	P *p;

	p = runtime·sched.pidle;
	if(p) {
		runtime·sched.pidle = p->link;
		// TODO: fast atomic
		runtime·xadd(&runtime·sched.npidle, -1); 
	}
	return p;
}

// 将目标 g 对象放到本地 p 对象的待执行队列(runq 成员)
// 与此相对 globrunqput()是将 g 放到全局 runq 执行队列中.
//
// caller:
// 	1. procresize() 调整 p 数量时, 调用此函数将原来的所有 g 任务先放到全局队列中, 然后重新分配.
//
// Put g on local runnable queue.
// TODO(dvyukov): consider using lock-free queue.
static void
runqput(P *p, G *gp)
{
	int32 h, t, s;

	runtime·lock(p);
retry:
	h = p->runqhead;
	t = p->runqtail;
	s = p->runqsize;
	// 这里是 p->runq 队列快满了要扩容吧
	if(t == h-1 || (h == 0 && t == s-1)) {
		runqgrow(p);
		goto retry;
	}
	p->runq[t++] = gp;
	if(t == s) t = 0;
	p->runqtail = t;
	runtime·unlock(p);
}

// 从指定 p 对象的本地待执行队列头部中取出一个 g 任务(逻辑很简单).
//
// caller:
// 	1. procresize() 调整 p 数量时, 调用此函数将原来的所有 g 任务先放到全局队列中, 然后重新分配.
//
// Get g from local runnable queue.
static G*
runqget(P *p)
{
	G *gp;
	int32 t, h, s;
	// 本地待执行队列为空, 返回nil
	if(p->runqhead == p->runqtail) return nil;

	runtime·lock(p);
	h = p->runqhead;
	t = p->runqtail;
	s = p->runqsize;
	// ...t==h不跟第一个if判断一样么
	// 不过也有可能是加锁前可能会发生变动, 
	// 所以这里重新判断了一遍, 同样返回nil
	if(t == h) {
		runtime·unlock(p);
		return nil;
	}
	gp = p->runq[h++]; // 指针++操作, 就是指针后移吧?
	// 这里是什么意思? h指针怎么可以与s相比了???
	if(h == s) h = 0;

	p->runqhead = h;
	runtime·unlock(p);
	return gp;
}

// Grow local runnable queue.
// TODO(dvyukov): consider using fixed-size array
// and transfer excess to the global list 
// (local queue can grow way too big).
static void
runqgrow(P *p)
{
	G **q;
	int32 s, t, h, t2;

	h = p->runqhead;
	t = p->runqtail;
	s = p->runqsize;
	t2 = 0;
	q = runtime·malloc(2*s*sizeof(*q));
	while(t != h) {
		q[t2++] = p->runq[h++];
		if(h == s)
			h = 0;
	}
	runtime·free(p->runq);
	p->runq = q;
	p->runqhead = 0;
	p->runqtail = t2;
	p->runqsize = 2*s;
}

// 从 p2 的本地队列中, 偷取一半的 g 任务对象, 并放到 p 的队列中.
//
// caller:
// 	1. findrunnable() 
//
// Steal half of elements from local runnable queue of p2
// and put onto local runnable queue of p.
// Returns one of the stolen elements (or nil if failed).
static G*
runqsteal(P *p, P *p2)
{
	G *gp, *gp1;
	int32 t, h, s, t2, h2, s2, c, i;

	if(p2->runqhead == p2->runqtail)
		return nil;
	// sort locks to prevent deadlocks
	if(p < p2)
		runtime·lock(p);
	runtime·lock(p2);
	if(p2->runqhead == p2->runqtail) {
		runtime·unlock(p2);
		if(p < p2)
			runtime·unlock(p);
		return nil;
	}
	if(p >= p2)
		runtime·lock(p);
	// now we've locked both queues and know the victim is not empty
	h = p->runqhead;
	t = p->runqtail;
	s = p->runqsize;
	h2 = p2->runqhead;
	t2 = p2->runqtail;
	s2 = p2->runqsize;
	gp = p2->runq[h2++];  // return value
	if(h2 == s2)
		h2 = 0;
	// steal roughly half
	if(t2 > h2)
		c = (t2 - h2) / 2;
	else
		c = (s2 - h2 + t2) / 2;
	// copy
	for(i = 0; i != c; i++) {
		// the target queue is full?
		if(t == h-1 || (h == 0 && t == s-1))
			break;
		// the victim queue is empty?
		if(t2 == h2)
			break;
		gp1 = p2->runq[h2++];
		if(h2 == s2)
			h2 = 0;
		p->runq[t++] = gp1;
		if(t == s)
			t = 0;
	}
	p->runqtail = t;
	p2->runqhead = h2;
	runtime·unlock(p2);
	runtime·unlock(p);
	return gp;
}

void
runtime·testSchedLocalQueue(void)
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
		if(runqget(&p) != nil)
			runtime·throw("runq is not empty initially");
		for(j = 0; j < i; j++)
			runqput(&p, &gs[i]);
		for(j = 0; j < i; j++) {
			if(runqget(&p) != &gs[i]) {
				runtime·printf("bad element at iter %d/%d\n", i, j);
				runtime·throw("bad element");
			}
		}
		if(runqget(&p) != nil)
			runtime·throw("runq is not empty afterwards");
	}
}

void
runtime·testSchedLocalQueueSteal(void)
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
		while(gp = runqget(&p1))
			gp->sig++;
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

extern void runtime·morestack(void);

// f 是否已经是处于栈顶的函数(再没有更上层的主调函数了)
// 比如 runtime·goexit()
//
// caller:
// 	1. src/pkg/runtime/traceback_x86.c -> runtime·gentraceback()
//
// Does f mark the top of a goroutine stack?
bool
runtime·topofstack(Func *f)
{
	return f->entry == (uintptr)runtime·goexit ||
		f->entry == (uintptr)runtime·mstart ||
		f->entry == (uintptr)runtime·mcall ||
		f->entry == (uintptr)runtime·morestack ||
		f->entry == (uintptr)runtime·lessstack ||
		f->entry == (uintptr)_rt0_go;
}

void
runtime∕debug·setMaxThreads(intgo in, intgo out)
{
	runtime·lock(&runtime·sched);
	out = runtime·sched.maxmcount;
	runtime·sched.maxmcount = in;
	checkmcount();
	runtime·unlock(&runtime·sched);
	FLUSH(&out);
}

static int8 experiment[] = GOEXPERIMENT; // defined in zaexperiment.h

static bool
haveexperiment(int8 *name)
{
	int32 i, j;
	
	for(i=0; i<sizeof(experiment); i++) {
		if((i == 0 || experiment[i-1] == ',') && experiment[i] == name[0]) {
			for(j=0; name[j]; j++)
				if(experiment[i+j] != name[j]) goto nomatch;

			if(experiment[i+j] != '\0' && experiment[i+j] != ',')
				goto nomatch;
			return 1;
		}
	nomatch:;
	}
	return 0;
}
