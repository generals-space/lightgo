// Copyright 2012 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Parallel for algorithm.
// 并行算法.

#include "runtime.h"
#include "arch_amd64.h"

struct ParForThread
{
	// 在 runtime·parforsetup() 函数中通过 [begin,end) 计算得来.
	// 然后在 runtime·parfordo 被取值, 然后还原成 [begin,end).
	//
	// [begin,end) 表示每个并行线程, 在任务列表中属于自己的任务.
	// 假设 work.roots 数组有 6 个成员, 一共有3个线程共同完成标记任务, 即 nthr = 3, n = 6.
	//
	// 那么这3个线程将分别处理 roots 列表中属于自己的任务:
	// 0: work.roots[0,2)
	// 1: work.roots[2,4)
	// 3: work.roots[4,6)
	//
	// the thread's iteration space [32lsb, 32msb)
	uint64 pos;
	// stats
	uint64 nsteal;
	uint64 nstealcnt;
	uint64 nprocyield;
	uint64 nosyield;
	uint64 nsleep;
	byte pad[CacheLineSize];
};

// 	@param nthrmax: 可执行 gc 的最大线程数 MaxGcproc
//
// caller:
// 	1. src/pkg/runtime/mgc0.c -> gc()
ParFor* runtime·parforalloc(uint32 nthrmax)
{
	ParFor *desc;

	// The ParFor object is followed by CacheLineSize padding
	// and then nthrmax ParForThread.
	desc = (ParFor*)runtime·malloc(sizeof(ParFor) + CacheLineSize + nthrmax * sizeof(ParForThread));
	desc->thr = (ParForThread*)((byte*)(desc+1) + CacheLineSize);
	desc->nthrmax = nthrmax;
	return desc;
}

// For testing from Go
// func parforalloc2(nthrmax uint32) *ParFor
void runtime·parforalloc2(uint32 nthrmax, ParFor *desc)
{
	desc = runtime·parforalloc(nthrmax);
	FLUSH(&desc);
}

// 	@param desc: src/pkg/runtime/mgc0.c -> work.{markfor, sweepfor} 成员, 
// 	这两个字段都是 ParFor 类型.
// 	@param nthr: (thread num) 取值为 src/pkg/runtime/mgc0.c -> work.nproc
// 	表示并发执行的线程数
// 	@param n: 应该是待执行的任务的数量, 分别对应 work.nroot 和 runtime·mheap.nspan,
// 	因为 ta 们表示的是并发标记/清除所要遍历的对象的数量(看一下主调函数立刻就会明白);
// 	@param ctx: nil
// 	@param body: 并发执行的方法名称, 分别为 markroot(), sweepspan()
//
// caller:
// 	1. src/pkg/runtime/mgc0.c -> gc() 只有这一处
// 	在完成了准备工作(STW), 真正执行 gc 前, 设置并发执行的函数, 包括标记(mark)和清除(sweep)
void runtime·parforsetup(
	ParFor *desc, uint32 nthr, uint32 n, void *ctx, bool wait, 
	void (*body)(ParFor*, uint32)
)
{
	uint32 i, begin, end;
	uint64 *pos;

	if(desc == nil || nthr == 0 || nthr > desc->nthrmax || body == nil) {
		runtime·printf("desc=%p nthr=%d count=%d body=%p\n", desc, nthr, n, body);
		runtime·throw("parfor: invalid args");
	}

	desc->body = body;
	desc->done = 0;
	desc->nthr = nthr;
	desc->thrseq = 0;
	desc->cnt = n;
	desc->ctx = ctx;
	desc->wait = wait;
	desc->nsteal = 0;
	desc->nstealcnt = 0;
	desc->nprocyield = 0;
	desc->nosyield = 0;
	desc->nsleep = 0;
	// 这里是为各并行线程分配任务索引的, 以 work.roots 为例, 
	// 假设 work.roots 数组有 6 个成员, 一共有3个线程共同完成标记任务, 即 nthr = 3, n = 6.
	//
	// 那么这3个线程将分别处理 roots 列表中属于自己的任务:
	// 0: work.roots[0,2)
	// 1: work.roots[2,4)
	// 3: work.roots[4,6)
	//
	// i 表示当前线程在线程组中的序号.
	// begin 对应每个线程分配到的索引起始点, 是闭区间, 而 end 则是结束点, 是开区间.
	for(i=0; i<nthr; i++) {
		begin = (uint64)n*i / nthr;
		end = (uint64)n*(i+1) / nthr;
		pos = &desc->thr[i].pos;
		if(((uintptr)pos & 7) != 0) {
			runtime·throw("parforsetup: pos is not aligned");
		}
		// pos 在下面 runtime·parfordo() 函数中被取值并还原成 [begin,end)
		*pos = (uint64)begin | (((uint64)end)<<32);
	}
}

// For testing from Go
// func parforsetup2(desc *ParFor, nthr, n uint32, ctx *byte, wait bool, body func(*ParFor, uint32))
void
runtime·parforsetup2(ParFor *desc, uint32 nthr, uint32 n, void *ctx, bool wait, void *body)
{
	runtime·parforsetup(desc, nthr, n, ctx, wait, *(void(**)(ParFor*, uint32))body);
}

// 并行的执行 gc 过程中的某个流程, 如"标记", 或是"清除"
//
// 	@param desc: work.markfor(标记流程), 或是 work.sweepfor(清除流程)
//
// caller:
// 	1. src/pkg/runtime/mgc0.c -> gc()
void
runtime·parfordo(ParFor *desc)
{
	ParForThread *me;
	uint32 tid, begin, end, begin2, try, victim, i;
	uint64 *mypos, *victimpos, pos, newpos;
	void (*body)(ParFor*, uint32);
	bool idle;

	// Obtain 0-based thread index.
	tid = runtime·xadd(&desc->thrseq, 1) - 1;
	if(tid >= desc->nthr) {
		runtime·printf("tid=%d nthr=%d\n", tid, desc->nthr);
		runtime·throw("parfor: invalid tid");
	}

	// desc->nthr 为 work.nproc, 即参与执行 GC 的线程数量.
	//
	// If single-threaded, just execute the for serially.
	if(desc->nthr==1) {
		for(i=0; i<desc->cnt; i++) {
			// 执行 work.markfor(标记流程), 或是 work.sweepfor(清除流程)
			desc->body(desc, i);
		}
		return;
	}

	// body 可以有2个值, 都是在 gc 时的行为, 分别为
	// 1. src/pkg/runtime/mgc0.c -> markroot() 标记
	// 2. src/pkg/runtime/mgc0.c -> sweepspan() 清除
	body = desc->body;
	me = &desc->thr[tid];
	mypos = &me->pos;
	for(;;) {
		for(;;) {
			// pos 在上面 runtime·parforsetup() 函数中通过 [begin,end) 计算得来.
			// 这里被取值并还原成 [begin,end)
			//
			// [begin,end) 表示每个并行线程, 在任务列表中属于自己的任务.
			// 假设 work.roots 数组有 6 个成员, 一共有3个线程共同完成标记任务,
			// 即 nthr = 3, n = 6.
			//
			// 那么这3个线程将分别处理 roots 列表中属于自己的任务:
			// 0: work.roots[0,2)
			// 1: work.roots[2,4)
			// 3: work.roots[4,6)
			//
			// While there is local work,
			// bump low index and execute the iteration.
			pos = runtime·xadd64(mypos, 1);
			begin = (uint32)pos-1;
			end = (uint32)(pos>>32);
			if(begin < end) {
				// 执行 work.markfor(标记流程), 或是 work.sweepfor(清除流程)
				body(desc, begin);
				continue;
			}
			// 当前线程已经完成了属于自己的所有任务, 则结束循环.
			break;
		}

		// Out of work, need to steal something.
		// 工作完成
		idle = false;
		for(try=0;; try++) {
			// If we don't see any work for long enough,
			// increment the done counter...
			if(try > desc->nthr*4 && !idle) {
				idle = true;
				runtime·xadd(&desc->done, 1);
			}
			// ...if all threads have incremented the counter,
			// we are done.
			if(desc->done + !idle == desc->nthr) {
				if(!idle) {
					runtime·xadd(&desc->done, 1);
				}
				goto exit;
			}
			// Choose a random victim for stealing.
			victim = runtime·fastrand1() % (desc->nthr-1);
			if(victim >= tid) {
				victim++;
			}
			victimpos = &desc->thr[victim].pos;
			for(;;) {
				// See if it has any work.
				pos = runtime·atomicload64(victimpos);
				begin = (uint32)pos;
				end = (uint32)(pos>>32);
				if(begin+1 >= end) {
					begin = end = 0;
					break;
				}
				if(idle) {
					runtime·xadd(&desc->done, -1);
					idle = false;
				}
				begin2 = begin + (end-begin)/2;
				newpos = (uint64)begin | (uint64)begin2<<32;
				if(runtime·cas64(victimpos, pos, newpos)) {
					begin = begin2;
					break;
				}
			}
			if(begin < end) {
				// Has successfully stolen some work.
				if(idle) {
					runtime·throw("parfor: should not be idle");
				}
				runtime·atomicstore64(mypos, (uint64)begin | (uint64)end<<32);
				me->nsteal++;
				me->nstealcnt += end-begin;
				break;
			}
			// Backoff.
			if(try < desc->nthr) {
				// nothing
			} else if (try < 4*desc->nthr) {
				me->nprocyield++;
				runtime·procyield(20);
			// If a caller asked not to wait for the others, exit now
			// (assume that most work is already done at this point).
			} else if (!desc->wait) {
				if(!idle) {
					runtime·xadd(&desc->done, 1);
				}
				goto exit;
			} else if (try < 6*desc->nthr) {
				me->nosyield++;
				runtime·osyield();
			} else {
				me->nsleep++;
				runtime·usleep(1);
			}
		}
	}
exit:
	runtime·xadd64(&desc->nsteal, me->nsteal);
	runtime·xadd64(&desc->nstealcnt, me->nstealcnt);
	runtime·xadd64(&desc->nprocyield, me->nprocyield);
	runtime·xadd64(&desc->nosyield, me->nosyield);
	runtime·xadd64(&desc->nsleep, me->nsleep);
	me->nsteal = 0;
	me->nstealcnt = 0;
	me->nprocyield = 0;
	me->nosyield = 0;
	me->nsleep = 0;
}

// For testing from Go
// func parforiters(desc *ParFor, tid uintptr) (uintptr, uintptr)
void
runtime·parforiters(ParFor *desc, uintptr tid, uintptr start, uintptr end)
{
	start = (uint32)desc->thr[tid].pos;
	end = (uint32)(desc->thr[tid].pos>>32);
	FLUSH(&start);
	FLUSH(&end);
}
