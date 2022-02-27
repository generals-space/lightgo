// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fixed-size object allocator.  Returned memory is not zeroed.
// 固定大小对象的分配器. 其返回的内存空间未清零
// See malloc.h for overview.

#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"

// 初始化 f 以分配指定大小 size 的对象, 其实就是为 f 对象的各个成员字段赋值, 其他的啥也没做.
//
// 实际的分配行为在下面的 runtime·FixAlloc_Alloc() 方法中完成.
//
// param f: MHeap->spanalloc, 或 MHeap->cachealloc
// param size: 在主调函数中, 分别被传入了 sizeof(MSpan), sizeof(MCache)
// param void (*first)(void*, byte*): 函数类型, *first为函数地址, (void*, byte*)为ta的两个参数
//
// caller: 
// 	1. src/pkg/runtime/mheap.c -> runtime·MHeap_Init() 只有这一处
//
// Initialize f to allocate objects of the given size,
// using the allocator to obtain chunks of memory.
void
runtime·FixAlloc_Init(FixAlloc *f, uintptr size, void (*first)(void*, byte*), void *arg, uint64 *stat)
{
	f->size = size;
	f->first = first;
	f->arg = arg;
	// 注意这里的值为 nil, 说明这是 golang 语法.
	f->list = nil; 
	f->chunk = nil;
	f->nchunk = 0;
	f->inuse = 0;
	f->stat = stat;
}


// param f: MHeap->spanalloc, 或 MHeap->cachealloc
//
// return: 返回值为 void* 类型, 但一般会被转换成 MSpan* 或 MCache* 类型.
//
// caller: 
// 	1. src/pkg/runtime/malloc.goc -> runtime·allocmcache() 初始化内存分配器时被调用.
//     参数为 MHeap->cachealloc
//     在此阶段, f 对象基本为经过上面 runtime·FixAlloc_Init() 函数初始化后的结果.
// 	2. src/pkg/runtime/mheap.c -> MHeap_AllocLocked()
//     参数为 MHeap->spanalloc
// 	3. src/pkg/runtime/mheap.c -> MHeap_Grow()
//     参数为 MHeap->spanalloc
void*
runtime·FixAlloc_Alloc(FixAlloc *f)
{
	void *v;

	if(f->size == 0) {
		runtime·printf("runtime: use of FixAlloc_Alloc before FixAlloc_Init\n");
		runtime·throw("runtime: internal error");
	}
	// 如果 list 不为空, 就直接拿一个内存块返回.
	//
	// 主调函数为 runtime·allocmcache() 时, list = nil.
	if(f->list) {
		v = f->list;
		f->list = *(void**)f->list;
		f->inuse += f->size;
		return v;
	}
	// 如果 list 为空, 就把焦点转移到 chunk 成员上去.

	// 如果 chunk 剩余的空间不足, 就从 OS 再申请 FixAllocChunk 大小的空间, 并重新计算.
	if(f->nchunk < f->size) {
		f->chunk = runtime·persistentalloc(FixAllocChunk, 0, f->stat);
		f->nchunk = FixAllocChunk;
	}

	v = f->chunk;
	// 从 chunk 中获取 size 大小的空间, 并记录.
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

