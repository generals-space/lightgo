// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fixed-size object allocator.  Returned memory is not zeroed.
// 固定大小对象的分配器. 其返回的内存空间未清零
// See malloc.h for overview.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "malloc.h"

// Initialize f to allocate objects of the given size,
// using the allocator to obtain chunks of memory.
// 初始化f以分配指定大小size的对象(创建f就是为了给size大小的对象分配空间)
// caller: mheap.c -> runtime·MHeap_Init()
// 第3个参数void (*first)(void*, byte*)是一函数类型 
// *first为函数地址, (void*, byte*)为ta的两个参数
void
runtime·FixAlloc_Init(FixAlloc *f, uintptr size, void (*first)(void*, byte*), void *arg, uint64 *stat)
{
	f->size = size;
	f->first = first;
	f->arg = arg;
	f->list = nil;
	f->chunk = nil;
	f->nchunk = 0;
	f->inuse = 0;
	f->stat = stat;
}

void*
runtime·FixAlloc_Alloc(FixAlloc *f)
{
	void *v;
	
	if(f->size == 0) {
		runtime·printf("runtime: use of FixAlloc_Alloc before FixAlloc_Init\n");
		runtime·throw("runtime: internal error");
	}
	// 如果list不为空, 就直接拿一个内存块返回.
	if(f->list) {
		v = f->list;
		f->list = *(void**)f->list;
		f->inuse += f->size;
		return v;
	}
	// 如果list为空, 就把焦点转移到chunk上去.

	// 如果chunk剩余的空间不足, 就从操作系统再申请FixAllocChunk大小的空间, 并重新计算.
	if(f->nchunk < f->size) {
		f->chunk = runtime·persistentalloc(FixAllocChunk, 0, f->stat);
		f->nchunk = FixAllocChunk;
	}

	v = f->chunk;
	// 从chunk中获取size大小的空间, 并记录.
	// 只有在初始化mheap->spanalloc时指定过first函数, 且arg参数为mheap本身.
	if(f->first) f->first(f->arg, v);

	// 记录分配数据
	f->chunk += f->size;
	f->nchunk -= f->size;
	f->inuse += f->size;
	
	return v;
}

void
runtime·FixAlloc_Free(FixAlloc *f, void *p)
{
	f->inuse -= f->size;
	*(void**)p = f->list;
	f->list = p;
}

