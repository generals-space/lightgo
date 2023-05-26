/*
 * proc_p.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "arch_amd64.h"
#include "malloc.h"

// 修改 p 对象的数量为 new, 不足的补上, 多余的丢掉(主要是修改 runtime·gomaxprocs 的值).
//
// 	@param new: [1, MaxGomaxprocs]
//
// caller: 
// 	1. runtime·schedinit() 为 runtime·allp 数组申请完空间后, 调用此函数将 P 数量调整到合适值.
// 	2. runtime·starttheworld()
//
// 话说, 按照这两个调用函数的时机, 似乎不会存在动态调整p数量的情况?
// 还是说每次调整p数量, 是要在本轮gc结束后的 starttheworld() ?
//
// Change number of processors. 
// The world is stopped, sched is locked.
void procresize(int32 new)
{
	int32 i, old;
	G *gp;
	P *p;
	// 获取此次调用之前的 runtime·gomaxprocs 的值
	old = runtime·gomaxprocs;
	if(old < 0 || old > MaxGomaxprocs || new <= 0 || new > MaxGomaxprocs) {
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
		while(gp = runqget(p)) {
			globrunqput(gp);
		} 
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

	if(m->p) {
		m->p->m = nil;
	} 

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

// 将目标 p 对象与当前 m 关联(只是修改了一下m->p的指向), 要求 p 的状态为 idle
//
// caller:
// 	1. procresize()
// 	2. runtime·mstart() 一个新创建出来的 m 对象, 调用该方法先绑定一个 p 对象, 
// 	之后会再调用 schedule() 方法去寻找可执行的任务.
//
// Associate p and the current m.
void acquirep(P *p)
{
	if(m->p || m->mcache) {
		runtime·throw("acquirep: already in go");
	}
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
P* releasep(void)
{
	P *p;

	if(m->p == nil || m->mcache == nil) {
		runtime·throw("releasep: invalid arg");
	}
	p = m->p;
	if(p->m != m || p->mcache != m->mcache || p->status != Prunning) {
		runtime·printf(
			"releasep: m=%p m->p=%p p->m=%p m->mcache=%p p->mcache=%p p->status=%d\n",
			m, m->p, p->m, m->mcache, p->mcache, p->status
		);
		runtime·throw("releasep: invalid p state");
	}
	m->p = nil;
	m->mcache = nil;
	p->m = nil;
	p->status = Pidle;
	return p;
}

// Put gp on the global runnable queue.
// Sched must be locked.
void globrunqput(G *gp)
{
	gp->schedlink = nil;
	if(runtime·sched.runqtail) {
		runtime·sched.runqtail->schedlink = gp;
	}
	else {
		runtime·sched.runqhead = gp;
	}
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
G* globrunqget(P *p, int32 max)
{
	G *gp, *gp1;
	int32 n;

	// ...全局待运行队列为0
	if(runtime·sched.runqsize == 0) {
		return nil;
	} 
	// ...要把runq中的任务平均分给每个p吗
	n = runtime·sched.runqsize/runtime·gomaxprocs+1;
	// 下面这种情况应该是gomaxprocs = 1时吧
	if(n > runtime·sched.runqsize) {
		n = runtime·sched.runqsize;
	} 

	if(max > 0 && n > max) {
		n = max;
	} 
	runtime·sched.runqsize -= n;

	if(runtime·sched.runqsize == 0) {
		runtime·sched.runqtail = nil;
	} 
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
void pidleput(P *p)
{
	p->link = runtime·sched.pidle;
	runtime·sched.pidle = p;
	// TODO: fast atomic
	runtime·xadd(&runtime·sched.npidle, 1); 
}

// Try get a p from pidle list.
// Sched must be locked.
// 从全局任务队列中获取并返回一个p对象
P* pidleget(void)
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
void runqput(P *p, G *gp)
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
G* runqget(P *p)
{
	G *gp;
	int32 t, h, s;
	// 本地待执行队列为空, 返回nil
	if(p->runqhead == p->runqtail) {
		return nil;
	} 

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
	if(h == s) {
		h = 0;
	} 

	p->runqhead = h;
	runtime·unlock(p);
	return gp;
}

// Grow local runnable queue.
// TODO(dvyukov): consider using fixed-size array
// and transfer excess to the global list 
// (local queue can grow way too big).
static void runqgrow(P *p)
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
G* runqsteal(P *p, P *p2)
{
	G *gp, *gp1;
	int32 t, h, s, t2, h2, s2, c, i;

	if(p2->runqhead == p2->runqtail) {
		return nil;
	}
	// sort locks to prevent deadlocks
	if(p < p2) {
		runtime·lock(p);
	}
	runtime·lock(p2);
	if(p2->runqhead == p2->runqtail) {
		runtime·unlock(p2);
		if(p < p2) {
			runtime·unlock(p);
		}
		return nil;
	}
	if(p >= p2) {
		runtime·lock(p);
	}
	// now we've locked both queues and know the victim is not empty
	h = p->runqhead;
	t = p->runqtail;
	s = p->runqsize;
	h2 = p2->runqhead;
	t2 = p2->runqtail;
	s2 = p2->runqsize;
	gp = p2->runq[h2++];  // return value
	if(h2 == s2) {
		h2 = 0;
	}
	// steal roughly half
	if(t2 > h2) {
		c = (t2 - h2) / 2;
	}
	else {
		c = (s2 - h2 + t2) / 2;
	}
	// copy
	for(i = 0; i != c; i++) {
		// the target queue is full?
		if(t == h-1 || (h == 0 && t == s-1)) {
			break;
		}
		// the victim queue is empty?
		if(t2 == h2) {
			break;
		}
		gp1 = p2->runq[h2++];
		if(h2 == s2) {
			h2 = 0;
		}
		p->runq[t++] = gp1;
		if(t == s) {
			t = 0;
		}
	}
	p->runqtail = t;
	p2->runqhead = h2;
	runtime·unlock(p2);
	runtime·unlock(p);
	return gp;
}
