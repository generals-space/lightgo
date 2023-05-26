/*
 * proc_m.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "type.h"
#include "stack.h"
#include "arch_amd64.h"
#include "malloc.h"

// 检查 m 总数量(默认10000), 超出会引发进程崩溃.
// 其实就是检测 runtime·sched.mcount 成员字段的值.
// 主调函数需保证已获取 sched 对象锁.
//
// caller:
// 	1. mcommoninit()
// 	2. runtime∕debug·setMaxThreads()
static void checkmcount(void)
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

//  @param mp: 当主调函数为 runtime·schedinit() 时, mp 为 m0 全局对象, 即主线程;
//  当主调函数为 runtime·allocm() 时, mp 一个刚通过 cnew 创建的空对象.
//
// caller: 
// 	1. runtime·schedinit() 进程启动, runtime 运行时初始化时被调用.
// 	2. runtime·allocm()
void mcommoninit(M *mp)
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

//
// caller:
// 	1. src/pkg/runtime/asm_amd64.s -> _rt0_go() 
//     程序启动入口, 开始运行主协程.
// 	2. src/pkg/runtime/os_linux.c -> runtime·newosproc()
//     创建新 m 对象时, 底层关联系统线程, 并通过 clone 系统调用执行本函数.
//
// Called to start an M.
void runtime·mstart(void)
{
	if(g != m->g0) {
		runtime·throw("bad runtime·mstart");
	}

	// Record top of stack for use by mcall.
	// Once we call schedule we're never coming back,
	// so other calls can reuse this stack space.
	runtime·gosave(&m->g0->sched);
	// make sure it is never used
	m->g0->sched.pc = (uintptr)-1; 
	// cgo sets only stackguard0, copy it to stackguard
	m->g0->stackguard = m->g0->stackguard0; 

	runtime·asminit();
	runtime·minit();

	// Install signal handlers; after minit so that minit can
	// prepare the thread to be able to handle the signals.
	// 装载信号处理函数
	if(m == &runtime·m0) {
		runtime·initsig();
	}
	// mstart 被 newm 调用时可能会提供 mstartfn 函数
	// 执行完成目标 fn 后, 该 m 会加入到 g 的抢夺队列(schedule).
	if(m->mstartfn) {
		m->mstartfn();
	} 

	if(m->helpgc) {
		// 对于 helpgc 的赋值, 有如下几种可能: 
		// 1. 如果是一个常规 clone 出来的新线程, helpgc 应该为0.
		// 如果 helpgc 不为 0, 表示其父进程之前是参与过 gc 的, 这里需要将其重置.
		// 感觉有点不严谨, 为什么不在 gc 结束时将其重置???
		//
		// 2. 在 gc 过程中进行 clone...好像更不对啊.
		m->helpgc = 0;
		stopm();
	} else if(m != &runtime·m0) {
		// 这里的 nextp 是在调用 newm() 创建一个 m 对象时(还未创建系统线程)传入的.
		// 如果 m 对象是用来运行 sysmon(), 或 mhelpgc() 函数的, 其实不需要绑定 p 对象.
		// 否则, 为了执行开发者级别的协程任务, 那么创建 m 时, 必然要求能够拿到 p 对象.
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
M* runtime·allocm(P *p)
{
	M *mp;
	// The Go type M
	static Type *mtype; 

	// disable GC because it can be called from sysmon
	m->locks++; 
	// temporarily borrow p for mallocs in this function
	if(m->p == nil) {
		acquirep(p); 
	}

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

	if(p == m->p) {
		releasep();
	} 

	m->locks--;
	// restore the preemption request 
	// in case we've cleared it in newstack
	if(m->locks == 0 && g->preempt) {
		g->stackguard0 = StackPreempt;
	}

	return mp;
}

// 创建一个 m 对象, 绑定目标 p 对象, 并执行 fn.
//
// 	@param fn: 目标 m 在启动时要执行的函数. 可为 sysmon(), mhelpgc(), 也可以为 nil.
// 	@param p: 新 m 需要绑定的 p 对象.
//
// 当 fn 是 sysmon(), mhelpgc() 函数时, p 为 nil, 这两个函数不需要绑定 p 就可以执行.
// 只有 fn = nil 时, 新 m 才是为了运行开发者级别的协程任务的.
// 而在这种场景下, 必然是因为 m 不够了, p 存在空闲了, 才需要新增一个 m 出来.
//
// caller:
// 	1. runtime·main() runtime 初始化完成后, 启动主协程 g0, 
//     并由 g0 调用此函数创建监控线程, 运⾏调度器监控.
// 	2. runtime·starttheworld()
// 	3. startm()
//
// Create a new m. 
// It will start off with a call to fn, or else the scheduler.
// 
void newm(void(*fn)(void), P *p)
{
	M *mp;

	mp = runtime·allocm(p);
	mp->nextp = p;
	mp->mstartfn = fn;

	if(runtime·iscgo) {
		CgoThreadStart ts;

		if(_cgo_thread_start == nil) {
			runtime·throw("_cgo_thread_start missing");
		}
		ts.m = mp;
		ts.g = mp->g0;
		ts.fn = runtime·mstart;
		runtime·asmcgocall(_cgo_thread_start, &ts);
		return;
	}
	// 调用 clone 系统调用创建新的系统线程
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
void stopm(void)
{
	if(m->locks) {
		runtime·throw("stopm holding locks");
	} 
	if(m->p) {
		runtime·throw("stopm holding p");
	} 
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

// mspinning 
//
// caller: 
// 	1. startm() 只有这一处
static void mspinning(void)
{
	m->spinning = true;
}

// 调度一些 m 对象以运行目标 p, 如果 m 不足则创建.
// 如果 p == nil, 尝试获取一个空闲的 p 对象.
//
// 如果获取不到, 则返回 false...这个函数好像没有返回值吧???
//
// caller:
// 	1. handoffp()
// 	2. wakep()
// 	3. injectglist()
//
// Schedules some M to run the p (creates an M if necessary).
// If p==nil, tries to get an idle P, if no idle P's returns false.
void startm(P *p, bool spinning)
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
			if(spinning) {
				runtime·xadd(&runtime·sched.nmspinning, -1);
			} 
			return;
		}
	}

	mp = mget();
	runtime·unlock(&runtime·sched);

	// 如果没能获取到 m, 则创建新的 m 以运行目标 p
	if(mp == nil) {
		fn = nil;
		if(spinning) {
			fn = mspinning;
		}
		newm(fn, p);
		return;
	}
	// 从全局调度器中获取到了一个 m
	if(mp->spinning) {
		runtime·throw("startm: m is spinning");
	} 
	if(mp->nextp) {
		runtime·throw("startm: m has p");
	} 
	mp->spinning = spinning;
	mp->nextp = p;
	runtime·notewakeup(&mp->park);
}

// 将 mp 放到全局调度器的 midle 链表头部, 并将其标记为 idle.
//
// 调用此函数时必须对sched全局调度器加锁
//
// Put mp on midle list.
// Sched must be locked.
static void mput(M *mp)
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
M* mget(void)
{
	M *mp;

	if((mp = runtime·sched.midle) != nil){
		runtime·sched.midle = mp->schedlink;
		runtime·sched.nmidle--;
	}
	return mp;
}

// golang原生: runtime/debug.SetMaxThreads()
//
// 设置 golang runtime 底层可使用的系统线程(thread)数, maxmcount 默认值为 10000.
void runtime∕debug·setMaxThreads(intgo in, intgo out)
{
	runtime·lock(&runtime·sched);
	out = runtime·sched.maxmcount;
	runtime·sched.maxmcount = in;
	checkmcount();
	runtime·unlock(&runtime·sched);
	FLUSH(&out);
}
