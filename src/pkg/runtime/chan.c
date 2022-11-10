// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "chan.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "../../cmd/ld/textflag.h"

uint32 runtime·Hchansize = sizeof(Hchan);

// caller:
// 	1. runtime·makechan()
Hchan*
runtime·makechan_c(ChanType *t, int64 hint)
{
	Hchan *c;
	Type *elem;

	elem = t->elem;

	// compiler checks this but be safe.
	if(elem->size >= (1<<16)) {
		runtime·throw("makechan: invalid channel element type");
	}
	if((sizeof(*c)%MAXALIGN) != 0 || elem->align > MAXALIGN) {
		runtime·throw("makechan: bad alignment");
	}

	if(hint < 0 || (intgo)hint != hint || (elem->size > 0 && hint > MaxMem / elem->size)) {
		runtime·panicstring("makechan: size out of range");
	}

	// allocate memory in one call
	c = (Hchan*)runtime·mallocgc(sizeof(*c) + hint*elem->size, (uintptr)t | TypeInfo_Chan, 0);
	c->elemsize = elem->size;
	c->elemalg = elem->alg;
	c->dataqsiz = hint;

	if(debug) {
		runtime·printf(
			"makechan: chan=%p; elemsize=%D; elemalg=%p; dataqsiz=%D\n",
			c, (int64)elem->size, elem->alg, (int64)c->dataqsiz
		);
	}

	return c;
}

// For reflect
//	func makechan(typ *ChanType, size uint64) (chan)
void
reflect·makechan(ChanType *t, uint64 size, Hchan *c)
{
	c = runtime·makechan_c(t, size);
	FLUSH(&c);
}

// golang原生: make(chan string, 10) 函数
//
// 调用 malloc 创建一个 channel 对象, 并分配内存.
//
// param t: channel类型(如 string, int, struct 等)
// param hint: channel 容量.
//
// makechan(t *ChanType, hint int64) (hchan *chan any);
void
runtime·makechan(ChanType *t, int64 hint, Hchan *ret)
{
	ret = runtime·makechan_c(t, hint);
	FLUSH(&ret);
}

// golang原生: chan <- var, 向 channel 对象中发送数据.
//
// 在同一个函数中同时实现了"同步"与"异步"
//
// 	@param pres: 是否"接收"成功, 指针类型, 此值被修改后, 主调函数可以得知结果.
// 	一般来说, 向 chan 中写入数据, 是没有 ok 变量的.
// 	只有在 select {case: chan <- var} 这种场景时, pres 才不为 nil.
//
// generic single channel send/recv if the bool pointer is nil,
// then the full exchange will occur.
// if pres is not nil, then the protocol will not sleep
// but return if it could not complete.
//
// sleep can wake up with g->param == nil
// when a channel involved in the sleep has been closed. 
// it is easiest to loop and re-run the operation; we'll see that it's now closed.
//
void
runtime·chansend(ChanType *t, Hchan *c, byte *ep, bool *pres, void *pc)
{
	SudoG *sg;
	SudoG mysg;
	G* gp;
	int64 t0;

	// 如果 channel 对象为 nil(比如只声明变量, 未使用 make() 进行初始)
	if(c == nil) {
		USED(t);
		if(pres != nil) {
			// [anchor] 运行到该 if 块内的示例, 请见 010.channel 中的 chan01() 函数.
			*pres = false;
			return;
		}
		runtime·park(nil, nil, "chan send (nil chan)");
		return;  // not reached
	}

	if(debug) {
		runtime·printf("chansend: chan=%p; elem=", c);
		c->elemalg->print(c->elemsize, ep);
		runtime·prints("\n");
	}

	t0 = 0;
	mysg.releasetime = 0;
	if(runtime·blockprofilerate > 0) {
		t0 = runtime·cputicks();
		mysg.releasetime = -1;
	}

	runtime·lock(c);
	if(raceenabled){
		runtime·racereadpc(c, pc, runtime·chansend);
	} 

	// 向已经关闭的 channel 中写入数据, 会引发 panic.
	if(c->closed) {
		goto closed;
	} 
	// 如果是有缓冲队列, 则执行异步版本函数.
	if(c->dataqsiz > 0) {
		goto asynch;
	} 

	sg = dequeue(&c->recvq);
	if(sg != nil) {
		if(raceenabled){
			racesync(c, sg);
		} 
		// 运行到此处, 说明是同步收发的流程, 同步收发完全可以两个 goroutine 一对一完成,
		// 不需要经过 channel, 这里直接把锁释放.
		runtime·unlock(c);

		gp = sg->g;
		gp->param = sg;
		if(sg->elem != nil) {
			// 将参数3对象的内容, 拷贝到参数2对象.
			// 这里是将待写入的数据直接放到 receiver 对象中, 不在 channel 中存储一遍了.
			//
			// 注意: receiver 中也有这个过程, 看当前的逻辑, sender 与 receiver 必定
			// 一前一后地订阅一个 channel, 而且总是后来者完成 copy 的工作.
			c->elemalg->copy(c->elemsize, sg->elem, ep);
		} 
		if(sg->releasetime) {
			sg->releasetime = runtime·cputicks();
		} 
		runtime·ready(gp);

		if(pres != nil) {
			*pres = true;
		} 
		return;
	}

	if(pres != nil) {
		runtime·unlock(c);
		*pres = false;
		return;
	}

	// 运行到这里, 说明 channel 是无缓冲的, 而且没有 receiver, 那么写入行为会被阻塞.
	// 在将自身 g 对象注册到 channel 对象的 sendq 队列后, 当前 g 就要挂起了(执行其他任务).
	mysg.elem = ep;
	mysg.g = g;
	mysg.selgen = NOSELGEN;
	g->param = nil;
	enqueue(&c->sendq, &mysg);
	// 这里当前 sender 会被挂起, 直到有新的协程成为该 channel 的 receiver, 
	// receiver 会调用 runtime·unlock(), 于是当前 sender 就会被被唤醒.
	//
	// [anchor] 运行到此处的示例, 请见 010.channel 中的 chan03() 函数.
	runtime·park(runtime·unlock, c, "chan send");

	if(g->param == nil) {
		runtime·lock(c);
		if(!c->closed) {
			runtime·throw("chansend: spurious wakeup");
		} 
		goto closed;
	}

	if(mysg.releasetime > 0) {
		runtime·blockevent(mysg.releasetime - t0, 2);
	} 

	// 无缓冲 channel 的数据发送处理结束
	return;

// 有缓冲 channel 的数据发送处理方案
asynch:
	if(c->closed) {
		goto closed;
	} 

	// 异步队列的缓冲区只有 dataqsiz 这么大, 如果其中的成员数量等于这个值, 说明已经满了,
	// 则发送端所在的 goroutine 需要挂起, 无法发送.
	if(c->qcount >= c->dataqsiz) {
		if(pres != nil) {
			runtime·unlock(c);
			*pres = false;
			return;
		}
		mysg.g = g;
		mysg.elem = nil;
		mysg.selgen = NOSELGEN;
		enqueue(&c->sendq, &mysg);
		runtime·park(runtime·unlock, c, "chan send");

		runtime·lock(c);
		// 被唤醒, 则回到 asynch 开头, 再次尝试向 channel 写数据.
		goto asynch;
	}

	if(raceenabled) {
		runtime·racerelease(chanbuf(c, c->sendx));
	} 

	c->elemalg->copy(c->elemsize, chanbuf(c, c->sendx), ep);
	if(++c->sendx == c->dataqsiz) {
		c->sendx = 0;
	} 
	c->qcount++;

	sg = dequeue(&c->recvq);
	if(sg != nil) {
		gp = sg->g;
		runtime·unlock(c);
		if(sg->releasetime) {
			sg->releasetime = runtime·cputicks();
		} 
		runtime·ready(gp);
	} else {
		runtime·unlock(c);
	}

	if(pres != nil) {
		*pres = true;
	} 
	if(mysg.releasetime > 0) {
		runtime·blockevent(mysg.releasetime - t0, 2);
	}
	return;

// channel 的异常操作, 引发 panic
closed:
	runtime·unlock(c);
	runtime·panicstring("send on closed channel");
}

// golang原生: var <- channel, 从 channel 对象中接收数据.
//
// 	@param ep: 接收结果的变量地址
// 	@param received: 应该是 var, ok := <- channel 场景中的 "ok" 值.
void
runtime·chanrecv(ChanType *t, Hchan* c, byte *ep, bool *selected, bool *received)
{
	SudoG *sg;
	SudoG mysg;
	G *gp;
	int64 t0;

	if(debug) {
		runtime·printf("chanrecv: chan=%p\n", c);
	} 

	if(c == nil) {
		USED(t);
		if(selected != nil) {
			*selected = false;
			return;
		}
		runtime·park(nil, nil, "chan receive (nil chan)");
		return;  // not reached
	}

	t0 = 0;
	mysg.releasetime = 0;
	if(runtime·blockprofilerate > 0) {
		t0 = runtime·cputicks();
		mysg.releasetime = -1;
	}

	runtime·lock(c);
	if(c->dataqsiz > 0) {
		goto asynch;
	}

	if(c->closed) {
		goto closed;
	}

	sg = dequeue(&c->sendq);
	if(sg != nil) {
		if(raceenabled) {
			racesync(c, sg);
		}
		// 运行到这里, 说明是无缓冲 channel, 并非已经有一个准备写入的 goroutine 了.
		// 由于没有 receiver, 写入的那个 goroutine 一定会因为调用了 runtime·lock(),
		//
		// [anchor] 可见 010.channel 中的 chan03() 函数, 对 sender 的影响.
		runtime·unlock(c);

		if(ep != nil) {
			// 将参数3对象的内容, 拷贝到参数2对象.
			// 这里是从 sender 的 elem 中直接获取数据, 不在 channel 中存储一遍了.
			//
			// 注意: sender 中也有这个过程, 看当前的逻辑, sender 与 receiver 必定
			// 一前一后地订阅一个 channel, 而且总是后来者完成 copy 的工作.
			c->elemalg->copy(c->elemsize, ep, sg->elem);
		}
		gp = sg->g;
		gp->param = sg;
		if(sg->releasetime) {
			sg->releasetime = runtime·cputicks();
		}
		runtime·ready(gp);

		if(selected != nil) {
			*selected = true;
		}
		if(received != nil) {
			*received = true;
		}
		return;
	}

	if(selected != nil) {
		runtime·unlock(c);
		*selected = false;
		return;
	}

	mysg.elem = ep;
	mysg.g = g;
	mysg.selgen = NOSELGEN;
	g->param = nil;
	enqueue(&c->recvq, &mysg);
	runtime·park(runtime·unlock, c, "chan receive");

	if(g->param == nil) {
		runtime·lock(c);
		if(!c->closed) {
			runtime·throw("chanrecv: spurious wakeup");
		}
		goto closed;
	}

	if(received != nil) {
		*received = true;
	}
	if(mysg.releasetime > 0) {
		runtime·blockevent(mysg.releasetime - t0, 2);
	}
	return;

asynch:
	if(c->qcount <= 0) {
		if(c->closed) {
			goto closed;
		}

		if(selected != nil) {
			runtime·unlock(c);
			*selected = false;
			if(received != nil) {
				*received = false;
			}
			return;
		}
		mysg.g = g;
		mysg.elem = nil;
		mysg.selgen = NOSELGEN;
		enqueue(&c->recvq, &mysg);
		runtime·park(runtime·unlock, c, "chan receive");

		runtime·lock(c);
		goto asynch;
	}

	if(raceenabled) {
		runtime·raceacquire(chanbuf(c, c->recvx));
	}

	if(ep != nil) {
		c->elemalg->copy(c->elemsize, ep, chanbuf(c, c->recvx));
	}
	c->elemalg->copy(c->elemsize, chanbuf(c, c->recvx), nil);
	if(++c->recvx == c->dataqsiz) {
		c->recvx = 0;
	}
	c->qcount--;

	sg = dequeue(&c->sendq);
	if(sg != nil) {
		gp = sg->g;
		runtime·unlock(c);
		if(sg->releasetime) {
			sg->releasetime = runtime·cputicks();
		}
		runtime·ready(gp);
	} else {
		runtime·unlock(c);
	}

	if(selected != nil) {
		*selected = true;
	}
	if(received != nil) {
		*received = true;
	}
	if(mysg.releasetime > 0) {
		runtime·blockevent(mysg.releasetime - t0, 2);
	}
	return;

closed:
	if(ep != nil) {
		c->elemalg->copy(c->elemsize, ep, nil);
	}
	if(selected != nil) {
		*selected = true;
	}
	if(received != nil) {
		*received = false;
	}
	if(raceenabled) {
		runtime·raceacquire(c);
	}
	runtime·unlock(c);
	if(mysg.releasetime > 0) {
		runtime·blockevent(mysg.releasetime - t0, 2);
	}
}

// chansend1(hchan *chan any, elem any);
#pragma textflag NOSPLIT
void
runtime·chansend1(ChanType *t, Hchan* c, ...)
{
	runtime·chansend(t, c, (byte*)(&c+1), nil, runtime·getcallerpc(&t));
}

// chanrecv1(hchan *chan any) (elem any);
#pragma textflag NOSPLIT
void
runtime·chanrecv1(ChanType *t, Hchan* c, ...)
{
	runtime·chanrecv(t, c, (byte*)(&c+1), nil, nil);
}

// chanrecv2(hchan *chan any) (elem any, received bool);
#pragma textflag NOSPLIT
void
runtime·chanrecv2(ChanType *t, Hchan* c, ...)
{
	byte *ae, *ap;

	ae = (byte*)(&c+1);
	ap = ae + t->elem->size;
	runtime·chanrecv(t, c, ae, nil, ap);
}

// func selectnbsend(c chan any, elem any) bool
//
// compiler implements
//
//	select {
//	case c <- v:
//		... foo
//	default:
//		... bar
//	}
//
// as
//
//	if selectnbsend(c, v) {
//		... foo
//	} else {
//		... bar
//	}
//
#pragma textflag NOSPLIT
void
runtime·selectnbsend(ChanType *t, Hchan *c, ...)
{
	byte *ae, *ap;

	ae = (byte*)(&c + 1);
	ap = ae + ROUND(t->elem->size, Structrnd);
	runtime·chansend(t, c, ae, ap, runtime·getcallerpc(&t));
}

// func selectnbrecv(elem *any, c chan any) bool
//
// compiler implements
//
//	select {
//	case v = <-c:
//		... foo
//	default:
//		... bar
//	}
//
// as
//
//	if selectnbrecv(&v, c) {
//		... foo
//	} else {
//		... bar
//	}
//
#pragma textflag NOSPLIT
void
runtime·selectnbrecv(ChanType *t, byte *v, Hchan *c, bool selected)
{
	runtime·chanrecv(t, c, v, &selected, nil);
}

// func selectnbrecv2(elem *any, ok *bool, c chan any) bool
//
// compiler implements
//
//	select {
//	case v, ok = <-c:
//		... foo
//	default:
//		... bar
//	}
//
// as
//
//	if c != nil && selectnbrecv2(&v, &ok, c) {
//		... foo
//	} else {
//		... bar
//	}
//
#pragma textflag NOSPLIT
void
runtime·selectnbrecv2(ChanType *t, byte *v, bool *received, Hchan *c, bool selected)
{
	runtime·chanrecv(t, c, v, &selected, received);
}

// For reflect:
//	func chansend(c chan, val iword, nb bool) (selected bool)
// where an iword is the same word an interface value would use:
// the actual data if it fits, or else a pointer to the data.
//
// The "uintptr selected" is really "bool selected" but saying
// uintptr gets us the right alignment for the output parameter block.
#pragma textflag NOSPLIT
void
reflect·chansend(ChanType *t, Hchan *c, uintptr val, bool nb, uintptr selected)
{
	bool *sp;
	byte *vp;

	if(nb) {
		selected = false;
		sp = (bool*)&selected;
	} else {
		*(bool*)&selected = true;
		FLUSH(&selected);
		sp = nil;
	}
	if(t->elem->size <= sizeof(val)) {
		vp = (byte*)&val;
	}
	else {
		vp = (byte*)val;
	}
	runtime·chansend(t, c, vp, sp, runtime·getcallerpc(&t));
}

// For reflect:
//	func chanrecv(c chan, nb bool) (val iword, selected, received bool)
// where an iword is the same word an interface value would use:
// the actual data if it fits, or else a pointer to the data.
void
reflect·chanrecv(ChanType *t, Hchan *c, bool nb, uintptr val, bool selected, bool received)
{
	byte *vp;
	bool *sp;

	if(nb) {
		selected = false;
		sp = &selected;
	} else {
		selected = true;
		FLUSH(&selected);
		sp = nil;
	}
	received = false;
	FLUSH(&received);
	if(t->elem->size <= sizeof(val)) {
		val = 0;
		vp = (byte*)&val;
	} else {
		vp = runtime·mal(t->elem->size);
		val = (uintptr)vp;
		FLUSH(&val);
	}
	runtime·chanrecv(t, c, vp, sp, &received);
}

static void closechan(Hchan *c, void *pc);

// closechan(sel *byte);
#pragma textflag NOSPLIT
void
runtime·closechan(Hchan *c)
{
	closechan(c, runtime·getcallerpc(&c));
}

// For reflect
//	func chanclose(c chan)
#pragma textflag NOSPLIT
void
reflect·chanclose(Hchan *c)
{
	closechan(c, runtime·getcallerpc(&c));
}

static void
closechan(Hchan *c, void *pc)
{
	SudoG *sg;
	G* gp;

	if(c == nil) {
		runtime·panicstring("close of nil channel");
	} 

	runtime·lock(c);
	if(c->closed) {
		runtime·unlock(c);
		runtime·panicstring("close of closed channel");
	}

	if(raceenabled) {
		runtime·racewritepc(c, pc, runtime·closechan);
		runtime·racerelease(c);
	}

	c->closed = true;

	// release all readers
	for(;;) {
		sg = dequeue(&c->recvq);
		if(sg == nil) {
			break;
		} 
		gp = sg->g;
		gp->param = nil;
		if(sg->releasetime) {
			sg->releasetime = runtime·cputicks();
		}
		runtime·ready(gp);
	}

	// release all writers
	for(;;) {
		sg = dequeue(&c->sendq);
		if(sg == nil) {
			break;
		}
		gp = sg->g;
		gp->param = nil;
		if(sg->releasetime) {
			sg->releasetime = runtime·cputicks();
		}
		runtime·ready(gp);
	}

	runtime·unlock(c);
}

// For reflect
//	func chanlen(c chan) (len int)
void
reflect·chanlen(Hchan *c, intgo len)
{
	if(c == nil) {
		len = 0;
	}
	else {
		len = c->qcount;
	}
	FLUSH(&len);
}

// For reflect
//	func chancap(c chan) int
void
reflect·chancap(Hchan *c, intgo cap)
{
	if(c == nil) {
		cap = 0;
	}
	else {
		cap = c->dataqsiz;
	}
	FLUSH(&cap);
}

////////////////////////////////////////////////////////////////////////////////
// 下述3个函数: dequeue, enqueue, racesync, 同时在 chan.c 和 select.c 中使用.
// 虽然在 chan.h 中有ta们的声明, 但是由于是静态类型, 需要在所有用到ta们的 .c 文件中定义.
// 这是C语言的特性规定, 以后可以考虑将这3个函数修改为非静态函数.
// 相关资料见: https://stackoverflow.com/questions/42056160/static-functions-declared-in-c-header-files

// 从 channel 自身的 recvq/sendq 队列中, 随便取一个 goroutine 返回.
static SudoG*
dequeue(WaitQ *q)
{
	SudoG *sgp;

loop:
	sgp = q->first;
	if(sgp == nil) {
		return nil;
	} 
	q->first = sgp->link;

	// if sgp is stale, ignore it
	if(sgp->selgen != NOSELGEN &&
		(sgp->selgen != sgp->g->selgen ||
		!runtime·cas(&sgp->g->selgen, sgp->selgen, sgp->selgen + 2))) {
		//prints("INVALID PSEUDOG POINTER\n");
		goto loop;
	}

	return sgp;
}

static void
enqueue(WaitQ *q, SudoG *sgp)
{
	sgp->link = nil;
	if(q->first == nil) {
		q->first = sgp;
		q->last = sgp;
		return;
	}
	q->last->link = sgp;
	q->last = sgp;
}

static void
racesync(Hchan *c, SudoG *sg)
{
	runtime·racerelease(chanbuf(c, 0));
	runtime·raceacquireg(sg->g, chanbuf(c, 0));
	runtime·racereleaseg(sg->g, chanbuf(c, 0));
	runtime·raceacquire(chanbuf(c, 0));
}
////////////////////////////////////////////////////////////////////////////////
