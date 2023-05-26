/*
 * proc_sysmon_preempt.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "stack.h"

// 监控线程(无限循环, 直到进程结束).
//
// 1. 通过 netpoll 不断监测是否有 ready 的 g 对象, 如果有, 则将ta们放回到全局队列, 等待执行;
// 2. 收回长时间处于 Psyscall 状态的 P
// 3. 向长时间运行的 g 发出抢占通知(其实除了 sysmon, 调度器在很多地方都会设置抢占标志)
//
// 监控线程也有可能休眠, 比如在 gc 时, 或是所有 p 都空闲的时候, 因为这些时候没有监控的必要.
//
// caller: 
// 	1. runtime·main() 在执行用户代码前, 启动独立线程执行此函数.
//
// 注意: 此函数并不是以 g 的形式执行(因此也不需要 p), 而是由 m 直接执行, 直接绑定一个系统线程.
void sysmon(void)
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
			// 首次启动, 设置 delay 为 20
			//
			// start with 20us sleep...
			delay = 20; 
		} else if(idle > 50) {
			// idle 每次循环就加1, 即 20us * 50 = 1ms 后,
			// delay 的值每次循环就乘以2, 不过有上限
			//
			// start doubling the sleep after 1ms...
			delay *= 2;
		}

		// delay 最大为 10ms
		//
		// up to 10ms
		if(delay > 10*1000) {
			delay = 10*1000;
		} 
		// runtime·usleep 是汇编代码, 实际是 select 的系统调用. 
		runtime·usleep(delay);

		// 当进程处于 gc 流程中, 或是所有的 p 都处于 idle 状态时, 也没有必要监控了
		// (反正大家都没在干活...)
		// sysmon 就会自行休眠, 直到其他地方获取了一个空闲的 p, 重新开始参与任务的执行时再被唤醒.
		//
		// schedtrace <= 0, 即开发者并未通过 GODEBUG 变量开启调试日志.
		// 否则, 就算满足了上面的条件, 也不能休眠...
		if(runtime·debug.schedtrace <= 0 &&
			(runtime·sched.gcwaiting || 
				runtime·atomicload(&runtime·sched.npidle) == runtime·gomaxprocs)) { 
			// TODO: fast atomic
			runtime·lock(&runtime·sched);
			// ...再次检查, 而且这次还是原子地获取 gcwaiting 的值
			// 注意此处的 if 条件与上面的完全一样.
			if(runtime·atomicload(&runtime·sched.gcwaiting) || 
				runtime·atomicload(&runtime·sched.npidle) == runtime·gomaxprocs) {
				// 只有这一个地方将 sysmonwait 设置为 1(true)
				runtime·atomicstore(&runtime·sched.sysmonwait, 1);
				runtime·unlock(&runtime·sched);
				// 陷入休眠, 等待被唤醒, 因为目前没有监控的必要.
				// 在 starttheworld, exitsyscallfast 中会为空闲的 p 赋予新的任务, 
				// 于是被唤醒, 重新开始监控流程.
				// 当然, 每一次重开监控, idle 和 delay 的值都会归零重新计算.
				runtime·notesleep(&runtime·sched.sysmonnote);
				// 从休眠中醒来, 重置锁对象(这样下次还可以在该对象上加锁)
				runtime·noteclear(&runtime·sched.sysmonnote);
				// 一次休眠然后被唤醒后, 重置 idle 和 delay
				idle = 0;
				delay = 20;
			} else{
				// 这里相当于什么都没做, 就是说如果有p正在做事, 
				runtime·unlock(&runtime·sched);
			}
		}
		////////////////////////////////////////////////////////////////////////
		// 如下才是正式流程

		// 如果距上次循环超过了 10ms
		//
		// poll network if not polled for more than 10ms
		lastpoll = runtime·atomicload64(&runtime·sched.lastpoll);
		now = runtime·nanotime();
		// if 条件成立, 则表示到了下一次循环的时间了.
		if(lastpoll != 0 && lastpoll + 10*1000*1000 < now) {
			runtime·cas64(&runtime·sched.lastpoll, lastpoll, now);
			// 非阻塞行为
			// 看看是不是有陷入 recv/send 等网络系统调用的 g 任务, 已经因为网络IO事件而
			// 被唤醒了, 获取ta们的信息, 并将他们放到任务队列, 继续执行.
			//
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
				// 先将处于 locked 状态的 m 的数量减1(假设 m 的数量大于1)
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
		// retake 回收陷入系统调用的 p 对象, 同时也抢占运行时间比较长的 g 任务.
		//
		// retake P's blocked in syscalls and preempt long running G's
		if(retake(now)){
			idle = 0;
		} else {
			idle++;
		}
		// 这一段与 GODEBUG 调试有关, 并不是调度逻辑
		if(runtime·debug.schedtrace > 0 && lasttrace + runtime·debug.schedtrace*1000000ll <= now) {
			lasttrace = now;
			runtime·schedtrace(runtime·debug.scheddetail);
		}
	} // for {} 循环结束
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

// retake 回收陷入系统调用的 p 对象, 同时也可能抢占运行时间比较长的 g 任务.
// (防止某一个任务占用CPU太久影响其他任务的执行)
//
// 	@param now: 为绝对时间 nanotime
//
// 返回值 n 为
// 注意对 running 状态的 p 的 g 任务的抢占并不计在 n 中, 因为抢占操作只是一个通知,
// 实际当 p 不再运行 g, 需要等待到下一次调度.
//
// caller: 
// 	1. sysmon() 只有这一处
static uint32 retake(int64 now)
{
	uint32 i, s, n;
	int64 t;
	P *p;
	Pdesc *pd;

	n = 0;
	// 遍历调度器中的所有 p 对象
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p == nil) {
			continue;
		}
		pd = &pdesc[i];
		s = p->status;
		if(s == Psyscall) {
			// Retake P from syscall if it's there for more than 1 sysmon tick (20us).
			// But only if there is other work to do.
			t = p->syscalltick;
			// if 条件为真, 说明是这个 p 对应的 pd 第一次被调用, 就当作初始化了
			if(pd->syscalltick != t) {
				pd->syscalltick = t;
				pd->syscallwhen = now;
				continue;
			}
			// 如果该 p 的本地任务队列只有一个成员,
			// 且有m对象处于 spinning (没活干) 或是 p 对象处于 idle 状态
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
			// 如果该 p 被调度处于 running 状态超过了 10ms, 就抢占ta的 g 对象, 
			// 以免对单个任务陷入长时间运行, 而造成其他任务没有机会执行的问题.

			// 注意: 对 running 状态的 p 的抢占并不会影响本次 retake() 执行的结果
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

// 告诉所有的正在运行的goroutines(其实遍历的是 p 对象链表), ta们被抢占了, 需要停止.
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
bool preemptall(void)
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
static bool preemptone(P *p)
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

// 开发者通过 GODEBUG=schedtrace=1000 执行程序时生效的方法, 打印出当前GMP调度信息.
//
// param detailed: 添加有 GODEBUG=scheddetail=1 参数时为 true, 否则默认为 false.
//
// 注意: GODEBUG=gctrace=1 在 src/pkg/runtime/mgc0.c -> gc() 方法中实现
//
// caller:
// 	1. sysmon()
// 	2. src/pkg/runtime/panic.c -> runtime·startpanic()
void runtime·schedtrace(bool detailed)
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
	
	// 以下分别对 P M G 列表进行遍历.
	
	// We must be careful while reading data from P's, M's and G's.
	// Even if we hold schedlock, most data can be changed concurrently.
	// E.g. (p->m ? p->m->id : -1) can crash if p->m changes from non-nil to nil.
	for(i = 0; i < runtime·gomaxprocs; i++) {
		p = runtime·allp[i];
		if(p == nil) {
			continue;
		} 
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
