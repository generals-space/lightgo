#include "runtime.h"
#include "arch_amd64.h"

#include "malloc.h"

#include "mgc0.h"
#include "mgc0__stats.h"

#include "type.h"

#include "mgc0__others.h"

extern struct WORK work;

extern Workbuf* getempty(Workbuf*);
extern Workbuf* handoff(Workbuf*);

// 	@param obj: work.roots 数组中的某个成员, 其实是 arena 区域中的某个地址.
//
// caller:
// 	1. markroot()
// 	2. scanblock()
//
// Append obj to the work buffer.
// _wbuf, _wp, _nobj are input/output parameters 
// and are specifying the work buffer.
void work_enqueue(Obj obj, Workbuf **_wbuf, Obj **_wp, uintptr *_nobj)
{
	uintptr nobj, off;
	Obj *wp;
	Workbuf *wbuf;

	if(Debug > 1) {
		runtime·printf("append obj(%p %D %p)\n", obj.p, (int64)obj.n, obj.ti);
	}

	// Align obj.b to a word boundary.
	off = (uintptr)obj.p & (PtrSize-1);
	if(off != 0) {
		obj.p += PtrSize - off;
		obj.n -= PtrSize - off;
		obj.ti = 0;
	}

	if(obj.p == nil || obj.n == 0) {
		return;
	} 

	// Load work buffer state
	wp = *_wp;
	wbuf = *_wbuf;
	nobj = *_nobj;

	// If another proc wants a pointer, give it some.
	if(work.nwait > 0 && nobj > handoffThreshold && work.full == 0) {
		wbuf->nobj = nobj;
		wbuf = handoff(wbuf);
		nobj = wbuf->nobj;
		wp = wbuf->obj + nobj;
	}

	// If buffer is full, get a new one.
	if(wbuf == nil || nobj >= nelem(wbuf->obj)) {
		if(wbuf != nil) {
			wbuf->nobj = nobj;
		}
		wbuf = getempty(wbuf);
		wp = wbuf->obj;
		nobj = 0;
	}

	*wp = obj;
	wp++;
	nobj++;

	// Save work buffer state
	*_wp = wp;
	*_wbuf = wbuf;
	*_nobj = nobj;
}

extern void scanblock(Workbuf *wbuf, Obj *wp, uintptr nobj, bool keepworking);

// 	@param desc: work.markfor, 实际是 markroot(), 是一个函数.
// 	@param i: 为任务索引, work.roots 为全部任务, i 的范围在 [0, (work.nroot-1)]
//
// caller: 
// 	1. gc() 只有这一处, 在 addroots() 之后被设置, 
// 	但实际是在 src/pkg/runtime/parfor.c -> runtime·parfordo() 中, 
// 	作为 desc->body 成员被调用.
void markroot(ParFor *desc, uint32 i)
{
	Obj *wp;
	Workbuf *wbuf;
	uintptr nobj;

	USED(&desc);
	wp = nil;
	wbuf = nil;
	nobj = 0;
	// runtime·printf("markroot index: %d\n", i);
	work_enqueue(work.roots[i], &wbuf, &wp, &nobj);
	scanblock(wbuf, wp, nobj, false);
}

////////////////////////////////////////////////////////////////////////////////
extern struct GCSTATS gcstats;

// 标记对象. 主要就是找到该对象在 bitmap 区域的映射地址, 添加 bitMarked 标记.
// (需要目标块已存在 bitAllocated 标记)
//
// 注意: 被标记过的块才是安全的, 反之, 则需要被回收.
//
// 	@param obj: arena 区域中的某个指针(ta在 bitmap 区域一定有对应的描述字(word)信息)
//
// caller:
// 	1. addroots()
//  2. scanblock()
//
// markonly marks an object. 
// It returns true if the object has been marked by this function, false otherwise.
// This function doesn't append the object to any buffer.
bool markonly(void *obj)
{
	byte *p;
	uintptr *bitp, bits, shift, x, xbits, off, j;
	MSpan *s;
	PageID k;

	// Words outside the arena cannot be pointers.
	if(obj < runtime·mheap.arena_start || obj >= runtime·mheap.arena_used) {
		return false;
	}

	// obj可能是指向一个对象的指针, 尝试找到这个对象的起始地址.
	// 这么说其实是obj可能是某个结构体内部的成员? 虽然是指针但并不指向对象的起始位置.
	//
	// obj may be a pointer to a live object.
	// Try to find the beginning of the object.

	// 向下取整到字边界...啥意思?
	//
	// Round down to word boundary. 
	obj = (void*)((uintptr)obj & ~((uintptr)PtrSize-1));

	// 下面几行计算语句, 根据 obj 对象地址, 反推其在 bitmap 中描述字的地址, 详细解释可见:
	// <!link!>: {32a3d702-70db-4cae-852b-5c12ce491afc}
	//
	// Find bits for this word.
	off = (uintptr*)obj - (uintptr*)runtime·mheap.arena_start;
	bitp = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;
	xbits = *bitp;
	bits = xbits >> shift;

	// 如果正好指向一个块的起始位置(后面还有连续的内容)
	//
	// 注意: 这里两个"bit位"是或运算, 表示同时成立.
	//
	// Pointing at the beginning of a block?
	if((bits & (bitAllocated|bitBlockBoundary)) != 0) {
		if(CollectStats) {
			runtime·xadd64(&gcstats.markonly.foundbit, 1);
		} 
		goto found;
	}

	// 如果指向的不是起始地址, 反向寻找直到找到边界.
	// 不过这个 for{} 循环的意思, 好像边界就是在当前 bitmap 块内...那更大的对象呢.
	//
	// Pointing just past the beginning?
	// Scan backward a little to find a block boundary.
	for(j=shift; j-->0; ) {
		if(((xbits>>j) & (bitAllocated|bitBlockBoundary)) != 0) {
			shift = j;
			bits = xbits>>shift;
			if(CollectStats) {
				runtime·xadd64(&gcstats.markonly.foundword, 1);
			} 
			goto found;
		}
	}

	// 下面几行计算语句, 根据 v 对象地址, 查找其在 span 区域的索引映射, 详细解释可见:
	// <!link!>: {eeaeb3fe-8dc4-44a3-8b9c-42919ae5ad4a}
	//
	// Otherwise consult span table to find beginning.
	// (Manually inlined copy of MHeap_LookupMaybe.)
	k = (uintptr)obj>>PageShift;
	x = k;
	if(sizeof(void*) == 8) {
		x -= (uintptr)runtime·mheap.arena_start>>PageShift;
	}
	s = runtime·mheap.spans[x];

	if(s == nil || k < s->start || obj >= s->limit || s->state != MSpanInUse) {
		return false;
	}

	p = (byte*)((uintptr)s->start<<PageShift);
	if(s->sizeclass == 0) {
		obj = p;
	} else {
		uintptr size = s->elemsize;
		int32 i = ((byte*)obj - p)/size;
		obj = p+i*size;
	}

	// Now that we know the object header, reload bits.
	off = (uintptr*)obj - (uintptr*)runtime·mheap.arena_start;
	bitp = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;
	xbits = *bitp;
	bits = xbits >> shift;
	if(CollectStats) {
		runtime·xadd64(&gcstats.markonly.foundspan, 1);
	} 

// 为目标字设置 bitMarked 标记.
found:
	// 如果 obj 已经失去了 bitAllocated 标记, 或是已经拥有了 bitMarked 标记.
	// 表示ta被人释放/标记过了, 直接退出, 不再进行下一步操作.
	//
	// Now we have bits, bitp, and shift correct for
	// obj pointing at the base of the object.
	// Only care about allocated and not marked.
	if((bits & (bitAllocated|bitMarked)) != bitAllocated) {
		return false;
	}
	// 如果只有一个 GC 工作线程, 直接添加 bitMarked 标记
	if(work.nproc == 1) {
		*bitp |= bitMarked<<shift;
	} else {
		for(;;) {
			x = *bitp;
			if(x & (bitMarked<<shift)) {
				return false;
			} 
			if(runtime·casp((void**)bitp, (void*)x, (void*)(x|(bitMarked<<shift)))) {
				break;
			} 
		}
	}

	// The object is now marked
	return true;
}
