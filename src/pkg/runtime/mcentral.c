// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Central free lists.
//
// See malloc.h for an overview.
//
// The MCentral doesn't actually contain the list of free objects; the MSpan does.
// Each MCentral is two lists of MSpans: those with free objects (c->nonempty)
// and those that are completely allocated (c->empty).
//
// TODO(rsc): tcmalloc uses a "transfer cache" to split the list
// into sections of class_to_transfercount[sizeclass] objects
// so that it is faster to move those lists between MCaches and MCentrals.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "malloc.h"

static bool MCentral_Grow(MCentral *c);
static void MCentral_Free(MCentral *c, void *v);

// Initialize a single central free list.
// 初始化mcentral的两个空的链表
// 只被调用这一次, 不过是一个for循环, 用来初始化堆中各size等级的mcentral.
// caller: runtime·MHeap_Init() 
void
runtime·MCentral_Init(MCentral *c, int32 sizeclass)
{
	c->sizeclass = sizeclass;
	runtime·MSpanList_Init(&c->nonempty);
	runtime·MSpanList_Init(&c->empty);
}

// Allocate a list of objects from the central free list.
// Return the number of objects allocated.
// The objects are linked together by their first words.
// On return, *pfirst points at the first object.
// 从mcentral的空闲列表中分配一批对象, 返回对象的个数.
// 这些对象组成一个链表, 返回时pfirst指向第一个对象的地址.
// 这个函数只被runtime·MCache_Refill()调用, 
// 是在线程的mcache没有多余的空间来分配对象时, 尝试从mcentral获取一批空闲空间.
// ...从传入参数来看, 好像没说要获取多少? 看心情吗?
// ...继续分析得出, 此函数直接从mcentral的noempty链表中拿出一个span,
// 并将pfirst指向这个span的freelist, 
// 再把这个span放到empty链表中(freelist都分配出去了, 相当于没有剩余空间了), 就完事了.
// caller: runtime·MCache_Refill()
int32
runtime·MCentral_AllocList(MCentral *c, MLink **pfirst)
{
	MSpan *s;
	int32 cap, n;
	// mcentral被多线程共享, 所以从mcentral获取内存时需要加锁
	runtime·lock(c);
	// Replenish central list if empty.
	// 如果mcentral中也没有空闲空间时, 就从堆中获取, 补充自身.
	if(runtime·MSpanList_IsEmpty(&c->nonempty)) {
		if(!MCentral_Grow(c)) {
			runtime·unlock(c);
			*pfirst = nil;
			return 0;
		}
	}

	s = c->nonempty.next;
	// cap 总共可容纳的object的个数
	cap = (s->npages << PageShift) / s->elemsize;
	// n 空闲的object的个数
	n = cap - s->ref;
	*pfirst = s->freelist;
	s->freelist = nil;
	s->ref += n;
	c->nfree -= n;
	runtime·MSpanList_Remove(s);
	runtime·MSpanList_Insert(&c->empty, s);
	runtime·unlock(c);
	return n;
}

// Free the list of objects back into the central free list.
void
runtime·MCentral_FreeList(MCentral *c, MLink *start)
{
	MLink *next;

	runtime·lock(c);
	for(; start != nil; start = next) {
		next = start->next;
		MCentral_Free(c, start);
	}
	runtime·unlock(c);
}

// Helper: free one object back into the central free list.
static void
MCentral_Free(MCentral *c, void *v)
{
	MSpan *s;
	MLink *p;
	int32 size;

	// Find span for v.
	s = runtime·MHeap_Lookup(&runtime·mheap, v);
	if(s == nil || s->ref == 0)
		runtime·throw("invalid free");

	// Move to nonempty if necessary.
	if(s->freelist == nil) {
		runtime·MSpanList_Remove(s);
		runtime·MSpanList_Insert(&c->nonempty, s);
	}

	// Add v back to s's free list.
	p = v;
	p->next = s->freelist;
	s->freelist = p;
	c->nfree++;

	// If s is completely freed, return it to the heap.
	if(--s->ref == 0) {
		size = runtime·class_to_size[c->sizeclass];
		runtime·MSpanList_Remove(s);
		runtime·unmarkspan((byte*)(s->start<<PageShift), s->npages<<PageShift);
		*(uintptr*)(s->start<<PageShift) = 1;  // needs zeroing
		s->freelist = nil;
		c->nfree -= (s->npages << PageShift) / size;
		runtime·unlock(c);
		runtime·MHeap_Free(&runtime·mheap, s, 0);
		runtime·lock(c);
	}
}

// Free n objects from a span s back into the central free list c.
// 释放目标span s中的n个对象, 将空间回收到mcentral的空闲列表c中.
// 记得mcentral也是有size等级的, 且调用者必然保证了, 与span分配的对象的size等级相同.
// caller: gc -> sweepspan()
void
runtime·MCentral_FreeSpan(MCentral *c, MSpan *s, int32 n, MLink *start, MLink *end)
{
	int32 size;
	// 话说此时应该处于gc过程中, 应该是经过了STW的, 
	// 当前应该只有一个线程在运行, 为什么还要加锁呢?
	runtime·lock(c);

	// Move to nonempty if necessary.
	// 如果该span完全被分配(freelist中没有元素), 相当于可以全部回收.
	// 实际操作就是, 将这个span从emptyq链表中移除, 再放回到noempty链表中.
	if(s->freelist == nil) {
		runtime·MSpanList_Remove(s);
		runtime·MSpanList_Insert(&c->nonempty, s);
	}

	// Add the objects back to s's free list.
	// 运行到这里, 说该span中有一部分被分配, 但不是完全被分配出去了.
	// 这里的操作就是把可回收的mlink链表附加到该span的freelist中.
	end->next = s->freelist;
	s->freelist = start;
	s->ref -= n;
	c->nfree += n;

	// If s is completely freed, return it to the heap.
	// 如果该span完全空闲, 将其归还到heap.
	// ref表示该span中已分配的对象数量.
	// 否则, 上面的操作已经够了, 直接解锁返回.
	if(s->ref == 0) {
		// 话说, 完全空闲是一种什么情况???
		// 既然是分配过的, 却又没有占用的对象...这不白干了吗?
		size = runtime·class_to_size[c->sizeclass];
		runtime·MSpanList_Remove(s); // 将这个span从emptyq链表中移除
		// 这一步是将span直接归还给heap, 
		// 所以下面的操作基本相当于销毁span在mcentral中的痕迹, 只把空间归还即可.
		// needs zeroing
		// 把该span的第一页的内容赋值为1...这是相当于清零? 能这么清???
		*(uintptr*)(s->start<<PageShift) = 1;
		s->freelist = nil;
		c->nfree -= (s->npages << PageShift) / size;
		runtime·unlock(c);
		// 将此span下的页块标记全部取消标记
		runtime·unmarkspan((byte*)(s->start<<PageShift), s->npages<<PageShift);
		runtime·MHeap_Free(&runtime·mheap, s, 0);
	} else {
		runtime·unlock(c);
	}
}

void
runtime·MGetSizeClassInfo(int32 sizeclass, uintptr *sizep, int32 *npagesp, int32 *nobj)
{
	int32 size;
	int32 npages;

	npages = runtime·class_to_allocnpages[sizeclass];
	size = runtime·class_to_size[sizeclass];
	*npagesp = npages;
	*sizep = size;
	*nobj = (npages << PageShift) / size;
}

// Fetch a new span from the heap and
// carve into objects for the free list.
// 从 heap 处取一个 span 块, 并切分成对象块, 并组成空闲链表备用.
static bool
MCentral_Grow(MCentral *c)
{
	int32 i, n, npages;
	uintptr size;
	MLink **tailp, *v;
	byte *p;
	MSpan *s;

	runtime·unlock(c);
	runtime·MGetSizeClassInfo(c->sizeclass, &size, &npages, &n);
	s = runtime·MHeap_Alloc(&runtime·mheap, npages, c->sizeclass, 0, 1);
	if(s == nil) {
		// TODO(rsc): Log out of memory
		runtime·lock(c);
		return false;
	}

	// Carve span into sequence of blocks.
	tailp = &s->freelist;
	p = (byte*)(s->start << PageShift);
	s->limit = p + size*n;
	for(i=0; i<n; i++) {
		v = (MLink*)p;
		*tailp = v;
		tailp = &v->next;
		p += size;
	}
	*tailp = nil;
	runtime·markspan((byte*)(s->start<<PageShift), size, n, size*n < (s->npages<<PageShift));

	runtime·lock(c);
	c->nfree += n;
	runtime·MSpanList_Insert(&c->nonempty, s);
	return true;
}
