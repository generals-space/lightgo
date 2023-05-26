/*
 * proc_syscall.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "stack.h"
#include "../../cmd/ld/textflag.h"

// 设置当前 g 对象的 g->sched 信息, 即当前正在执行的函数的主调函数 pc 和 sp 等信息.
//
// caller: 
// 	1. ·entersyscall() m 在执行 g 任务时, 陷入系统调用前会事先调用此函数保存现场信息,
//     之后 m 会切换执行其他 g 任务(切换的是 p, 还是同一个 p 的其他本地任务).
// 	2. ·entersyscallblock()
//
// 主要还是用于从系统调用返回时重新唤醒主调函数的吧, sp 用于定位局部变量
// 注意: 这里的sched存放的主调函数信息其实是进行syscall的函数, 用于返回的.
#pragma textflag NOSPLIT
static void save(void *pc, uintptr sp)
{
	g->sched.pc = (uintptr)pc;
	g->sched.sp = sp; 
	g->sched.lr = 0;
	g->sched.ret = 0;
	g->sched.ctxt = 0;
	g->sched.g = g;
}

// 协程对象 g 将要陷入系统调用, 这里记录 ta 将不再占用 cpu.
// 此函数只由 golang 的 syscall 库和 cgocall 函数调用, 而不是 runtime 的其他底层系统调用.
// 
// entersyscall 不能被分割栈, runtime·gosave() 必定会将
// g->sched 赋值为调用方的栈段, 因为 entersyscall 将会马上返回.
//
// 	@param dummy: 应该是主调函数的函数名称所在地址, 这是一个不确定的值.
// 主要就设置了一个 p 的状态为 Psyscall, 解绑 m与p, 然后标记为当前 g 对象被抢占, 好像就没了?
//
// caller: 
// 	1. 大都是汇编的调用, 即各平台的 Syscall(SB) 函数.
// 	2. runtime·cgocall()
// 	3. runtime·cgocallbackg()
//
// The goroutine g is about to enter a system call.
// Record that it's not using the cpu anymore.
// This is called only from the go syscall library and cgocall,
// not from the low-level system calls used by the runtime.
//
// Entersyscall cannot split the stack: the runtime·gosave must
// make g->sched refer to the caller's stack segment, 
// because entersyscall is going to return immediately after.
//
#pragma textflag NOSPLIT
void ·entersyscall(int32 dummy)
{
	// 禁止抢占. 因为在此函数中 g 将处于(或者说被修改为) Gsyscall 状态,
	// 但可能会出现 g->sched 不一致的情况, 别让gc线程发现这里.
	//
	// Disable preemption because during this function 
	// g is in Gsyscall status,
	// but can have inconsistent g->sched, do not let GC observe it.
	m->locks++;

	// 保留SP用于GC和回溯
	//
	// Leave SP around for GC and traceback.
	save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));

	g->syscallsp = g->sched.sp;
	g->syscallpc = g->sched.pc;
	g->syscallstack = g->stackbase;
	g->syscallguard = g->stackguard;
	g->status = Gsyscall;

	// 这里 if 语句的第一个条件是判断栈溢出的, 可参考如下函数中的 overflow 检查:
	// stack.c -> runtime·newstack() 
	// 至于第二个条件, 由于 g->stackbase 本来就表示
	if(g->syscallsp < g->syscallguard-StackGuard || g->syscallstack < g->syscallsp) {
		// runtime·printf("entersyscall inconsistent %p [%p,%p]\n",
		//	g->syscallsp, g->syscallguard-StackGuard, g->syscallstack);
		runtime·throw("entersyscall");
	}

	// TODO: fast atomic
	// 只有 sysmon() 这一个地方将 sysmonwait 设置为1, 
	// 然后在 sysmonnote 陷入休眠(当没有任务需要监控的时候)
	// 现在任务来了, 唤醒ta吧.
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

	// 这里 m 单方面解除了 p 与自己的绑定关系, 在系统调用期间, m->p 可能会被 sysmon 线程拿走.
	// 但是 m 仍然保留了 p, 因为ta在 runtime·exitsyscall 中, 还"奢望"着自己从系统调用结束后,
	// 还能继续使用这个 p.
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
		if (runtime·sched.stopwait > 0 && runtime·cas(&m->p->status, Psyscall, Pgcstop)) {
			if(--runtime·sched.stopwait == 0) {
				runtime·notewakeup(&runtime·sched.stopnote);
			}
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

// 与 runtime·entersyscall() 相同, 但这里的系统调用是阻塞的.
//
// caller: 
// 	1. src/pkg/runtime/lock_futex.c -> runtime·notetsleepg() 只有这一处.
//
// The same as runtime·entersyscall(), 
// but with a hint that the syscall is blocking.
#pragma textflag NOSPLIT
void ·entersyscallblock(int32 dummy)
{
	P *p;

	// see comment in entersyscall
	m->locks++;

	// 保留SP用于GC和回溯
	//
	// Leave SP around for GC and traceback.
	save(runtime·getcallerpc(&dummy), runtime·getcallersp(&dummy));

	g->syscallsp = g->sched.sp;
	g->syscallpc = g->sched.pc;
	g->syscallstack = g->stackbase;
	g->syscallguard = g->stackguard;
	g->status = Gsyscall;

	if(g->syscallsp < g->syscallguard-StackGuard || g->syscallstack < g->syscallsp) {
		// runtime·printf("entersyscall inconsistent %p [%p,%p]\n",
		//	g->syscallsp, g->syscallguard-StackGuard, g->syscallstack);
		runtime·throw("entersyscallblock");
	}
	// 直到这里, 仍与 ·entersyscall 保持一致, 区别在下半部分.

	p = releasep();
	// 把 p 移交到新的 m, 而当前 m 则与 g 任务仍绑定在一起.
	// 即 m 阻塞在这里等 g 任务从 syscall 中返回, 而让 p 则去寻找其他的 m 继续发挥作用.
	handoffp(p);
	// do not consider blocked scavenger for deadlock detection
	if(g->isbackground) {
		incidlelocked(1);
	}

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
void runtime·exitsyscall(void)
{
	m->locks++;  // see comment in entersyscall

	// do not consider blocked scavenger for deadlock detection
	if(g->isbackground) {
		incidlelocked(-1);
	}

	// exitsyscallfast 为 m 重新绑定 p 对象, 绑定成功则为true.
	// 注意, 进入此 if 块后必定会 return
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
	// 自己没有 p 对象, 无法再继续 g 任务中系统调用之后的操作了.
	// 但是 g 不能不运行, 于是将自己的 g 对象放回到全局队列, 自己则陷入休眠.
	//
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
static bool exitsyscallfast(void)
{
	P *p;

	// Freezetheworld sets stopwait but does not retake P's.
	// 这是啥意思??? 
	if(runtime·sched.stopwait) {
		m->p = nil;
		return false;
	}

	// 1. 尝试绑定上一个p(即 m->p)
	// 在 m 处于系统调用期间, m->p 可能会被 sysmon 线程拿走, 
	// 此时 status 状态会从 Psyscall 变成其他的.
	//
	// Try to re-acquire the last P.
	if(m->p && m->p->status == Psyscall && 
		runtime·cas(&m->p->status, Psyscall, Prunning)) {
		// There's a cpu for us, so we can run.
		// 真的找回了这个cpu(就是 p 对象), 那么重新绑定
		m->mcache = m->p->mcache;
		m->p->m = m;
		return true;
	}

	// 2. m->p 被 sysmon 线程拿走了, 则尝试重新获取一个空闲的 p.
	//
	// Try to get any other idle P.
	m->p = nil;
	if(runtime·sched.pidle) {
		runtime·lock(&runtime·sched);
		p = pidleget();
		// sysmonwait 只有在sysmon()这一个函数中, 被设置为1, 然后陷入休眠.
		// 既然p有任务来了, sysmon 也没必要再休眠了.
		if(p && runtime·atomicload(&runtime·sched.sysmonwait)) {
			runtime·atomicstore(&runtime·sched.sysmonwait, 0);
			runtime·notewakeup(&runtime·sched.sysmonnote);
		}
		runtime·unlock(&runtime·sched);
		if(p) {
			// 把 p 与当前的 m 绑定
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
static void exitsyscall0(G *gp)
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
