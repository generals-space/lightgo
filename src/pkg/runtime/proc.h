/*
 * proc.h 由 proc.c 拆分而来, 20230526
 */

#include "runtime.h"

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
	// number of m's that have been created
	int32	mcount;
	// 可创建的 m 的最大数量(也就是底层的系统线程数), 在 runtime·schedinit()中声明, 
	// 默认值为1w, 开发者可通过 runtime∕debug·setMaxThreads() 动态设置.
	//
	// maximum number of m's allowed (or die)
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

	// gcwaiting 表示是否处于 GC 过程中.
	//
    // 取值0或1, 在gc操作前后的 runtime·stoptheworld(), 及 runtime·starttheworld()
    // 还有一个 runtime·freezetheworld() 函数会设置这个值.
    // 一般在 stoptheworld/freezetheworld 时会将其设置为1, starttheworld 则会设置为0.
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
	// sysmonwait 用于表示 sysmon 线程是否处于休眠状态.
	//
	// 只有 sysmon() 会将该字段设置为 1(true).
	// 
	// 在监测到处于 gc 过程中, 或是所有 p 都空闲时, 设置该值为 1, 
	// 然后将自己挂起, 主动休眠. 直到有新的任务执行时, 才被唤醒.
	uint32	sysmonwait;
	Note	sysmonnote;
	uint64	lastpoll; // 这个值是一个时间值, 纳秒nanotime()

	// cpu数据采集的速率...hz是表示赫兹吗...
	//
	// cpu profiling rate
	int32	profilehz;
};

Sched	runtime·sched;

// 固定值. 开发者通过 GOMAXPROCS 可设置的 P 的最大值, 超过该值不会报错, 而是降低到这个值.
//
// The max value of GOMAXPROCS.
// There are no fundamental restrictions on the value.
enum { MaxGomaxprocs = 1<<8 };

// 在 runtime·gomaxprocsfunc(n)中设置为n
// 一般用来全局存储 procresize(n) 中设置的n值.
int32	newprocs;
// 所谓的主线程, 全局唯一, 具有特殊性.
// 1. 主线程的线程 tid 即是进程 pid, 而子线程的 tid 则不一样.
// 2. 主线程才能接收并处理来自控制台终端的 signal 信号.
// 3. gc时, STW 完成后, 只剩下 m0, 之后实际的 gc 行为也是由 m0 发起.
M	runtime·m0;
G	runtime·g0;	 // idle goroutine for m0

////////////////////////////////////////////////////////////////////////////////

// static 函数只能用于源文件自身, 要实现 proc_XXX.c 文件的函数相互调用, 
// 则需要将被调函数的 static 标记移除.

void runtime·mstart(void);
G* runqget(P*);
G* runqsteal(P*, P*);
M* mget(void);
void mcommoninit(M*);
void schedule(void);
void acquirep(P*);
P* releasep(void);
void newm(void(*)(void), P*);
void stopm(void);
void startm(P*, bool);
void wakep(void);
void stoplockedm(void);
void checkdead(void);
void gfpurge(P*);
void globrunqput(G*);
G* globrunqget(P*, int32);
P* pidleget(void);
void pidleput(P*);
bool preemptall(void);
void sysmon(void);
void handoffp(P*);
void injectglist(G*);
void incidlelocked(int32);
void mhelpgc(void);
bool needaddgcproc(void);
void procresize(int32);
void runqput(P*, G*);
G* findrunnable(void);
void runtime·main(void);
void execute(G *gp);
void gcstopm(void);
M* runtime·allocm(P *p);

static void runqgrow(P*);
static void mput(M*);
static void startlockedm(G*);
static uint32 retake(int64);
static void exitsyscall0(G*);
static void park0(G*);
static void goexit0(G*);
static void gfput(P*, G*);
static G* gfget(P*);
static bool preemptone(P*);
static bool exitsyscallfast(void);
static bool haveexperiment(int8*);
