// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Page heap.
//
// See malloc.h for overview.
//
// When a MSpan is in the heap free list, state == MSpanFree
// and heapmap(s->start) == span, heapmap(s->start+s->npages-1) == span.
//
// When a MSpan is allocated, state == MSpanInUse
// and heapmap(i) == span for all s->start <= i < s->start+s->npages.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "malloc.h"

static MSpan *MHeap_AllocLocked(MHeap*, uintptr, int32);
static bool MHeap_Grow(MHeap*, uintptr);
static void MHeap_FreeLocked(MHeap*, MSpan*);
static MSpan *MHeap_AllocLarge(MHeap*, uintptr);
static MSpan *BestFit(MSpan*, uintptr, MSpan*);

// 为heap.allspans分配空间, 用来存储所有span对象指针
// caller: runtime·MHeap_Init()
// 这个函数只在其他地址出现了一次, 但并不是只调用一次, 
// 因为ta是作为成员函数被嵌入了heap->spanalloc对象
// 实际会在runtime·FixAlloc_Alloc()函数中调用. 调用时, 
// 参数vh是FixAlloc对象的args成员地址(按照ta唯一一次初始化所写的, vh其实是mheap的地址), 
// 参数p是FixAlloc对象的chunk成员地址(chunk类型其实是byte*, 这里直接当作MSpan*来用了).
static void
RecordSpan(void *vh, byte *p)
{
	MHeap *h;
	MSpan *s;
	MSpan **all;
	uint32 cap;

	h = vh;
	s = (MSpan*)p;
	// 内存不⾜时(allspans指针空间不足), 重新申请, 并将原数据转移过来.
	if(h->nspan >= h->nspancap) {
		// ...总空间64k?
		cap = 64*1024/sizeof(all[0]);
		// 扩容到1.5倍
		if(cap < h->nspancap*3/2) cap = h->nspancap*3/2;

		all = (MSpan**)runtime·SysAlloc(cap*sizeof(all[0]), &mstats.other_sys);

		if(all == nil) runtime·throw("runtime: cannot allocate memory");
		// 如果h->allspans不为空, 这是要先释放掉?
		if(h->allspans) {
			runtime·memmove(all, h->allspans, h->nspancap*sizeof(all[0]));
			runtime·SysFree(h->allspans, h->nspancap*sizeof(all[0]), &mstats.other_sys);
		}
		h->allspans = all;
		h->nspancap = cap;
	}
	h->allspans[h->nspan++] = s;
}

// Initialize the heap; fetch memory using alloc.
// 初始化堆内存, 使用alloc从OS申请初始内存. 
// 构造mheap的free, large成员为初始的双向循环链表, 但实际只分配了一个表头, 没有真正的分配空间.
// 然后就是调用runtime·MCentral_Init()初始化mcentral成员.
// caller: malloc.goc -> runtime·mallocinit()
// @param: *h runtime·mheap对象指针
void
runtime·MHeap_Init(MHeap *h)
{
	uint32 i;
	// FixAlloc_Init()位于mfixalloc.c
	runtime·FixAlloc_Init(&h->spanalloc, sizeof(MSpan), RecordSpan, h, &mstats.mspan_sys);
	runtime·FixAlloc_Init(&h->cachealloc, sizeof(MCache), nil, nil, &mstats.mcache_sys);
	// h->mapcache needs no init
	// runtime·MSpanList_Init()只是将目标对象初始化为空的双向链表, 没有额外操作.
	// 按照MHeap中对free的定义: `MSpan free[MaxMHeapList]`, free数组可容纳256个双向链表.
	for(i=0; i<nelem(h->free); i++) runtime·MSpanList_Init(&h->free[i]);

	runtime·MSpanList_Init(&h->large);
	// h->central与h->free的作用差不多, 类型也差不多, 只不过为对象分配空间时, h->free的优先级更高.
	for(i=0; i<nelem(h->central); i++) runtime·MCentral_Init(&h->central[i], i);
}

void
runtime·MHeap_MapSpans(MHeap *h)
{
	uintptr n;

	// Map spans array, PageSize at a time.
	n = (uintptr)h->arena_used;
	if(sizeof(void*) == 8)
		n -= (uintptr)h->arena_start;
	n = n / PageSize * sizeof(h->spans[0]);
	n = ROUND(n, PageSize);
	if(h->spans_mapped >= n)
		return;
	runtime·SysMap((byte*)h->spans + h->spans_mapped, n - h->spans_mapped, &mstats.other_sys);
	h->spans_mapped = n;
}

// Allocate a new span of npage pages from the heap
// and record its size class in the HeapMap and HeapMapCache.
// 从heap分配一组大小为npage个页的空间, 
// 并且在HeapMap和HeapMapCache记录其size等级...
// 超过32k的对象才从heap分配, 哪还有size等级???
// 好吧, 在runtime·mallocgc()中调用时的确是为大对象分配空间, 而且sizeclass值指定为0.
// caller: runtime·mallocgc(), MCentral_Grow()
MSpan*
runtime·MHeap_Alloc(MHeap *h, uintptr npage, int32 sizeclass, int32 acct, int32 zeroed)
{
	MSpan *s;
	// 由于heap是被所有线程共享的, 所以对heap的操作需要加锁.
	runtime·lock(h);
	// ...从mcache分配小于32k对象时会记录在mcahe的local_cachealloc, 但这里是什么意思? 
	// 是把从mcache中分配的空间全部记算到mheap的头上, 之后归还空间的时候直接还给mheap?
	// 好像也不是不可以, 毕竟从mcache分配空间时, 直接把内存块从mcache下链表中移除了...
	mstats.heap_alloc += m->mcache->local_cachealloc;
	m->mcache->local_cachealloc = 0;

	s = MHeap_AllocLocked(h, npage, sizeclass);
	if(s != nil) {
		mstats.heap_inuse += npage<<PageShift;
		if(acct) {
			mstats.heap_objects++;
			mstats.heap_alloc += npage<<PageShift;
		}
	}
	runtime·unlock(h);
	if(s != nil && *(uintptr*)(s->start<<PageShift) != 0 && zeroed)
		runtime·memclr((byte*)(s->start<<PageShift), s->npages<<PageShift);
	return s;
}

// 只有一处调用, 看来是在runtime·MHeap_Alloc()先加锁, 实际的操作在这里啊.
// caller: MHeap_AllocLocked()
static MSpan*
MHeap_AllocLocked(MHeap *h, uintptr npage, int32 sizeclass)
{
	uintptr n;
	MSpan *s, *t;
	PageID p;

	// Try in fixed-size lists up to max.
	for(n=npage; n < nelem(h->free); n++) {
		// 如果当前span链表不为空.
		if(!runtime·MSpanList_IsEmpty(&h->free[n])) {
			s = h->free[n].next;
			goto HaveSpan;
		}
	}

	// Best fit in list of large spans.
	if((s = MHeap_AllocLarge(h, npage)) == nil) {
		if(!MHeap_Grow(h, npage)) return nil;
		if((s = MHeap_AllocLarge(h, npage)) == nil) return nil;
	}

HaveSpan:
	// Mark span in use.
	// 标记该span对象为MSpanInUse状态, 并从free的这个span链表中移除.
	if(s->state != MSpanFree) runtime·throw("MHeap_AllocLocked - MSpan not free");
	if(s->npages < npage) runtime·throw("MHeap_AllocLocked - bad npages");
	runtime·MSpanList_Remove(s);
	s->state = MSpanInUse;

	// 记录此次分配的内存空间数据变化
	mstats.heap_idle -= s->npages<<PageShift;
	mstats.heap_released -= s->npreleased<<PageShift;
	if(s->npreleased > 0) {
		// We have called runtime·SysUnused with these pages, 
		// and on Unix systems it called madvise. 
		// 我们对这些页已经调用过runtime·SysUnused()处理, 在Unix系统中其实就是简单的调用了runtime·madvise()
		// At this point at least some BSD-based kernels will return these pages 
		// either as zeros or with the old data.  
		// For our caller, the first word in the page indicates whether the span contains zeros or not 
		// (this word was set when the span was freed by MCentral_Free or runtime·MCentral_FreeSpan). 
		// 对我们的调用者而言, 页的第一个字(word)表示该span的内容是否被清零
		// (这个字(word)在对span对象调用MCentral_Free()或runtime·MCentral_FreeSpan()释放时设置)
		// If the first page in the span is returned as zeros, 
		// and some subsequent page is returned with the old data, 
		// then we will be returning a span that is assumed to be all zeros, 
		// but the actual data will not be all zeros. 
		// 如果span的第一页存放的是0值, 即使之后的页却仍然保留着旧数据, 
		// 那我们也将假设返回的span已经被清零过, 实际上并不一定是.
		// Avoid that problem by explicitly marking the span as not being zeroed, just in case. 
		// 以防万一, 我们显式得将span对象标记为未清零状态, 以避免这个问题.
		// The beadbead constant we use here means nothing, 
		// it is just a unique constant not seen elsewhere in the runtime, 
		// as a clue in case it turns up unexpectedly in memory or in a stack trace.
		// beadbead常量没什么特殊的含义, 只是在runtime时的一个唯一值, 
		// 在出现内存异常或是在堆栈跟踪时可以作为一个排查线索.
		runtime·SysUsed((void*)(s->start<<PageShift), s->npages<<PageShift);
		*(uintptr*)(s->start<<PageShift) = (uintptr)0xbeadbeadbeadbeadULL;
	}
	s->npreleased = 0;

	// 找到的span对象包含的页数可能多于期望值, 此时需要将多余的页放回heap.
	if(s->npages > npage) {
		// Trim extra and put it back in the heap.
		// mheap->spanalloc在runtime·FixAlloc_Init()时指定了first函数为RecordSpan, 
		// 此时就可以用上了.
		t = runtime·FixAlloc_Alloc(&h->spanalloc);
		runtime·MSpan_Init(t, s->start + npage, s->npages - npage);
		s->npages = npage;
		p = t->start;
		if(sizeof(void*) == 8) p -= ((uintptr)h->arena_start>>PageShift);
		if(p > 0) h->spans[p-1] = s;
		h->spans[p] = t;
		h->spans[p+t->npages-1] = t;
		*(uintptr*)(t->start<<PageShift) = *(uintptr*)(s->start<<PageShift);  // copy "needs zeroing" mark
		t->state = MSpanInUse;
		MHeap_FreeLocked(h, t);
		t->unusedsince = s->unusedsince; // preserve age
	}
	s->unusedsince = 0;

	// Record span info, because gc needs to be able to map interior pointer to containing span.
	s->sizeclass = sizeclass;
	s->elemsize = (sizeclass==0 ? s->npages<<PageShift : runtime·class_to_size[sizeclass]);
	s->types.compression = MTypes_Empty;
	p = s->start;
	if(sizeof(void*) == 8) p -= ((uintptr)h->arena_start>>PageShift);
	for(n=0; n<npage; n++) h->spans[p+n] = s;
	return s;
}

// Allocate a span of exactly npage pages from the list of large spans.
static MSpan*
MHeap_AllocLarge(MHeap *h, uintptr npage)
{
	return BestFit(&h->large, npage, nil);
}

// Search list for smallest span with >= npage pages.
// If there are multiple smallest spans, take the one
// with the earliest starting address.
static MSpan*
BestFit(MSpan *list, uintptr npage, MSpan *best)
{
	MSpan *s;

	for(s=list->next; s != list; s=s->next) {
		if(s->npages < npage)
			continue;
		if(best == nil
		|| s->npages < best->npages
		|| (s->npages == best->npages && s->start < best->start))
			best = s;
	}
	return best;
}

// Try to add at least npage pages of memory to the heap,
// returning whether it worked.
// 尝试增加至少npage个页的内存空间给目标堆
// 返回是否成功
// ...这是堆容量的时候要增长的?
static bool
MHeap_Grow(MHeap *h, uintptr npage)
{
	uintptr ask;
	void *v;
	MSpan *s;
	PageID p;

	// Ask for a big chunk, to reduce the number of mappings
	// the operating system needs to track; also amortizes
	// the overhead of an operating system mapping.
	// Allocate a multiple of 64kB (16 pages).
	npage = (npage+15)&~15;
	ask = npage<<PageShift;
	if(ask < HeapAllocChunk)
		ask = HeapAllocChunk;

	v = runtime·MHeap_SysAlloc(h, ask);
	if(v == nil) {
		if(ask > (npage<<PageShift)) {
			ask = npage<<PageShift;
			v = runtime·MHeap_SysAlloc(h, ask);
		}
		if(v == nil) {
			runtime·printf("runtime: out of memory: cannot allocate %D-byte block (%D in use)\n", (uint64)ask, mstats.heap_sys);
			return false;
		}
	}

	// Create a fake "in use" span and free it, so that the
	// right coalescing happens.
	s = runtime·FixAlloc_Alloc(&h->spanalloc);
	runtime·MSpan_Init(s, (uintptr)v>>PageShift, ask>>PageShift);
	p = s->start;
	if(sizeof(void*) == 8)
		p -= ((uintptr)h->arena_start>>PageShift);
	h->spans[p] = s;
	h->spans[p + s->npages - 1] = s;
	s->state = MSpanInUse;
	MHeap_FreeLocked(h, s);
	return true;
}

// Look up the span at the given address.
// Address is guaranteed to be in map
// and is guaranteed to be start or end of span.
MSpan*
runtime·MHeap_Lookup(MHeap *h, void *v)
{
	uintptr p;
	
	p = (uintptr)v;
	if(sizeof(void*) == 8)
		p -= (uintptr)h->arena_start;
	return h->spans[p >> PageShift];
}

// Look up the span at the given address.
// Address is *not* guaranteed to be in map
// and may be anywhere in the span.
// Map entries for the middle of a span are only
// valid for allocated spans.  Free spans may have
// other garbage in their middles, so we have to
// check for that.
MSpan*
runtime·MHeap_LookupMaybe(MHeap *h, void *v)
{
	MSpan *s;
	PageID p, q;

	if((byte*)v < h->arena_start || (byte*)v >= h->arena_used)
		return nil;
	p = (uintptr)v>>PageShift;
	q = p;
	if(sizeof(void*) == 8)
		q -= (uintptr)h->arena_start >> PageShift;
	s = h->spans[q];
	if(s == nil || p < s->start || v >= s->limit || s->state != MSpanInUse)
		return nil;
	return s;
}

// Free the span back into the heap.
void
runtime·MHeap_Free(MHeap *h, MSpan *s, int32 acct)
{
	runtime·lock(h);
	mstats.heap_alloc += m->mcache->local_cachealloc;
	m->mcache->local_cachealloc = 0;
	mstats.heap_inuse -= s->npages<<PageShift;
	if(acct) {
		mstats.heap_alloc -= s->npages<<PageShift;
		mstats.heap_objects--;
	}
	MHeap_FreeLocked(h, s);
	runtime·unlock(h);
}

static void
MHeap_FreeLocked(MHeap *h, MSpan *s)
{
	uintptr *sp, *tp;
	MSpan *t;
	PageID p;

	s->types.compression = MTypes_Empty;

	if(s->state != MSpanInUse || s->ref != 0) {
		runtime·printf("MHeap_FreeLocked - span %p ptr %p state %d ref %d\n", s, s->start<<PageShift, s->state, s->ref);
		runtime·throw("MHeap_FreeLocked - invalid free");
	}
	mstats.heap_idle += s->npages<<PageShift;
	s->state = MSpanFree;
	runtime·MSpanList_Remove(s);
	sp = (uintptr*)(s->start<<PageShift);
	// Stamp newly unused spans. The scavenger will use that
	// info to potentially give back some pages to the OS.
	s->unusedsince = runtime·nanotime();
	s->npreleased = 0;

	// Coalesce with earlier, later spans.
	p = s->start;
	if(sizeof(void*) == 8)
		p -= (uintptr)h->arena_start >> PageShift;
	if(p > 0 && (t = h->spans[p-1]) != nil && t->state != MSpanInUse) {
		if(t->npreleased == 0) {  // cant't touch this otherwise
			tp = (uintptr*)(t->start<<PageShift);
			*tp |= *sp;	// propagate "needs zeroing" mark
		}
		s->start = t->start;
		s->npages += t->npages;
		s->npreleased = t->npreleased; // absorb released pages
		p -= t->npages;
		h->spans[p] = s;
		runtime·MSpanList_Remove(t);
		t->state = MSpanDead;
		runtime·FixAlloc_Free(&h->spanalloc, t);
	}
	if((p+s->npages)*sizeof(h->spans[0]) < h->spans_mapped && (t = h->spans[p+s->npages]) != nil && t->state != MSpanInUse) {
		if(t->npreleased == 0) {  // cant't touch this otherwise
			tp = (uintptr*)(t->start<<PageShift);
			*sp |= *tp;	// propagate "needs zeroing" mark
		}
		s->npages += t->npages;
		s->npreleased += t->npreleased;
		h->spans[p + s->npages - 1] = s;
		runtime·MSpanList_Remove(t);
		t->state = MSpanDead;
		runtime·FixAlloc_Free(&h->spanalloc, t);
	}

	// Insert s into appropriate list.
	if(s->npages < nelem(h->free))
		runtime·MSpanList_Insert(&h->free[s->npages], s);
	else
		runtime·MSpanList_Insert(&h->large, s);
}

static void
forcegchelper(Note *note)
{
	runtime·gc(1);
	runtime·notewakeup(note);
}

static uintptr
scavengelist(MSpan *list, uint64 now, uint64 limit)
{
	uintptr released, sumreleased;
	MSpan *s;

	if(runtime·MSpanList_IsEmpty(list))
		return 0;

	sumreleased = 0;
	for(s=list->next; s != list; s=s->next) {
		if((now - s->unusedsince) > limit && s->npreleased != s->npages) {
			released = (s->npages - s->npreleased) << PageShift;
			mstats.heap_released += released;
			sumreleased += released;
			s->npreleased = s->npages;
			runtime·SysUnused((void*)(s->start << PageShift), s->npages << PageShift);
		}
	}
	return sumreleased;
}

static void
scavenge(int32 k, uint64 now, uint64 limit)
{
	uint32 i;
	uintptr sumreleased;
	MHeap *h;
	
	h = &runtime·mheap;
	sumreleased = 0;
	for(i=0; i < nelem(h->free); i++)
		sumreleased += scavengelist(&h->free[i], now, limit);
	sumreleased += scavengelist(&h->large, now, limit);

	if(runtime·debug.gctrace > 0) {
		if(sumreleased > 0)
			runtime·printf("scvg%d: %D MB released\n", k, (uint64)sumreleased>>20);
		runtime·printf("scvg%d: inuse: %D, idle: %D, sys: %D, released: %D, consumed: %D (MB)\n",
			k, mstats.heap_inuse>>20, mstats.heap_idle>>20, mstats.heap_sys>>20,
			mstats.heap_released>>20, (mstats.heap_sys - mstats.heap_released)>>20);
	}
}

static FuncVal forcegchelperv = {(void(*)(void))forcegchelper};

// Release (part of) unused memory to OS.
// Goroutine created at startup.
// Loop forever.
void
runtime·MHeap_Scavenger(void)
{
	MHeap *h;
	uint64 tick, now, forcegc, limit;
	int32 k;
	Note note, *notep;

	g->issystem = true;
	g->isbackground = true;

	// If we go two minutes without a garbage collection, force one to run.
	forcegc = 2*60*1e9;
	// If a span goes unused for 5 minutes after a garbage collection,
	// we hand it back to the operating system.
	limit = 5*60*1e9;
	// Make wake-up period small enough for the sampling to be correct.
	if(forcegc < limit)
		tick = forcegc/2;
	else
		tick = limit/2;

	h = &runtime·mheap;
	for(k=0;; k++) {
		runtime·noteclear(&note);
		runtime·notetsleepg(&note, tick);

		runtime·lock(h);
		now = runtime·nanotime();
		if(now - mstats.last_gc > forcegc) {
			runtime·unlock(h);
			// The scavenger can not block other goroutines,
			// otherwise deadlock detector can fire spuriously.
			// GC blocks other goroutines via the runtime·worldsema.
			runtime·noteclear(&note);
			notep = &note;
			runtime·newproc1(&forcegchelperv, (byte*)&notep, sizeof(notep), 0, runtime·MHeap_Scavenger);
			runtime·notetsleepg(&note, -1);
			if(runtime·debug.gctrace > 0)
				runtime·printf("scvg%d: GC forced\n", k);
			runtime·lock(h);
			now = runtime·nanotime();
		}
		scavenge(k, now, limit);
		runtime·unlock(h);
	}
}

void
runtime∕debug·freeOSMemory(void)
{
	runtime·gc(1);
	runtime·lock(&runtime·mheap);
	scavenge(-1, ~(uintptr)0, 0);
	runtime·unlock(&runtime·mheap);
}

// Initialize a new span with the given start and npages.
void
runtime·MSpan_Init(MSpan *span, PageID start, uintptr npages)
{
	span->next = nil;
	span->prev = nil;
	span->start = start;
	span->npages = npages;
	span->freelist = nil;
	span->ref = 0;
	span->sizeclass = 0;
	span->elemsize = 0;
	span->state = 0;
	span->unusedsince = 0;
	span->npreleased = 0;
	span->types.compression = MTypes_Empty;
}

// Initialize an empty doubly-linked list.
// 初始化空的双向链表
void
runtime·MSpanList_Init(MSpan *list)
{
	list->state = MSpanListHead;
	list->next = list;
	list->prev = list;
}

void
runtime·MSpanList_Remove(MSpan *span)
{
	if(span->prev == nil && span->next == nil)
		return;
	span->prev->next = span->next;
	span->next->prev = span->prev;
	span->prev = nil;
	span->next = nil;
}

bool
runtime·MSpanList_IsEmpty(MSpan *list)
{
	return list->next == list;
}

void
runtime·MSpanList_Insert(MSpan *list, MSpan *span)
{
	if(span->next != nil || span->prev != nil) {
		runtime·printf("failed MSpanList_Insert %p %p %p\n", span, span->next, span->prev);
		runtime·throw("MSpanList_Insert");
	}
	span->next = list->next;
	span->prev = list;
	span->next->prev = span;
	span->prev->next = span;
}


