/*
 * proc_stw_gc.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "stack.h"
#include "arch_amd64.h"
#include "malloc.h"

// Similar to stoptheworld but best-effort 
// and can be called several times.
// There is no reverse operation, used during crashing.
// This function must not lock any mutexes.
void runtime·freezetheworld(void)
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

// caller: 
// 	1. src/pkg/runtime/mgc0.c -> runtime·gc()
// 	开始进行 gc 前, 先调用本函数进入 STW 阶段
// 	2. src/pkg/runtime/mgc0.c -> runtime·ReadMemStats()
// 	3. proc.c -> runtime·gomaxprocsfunc()
void runtime·stoptheworld(void)
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

// caller: 
// 	1. runtime·starttheworld() 只有这一处调用.
void mhelpgc(void)
{
	m->helpgc = -1;
}

// 是否需要增加新的 proc 以执行gc.
// 标准就是判断当前处于 midle 的m是否大于0
// 因为只要大于0
// 由于只有一处调用, 可以认为被调用时只有一个m在运行, 其他都在休眠.
//
// caller: 
// 	1. runtime·starttheworld() 只有这一处
bool needaddgcproc(void)
{
	int32 n;

	runtime·lock(&runtime·sched);
	
	n = runtime·gomaxprocs;
	if(n > runtime·ncpu) {
		n = runtime·ncpu;
	} 
	if(n > MaxGcproc) {
		n = MaxGcproc;
	} 
	
	// one M is currently running
	// 在被调用时, 只有1个m处于运行状态, 其他都是 midle
	// 将这个 m 移除再比较
	n -= runtime·sched.nmidle+1; 
	runtime·unlock(&runtime·sched);
	return n > 0;
}

void runtime·starttheworld(void)
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
	} else {
		procresize(runtime·gomaxprocs);
	}
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
			if(mp->nextp) {
				runtime·throw("starttheworld: inconsistent mp->nextp");
			} 
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
	if(m->locks == 0 && g->preempt) {
		g->stackguard0 = StackPreempt;
	} 
}

// 确定参与并⾏回收的线程数量 = min(GOMAXPROCS, ncpu, MaxGcproc)
//
// caller: 
// 	1. mgc0.c -> gc() 只有这一处
int32 runtime·gcprocs(void)
{
	int32 n;

	// 确定可用于GC操作的CPU数量, 三者中取最小值.
	//
	// Figure out how many CPUs to use during GC.
	// Limited by gomaxprocs, number of actual CPUs, and MaxGcproc.
	runtime·lock(&runtime·sched);

	n = runtime·gomaxprocs;
	if(n > runtime·ncpu) {
		n = runtime·ncpu;
	}
	if(n > MaxGcproc) {
		n = MaxGcproc;
	}

	// 在被调用时, 只有1个m处于运行状态, 其他都是 midle.
	// 如果 n > nmidle+1, 其实是超出了当前创建了的m的总量.
	// 最多是让所有m都去执行gc嘛.
	//
	// one M is currently running
	if(n > runtime·sched.nmidle+1) {
		n = runtime·sched.nmidle+1;
	}
	runtime·unlock(&runtime·sched);
	return n;
}

// 从调度器中获取 nproc-1 个 m 对象, 为ta们分配 helpgc 的任务.
//
// 	@param nproc: 执行 gc 的协程数量.
//
// caller: 
// 	1. mgc0.c -> gc() 只有这一处
// 	当执行gc操作的协程数量大于1时调用此函数(此时处于 STW 阶段, 只有1个 M 在运行).
// 	不过此时还没有开始, 要等 src/pkg/runtime/mgc0.c -> gchelperstart() 后,
// 	才会正式执行 gc 任务.
void runtime·helpgc(int32 nproc)
{
	M *mp;
	int32 n, pos;

	runtime·lock(&runtime·sched);
	pos = 0;
	// one M is currently running
	for(n = 1; n < nproc; n++) { 
		// m 中的 mcache 用的其实是ta绑定的 p 的 mcache
		// 如果这个if条件成立, 说明遍历到了当前m的p对象, 需要跳过.
		if(runtime·allp[pos]->mcache == m->mcache) {
			pos++;
		}

		// 从 sched 获取一个空闲的 m 对象, 设置其 helpgc 序号, 并且与 allp[pos] 绑定.
		// 这些 m 都将执行 gc 的任务(标记, 清除).
		mp = mget();
		if(mp == nil) {
			// inconsistency 不一致
			runtime·throw("runtime·gcprocs inconsistency");
		}
		// 设置当前 m 的 helpgc 序号, 表示当前 m 在所有参与 gc 的线程列表中的索引.
		mp->helpgc = n;
		// 为什么只绑定 mcache? 不使用 acquirep() 完成?
		mp->mcache = runtime·allp[pos]->mcache;
		pos++;
		runtime·notewakeup(&mp->park);
	}
	runtime·unlock(&runtime·sched);
}
