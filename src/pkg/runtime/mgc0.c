// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Garbage collector.

#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "stack.h"
#include "mgc0.h"
#include "mgc0__stats.h"
#include "mgc0__funcs.h"
#include "race.h"
#include "type.h"
#include "typekind.h"
#include "funcdata.h"
#include "mgc0__others.h"

// Holding worldsema grants an M the right to try to stop the world.
// 哪个M对象获取到了worldsema, 谁都拥有STW的权力
// The procedure is:
//
//	runtime·semacquire(&runtime·worldsema);
//	m->gcing = 1;
//	runtime·stoptheworld();
//
//	... do stuff ...
//
//	m->gcing = 0;
//	runtime·semrelease(&runtime·worldsema);
//	runtime·starttheworld();
//
uint32 runtime·worldsema = 1;

static void runfinq(void);

extern struct GCSTATS gcstats;

struct WORK work;

// Structure of arguments passed to function gc().
// This allows the arguments to be passed via runtime·mcall.
// 传入gc()函数的的参数结构体(注意不是runtime·gc()),
// 这允许通过runtime·mcall()传递参数.
struct gc_args
{
	// start time of GC in ns (just before stoptheworld)
	int64 start_time; 
};

static void gc(struct gc_args *args);
static void mgc(G *gp);

static FuncVal runfinqv = {runfinq};
extern int32 gcpercent;

// runtime·gc 先进入 STW 阶段, 然后调用 runtime·mcall(mgc) 在 g0 上进行实际 gc 行为,
// 完成后, 再解除 STW, 重新启用调度器.
//
// 	@param force: 是否为强制 gc(其实应该叫例行 gc) 普通 gc 是看空间使用量, 到达一定比率开始执行.
// 	如果2分钟内没有被动 gc, 则进行主动的例行 gc.
//
// caller:
// 	1. src/pkg/runtime/malloc.goc -> runtime·mallocgc() force 参数 = false
// 	分配可gc对象的同时, 检查空间使用情况, 满足条件时调用本函数进行一次 gc.
// 	2. src/pkg/runtime/mheap.c -> forcegchelper() force 参数 = true
// 	2分钟内没有进行 gc 时, 则进行例行 gc(也叫强制 gc)
// 	在进程启动初期就创建一个协程完成这项工作(runtime·MHeap_Scavenger())
// 	3. runfinq()
// 	还没想通使用场景
// 	4. src/pkg/runtime/mheap.c -> runtime∕debug·freeOSMemory()
// 	调试使用
// 	5. GC() runtime 标准库中的 GC() 函数, 由开发者主动调用
void runtime·gc(int32 force)
{
	struct gc_args a;
	int32 i;

	// 在 empty/full 这两个 uint64 类型的成员没有在 uint64 边界上对齐时, 
	// 原子操作其实并不原子...
	// 这个问题在之前就出现了.
	// 7 == 0111
	// work对象在当前文件的开头定义(第200行左右)
	//
	// The atomic operations are not atomic if the uint64s
	// are not aligned on uint64 boundaries. 
	// This has been a problem in the past.
	if((((uintptr)&work.empty) & 7) != 0) {
		runtime·throw("runtime: gc work buffer is misaligned");
	}
	if((((uintptr)&work.full) & 7) != 0) {
		runtime·throw("runtime: gc work buffer is misaligned");
	}

	// 如下情况退出GC
	// 1. enablegc == 0 表示 runtime 还未启动完成, 该值在 runtime·schedinit() 末尾才被赋值为 1
	// 2. 很多持有lock锁的库中会调用malloc, 为了避免优先级混乱的问题, 在持有锁的函数中就不要调用gc了.
	// malloc下面的mallocgc可以在没有锁的情况下进行gc操作.
	//
	// The gc is turned off (via enablegc) until the bootstrap has completed.
	// Also, malloc gets called in the guts of a number of libraries
	// that might be holding locks. 
	// To avoid priority inversion problems, 
	// don't bother trying to run gc while holding a lock. 
	// The next mallocgc without a lock will do the gc instead.
	if(!mstats.enablegc || g == m->g0 || m->locks > 0 || runtime·panicking) {
		return;
	}

	// 首次调用(gcpercent 初始值就是 GcpercentUnknown)
	//
	// first time through
	if(gcpercent == GcpercentUnknown) {
		runtime·lock(&runtime·mheap);
		// 第一次执行, 调用 readgogc() 从环境变量中取 GOGC 设置. 
		// GOGC == off 时得到 -1, 其他的默认情况下返回 100
		if(gcpercent == GcpercentUnknown) {
			gcpercent = readgogc();
		} 

		runtime·unlock(&runtime·mheap);
	}
	// 当 GOGC == off 时, gcpercent 的值将取-1, 相当于关闭GC.
	if(gcpercent < 0) {
		return;
	} 

	// gc STW 前尝试获取全局锁
	runtime·semacquire(&runtime·worldsema, false);

	// 如果不是强制(例行) gc, 而此时还没有 next_gc 的时间时, 就退出了...
	// 这种情况适用于第1种主调函数
	if(!force && mstats.heap_alloc < mstats.next_gc) {
		// typically threads which lost the race to grab worldsema exit here when gc is done.
		runtime·semrelease(&runtime·worldsema);
		return;
	}

	// Ok, we're doing it! Stop everybody else
	a.start_time = runtime·nanotime();
	m->gcing = 1;
	runtime·stoptheworld();

	// GODEBUG="gotrace=2" 会引发两次回收
	//
	// Run gc on the g0 stack. 
	// We do this so that the g stack we're currently running on will no longer change.
	// Cuts the root set down a bit
	// (g0 stacks are not scanned, and we don't need to scan gc's internal state).
	// Also an enabler for copyable stacks.
	for(i = 0; i < (runtime·debug.gctrace > 1 ? 2 : 1); i++) {
		// 调用 runtime·mcall(mgc), 在 g0 上进行实际的 gc 操作.
		//
		// switch to g0, call gc(&a), then switch back
		g->param = &a;
		g->status = Gwaiting;
		g->waitreason = "garbage collection";
		// mgc上下面定义的函数名, 是实际执行GC的函数.
		// runtime·mcall 是汇编代码, 原型为void mcall(void (*fn)(G*))
		// 作用是切换到 m->g0 的上下文, 并调用mgc(g).
		// 注意参数g应该就是此处的局部变量g.
		runtime·mcall(mgc);
		// record a new start time in case we're going around again
		a.start_time = runtime·nanotime();
	}

	// gc 完成后.
	//
	// all done
	m->gcing = 0;
	m->locks++;
	// gc 后释放全局锁, 
	runtime·semrelease(&runtime·worldsema);
	runtime·starttheworld();
	m->locks--;

	// 现在 gc 已经完成, 如果有需要, 则启动 finalizer 线程.
	// finq 等待执行的finalizer链表
	//
	// now that gc is done, kick off finalizer thread if needed
	if(finq != nil) {
		runtime·lock(&finlock);
		// fing 用来执行finalizer的线程对象, 指针类型
		// 如果finq不空, 但fing为nil, 就创建一个G协程, 来运行runfinqv.
		// 而funfinqv其实就是runqinq函数.
		//
		// kick off or wake up goroutine to run queued finalizers
		if(fing == nil) {
			fing = runtime·newproc1(&runfinqv, nil, 0, 0, runtime·gc);
		}
		else if(fingwait) {
			// 如果fingwait, 表示此时fing中的g对象都为Gwaiting状态.
			// 这里设置其值为0, 并调用runtime·ready()将其转换为Grunnable状态.
			// ...runtime·newproc1()创建的g对象的状态就是Grunnable.
			fingwait = 0;
			// 不过runtime·ready()貌似是只设置单个g对象的状态, 难道fing就只有一个?
			runtime·ready(fing);
		}
		runtime·unlock(&finlock);
	}
	// 使已经在排队的 finalizer 有机会执行...
	// 话说 startworld 并不表示重新开始调度吗???
	//
	// give the queued finalizers, if any, a chance to run
	runtime·gosched();
}

// 	@param gp: 主调函数所在的 g 对象
//
// caller: 
// 	1. runtime·gc() 在进入 STW 阶段后, 调用 runtime·mcall(mgc) 在 g0 上执行该函数
static void mgc(G *gp)
{
	gc(gp->param);
	gp->param = nil;
	gp->status = Grunning;
	runtime·gogo(&gp->sched);
}

extern updatememstats(GCStats *stats);
extern void debug_scanblock(byte *b, uintptr n);
extern void markroot(ParFor *desc, uint32 i);
extern void sweepspan(ParFor *desc, uint32 idx);
extern void scanblock(Workbuf *wbuf, Obj *wp, uintptr nobj, bool keepworking);
static void	gchelperstart(void);

// 确定参与 gc 的协程数量, 然后调用 addroots() 添加根节点.
//
// caller:
// 	1. mgc() 只有这一处, mgc()则是由 runtime·gc()调用的, 该函数是在 g0 上执行的.
static void gc(struct gc_args *args)
{
	int64 t0, t1, t2, t3, t4;
	uint64 heap0, heap1, obj0, obj1, ninstr;
	GCStats stats;
	M *mp;
	uint32 i;
	Eface eface;

	t0 = args->start_time;

	// 清空 gcstats, 记录本次 gc 的数据
	if(CollectStats) {
		runtime·memclr((byte*)&gcstats, sizeof(gcstats));
	} 

	for(mp=runtime·allm; mp; mp=mp->alllink) {
		runtime·settype_flush(mp);
	}

	heap0 = 0;
	obj0 = 0;
	if(runtime·debug.gctrace) {
		updatememstats(nil);
		heap0 = mstats.heap_alloc;
		obj0 = mstats.nmalloc - mstats.nfree;
	}

	// ...这还要啥disable啊, 现在就是gc啊
	//
	// disable gc during mallocs in parforalloc
	m->locks++;
	if(work.markfor == nil) {
		work.markfor = runtime·parforalloc(MaxGcproc);
	}
	if(work.sweepfor == nil) {
		work.sweepfor = runtime·parforalloc(MaxGcproc);
	}
	m->locks--;

	if(itabtype == nil) {
		// get C pointer to the Go type "itab"
		runtime·gc_itab_ptr(&eface);
		itabtype = ((PtrType*)eface.type)->elem;
	}

	work.nwait = 0;
	work.ndone = 0;
	work.debugmarkdone = 0;
	// 确定并⾏回收的 goroutine 数量 = min(GOMAXPROCS, ncpu, MaxGcproc).
	work.nproc = runtime·gcprocs();
	addroots();
	// 将 work.markfor 成员的 body 字段, 设置为 markroot 函数
	runtime·parforsetup(work.markfor, work.nproc, work.nroot, nil, false, markroot);
	// 将 work.sweepfor 成员的 body 字段, 设置为 sweepspan 函数
	runtime·parforsetup(work.sweepfor, work.nproc, runtime·mheap.nspan, nil, true, sweepspan);
	// 如果执行 gc 操作的协程数量大于1, 那么在进行之后的操作之前, 需要保证这些协程全部完成才行.
	// 所以下面有 notesleep(alldone) 的语句, 而在 sleep 之前, 则需要使用 clear 对目标锁对象清零.
	if(work.nproc > 1) {
		runtime·noteclear(&work.alldone);
		runtime·helpgc(work.nproc);
	}

	t1 = runtime·nanotime();

	gchelperstart();
	// 并行 mark, 执行上面设置的 markroot() 函数
	runtime·parfordo(work.markfor);
	// ...这参数都是nil, 效果是啥???
	// 对了, 在parfordo中循环调用的 markfor, 也就是 markroot 函数中, 最后一句就是 scanblock.
	// 那么, 这里的scanblock...是查漏补缺的? 但好像也没什么实际作用啊.
	scanblock(nil, nil, 0, true);

	if(DebugMark) {
		for(i=0; i<work.nroot; i++){
			debug_scanblock(work.roots[i].p, work.roots[i].n);
		}
		runtime·atomicstore(&work.debugmarkdone, 1);
	}

	t2 = runtime·nanotime();

	// 并行 sweep, 执行上面设置的 sweepspan() 函数
	runtime·parfordo(work.sweepfor);
	bufferList[m->helpgc].busy = 0;

	t3 = runtime·nanotime();
	
	// 如果执行GC的协程数量多于1个, 那就等待直到ta们全部完成.
	// wakeup 的操作在 runtime·gchelper() 函数中.
	// ...如果只有1个就不用了? 只有一个协程时运行到这里说明sweepfor已经完成了是吗???
	if(work.nproc > 1) {
		runtime·notesleep(&work.alldone);
	}

	// 统计数据
	cachestats();
	// 计算下一次GC触发的时间...好像不能说是时间, 而是空间吧.
	// next_gc总是与heap_alloc进行比较, 应该是每增长百分之gcpercent就进行一次GC吧.
	mstats.next_gc = mstats.heap_alloc+mstats.heap_alloc*gcpercent/100;

	t4 = runtime·nanotime();
	mstats.last_gc = t4; // 设置最近一次gc的时间(绝对时间)
	mstats.pause_ns[mstats.numgc%nelem(mstats.pause_ns)] = t4 - t0;
	mstats.pause_total_ns += t4 - t0;
	mstats.numgc++; // gc次数加1
	if(mstats.debuggc){
		runtime·printf("pause %D\n", t4-t0);
	}

	// 打印gc调试信息
	// GODEBUG=gctrace=1 
	if(runtime·debug.gctrace) {
		updatememstats(&stats);
		heap1 = mstats.heap_alloc;
		obj1 = mstats.nmalloc - mstats.nfree;

		stats.nprocyield += work.sweepfor->nprocyield;
		stats.nosyield += work.sweepfor->nosyield;
		stats.nsleep += work.sweepfor->nsleep;

		runtime·printf(
			"gc%d(%d): %D+%D+%D ms, %D -> %D MB %D -> %D (%D-%D) objects,"
			" %D(%D) handoff, %D(%D) steal, %D/%D/%D yields\n",
			mstats.numgc, work.nproc, (t2-t1)/1000000, (t3-t2)/1000000, (t1-t0+t4-t3)/1000000,
			heap0>>20, heap1>>20, obj0, obj1, mstats.nmalloc, mstats.nfree,
			stats.nhandoff, stats.nhandoffcnt,
			work.sweepfor->nsteal, work.sweepfor->nstealcnt,
			stats.nprocyield, stats.nosyield, stats.nsleep
		);
		if(CollectStats) {
			runtime·printf(
				"scan: %D bytes, %D objects, %D untyped, %D types from MSpan\n",
				gcstats.nbytes, gcstats.obj.cnt, gcstats.obj.notype, gcstats.obj.typelookup
			);
			if(gcstats.ptr.cnt != 0) {
				runtime·printf(
					"avg ptrbufsize: %D (%D/%D)\n",
					gcstats.ptr.sum/gcstats.ptr.cnt, gcstats.ptr.sum, gcstats.ptr.cnt
				);
			}
			if(gcstats.obj.cnt != 0) {
				runtime·printf(
					"avg nobj: %D (%D/%D)\n",
					gcstats.obj.sum/gcstats.obj.cnt, gcstats.obj.sum, gcstats.obj.cnt
				);
			}
			runtime·printf(
				"rescans: %D, %D bytes\n", gcstats.rescan, gcstats.rescanbytes
			);

			runtime·printf("instruction counts:\n");
			ninstr = 0;
			for(i=0; i<nelem(gcstats.instr); i++) {
				runtime·printf("\t%d:\t%D\n", i, gcstats.instr[i]);
				ninstr += gcstats.instr[i];
			}
			runtime·printf("\ttotal:\t%D\n", ninstr);

			runtime·printf(
				"putempty: %D, getfull: %D\n", gcstats.putempty, gcstats.getfull
			);

			runtime·printf(
				"markonly base lookup: bit %D word %D span %D\n", 
				gcstats.markonly.foundbit, 
				gcstats.markonly.foundword, 
				gcstats.markonly.foundspan
			);
			runtime·printf(
				"flushptrbuf base lookup: bit %D word %D span %D\n", 
				gcstats.flushptrbuf.foundbit, 
				gcstats.flushptrbuf.foundword, 
				gcstats.flushptrbuf.foundspan
			);
		}
	}

	runtime·MProf_GC();
}

// caller:
// 	1. 这个函数的调用有点复杂
static void runfinq(void)
{
	Finalizer *f;
	FinBlock *fb, *next;
	byte *frame;
	uint32 framesz, framecap, i;
	Eface *ef, ef1;

	frame = nil;
	framecap = 0;
	for(;;) {
		runtime·lock(&finlock);
		fb = finq;
		finq = nil;
		if(fb == nil) {
			fingwait = 1;
			runtime·park(runtime·unlock, &finlock, "finalizer wait");
			continue;
		}
		runtime·unlock(&finlock);
		if(raceenabled) {
			runtime·racefingo();
		} 
		for(; fb; fb=next) {
			next = fb->next;
			for(i=0; i<fb->cnt; i++) {
				f = &fb->fin[i];
				framesz = sizeof(Eface) + f->nret;
				if(framecap < framesz) {
					runtime·free(frame);
					// The frame does not contain pointers interesting for GC,
					// all not yet finalized objects are stored in finc.
					// If we do not mark it as FlagNoScan,
					// the last finalized object is not collected.
					frame = runtime·mallocgc(framesz, 0, FlagNoScan|FlagNoInvokeGC);
					framecap = framesz;
				}
				if(f->fint == nil) {
					runtime·throw("missing type in runfinq");
				} 
				if(f->fint->kind == KindPtr) {
					// direct use of pointer
					*(void**)frame = f->arg;
				} else if(((InterfaceType*)f->fint)->mhdr.len == 0) {
					// convert to empty interface
					ef = (Eface*)frame;
					ef->type = f->ot;
					ef->data = f->arg;
				} else {
					// convert to interface with methods, via empty interface.
					ef1.type = f->ot;
					ef1.data = f->arg;
					if(!runtime·ifaceE2I2((InterfaceType*)f->fint, ef1, (Iface*)frame)) {
						runtime·throw("invalid type conversion in runfinq");
					}
				}
				reflect·call(f->fn, frame, framesz);
				f->fn = nil;
				f->arg = nil;
				f->ot = nil;
			}
			fb->cnt = 0;
			fb->next = finc;
			finc = fb;
		}
		// trigger another gc to clean up the finalized objects, if possible
		runtime·gc(1);
	}
}

// 做一些准备工作, 比如验证 m->helpgc 的合法性, 判断当前g是否为g0等
//
// caller: 
// 	1. runtime·gchelper()
// 	2. gc()
static void gchelperstart(void)
{
	if(m->helpgc < 0 || m->helpgc >= MaxGcproc) {
		runtime·throw("gchelperstart: bad m->helpgc");
	}
	if(runtime·xchg(&bufferList[m->helpgc].busy, 1)) {
		runtime·throw("gchelperstart: already busy");
	}
	if(g != m->g0) {
		runtime·throw("gchelper not running on g0 stack");
	}
}

// 执行这个函数的都是参与gc的几个协程, 数量在 [1-nproc] 之间.
//
// caller: 
// 	1. proc.c -> stopm(), 只有这一处.
// 	stopm() 这个函数有一个休眠等待的过程, 被唤醒时, 如果 m->helpgc > 0,
// 	就会调用这个函数, 来辅助执行 gc.
void runtime·gchelper(void)
{
	// 调用这个做一些准备工作, 比如验证 m->helpgc 的合法性, 判断当前g是否为g0等
	gchelperstart();

	// parallel mark for over gc roots
	runtime·parfordo(work.markfor);

	// help other threads scan secondary blocks
	scanblock(nil, nil, 0, true);

	if(DebugMark) {
		// wait while the main thread executes mark(debug_scanblock)
		while(runtime·atomicload(&work.debugmarkdone) == 0) {
			runtime·usleep(10);
		}
	}

	runtime·parfordo(work.sweepfor);

	bufferList[m->helpgc].busy = 0;
	// ndone+1 后如果等于 nproc-1, 则说明这是最后一个完成gc的线程, 
	// 可以唤醒 alldone 处等待的协程了.
	if(runtime·xadd(&work.ndone, +1) == work.nproc-1) {
		runtime·notewakeup(&work.alldone);
	}
}
