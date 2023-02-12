// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fixed-size object allocator.  Returned memory is not zeroed.
// 固定大小对象的分配器. 其返回的内存空间未清零
// See malloc.h for overview.

#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"

// FixAlloc 用于分配固定大小的对象, 即 MCache 与 MSpan, 与开发者代码中创建的各种变量相比,
// 属于 runtime 的这2种对象, 大小是可以预见的, 不需要经过 sizeclass 比对,
// 就可以明确的知道申请多大的空间合适.

// 初始化 f 以分配指定大小 size 的对象(其实就是为 f 对象的各个成员字段赋值, 其他的啥也没做).
//
// 实际的分配行为在下面的 runtime·FixAlloc_Alloc() 方法中完成.
//
// 	@param f: MHeap->spanalloc, 或 MHeap->cachealloc
// 	@param size: 在主调函数中, 分别被传入了 sizeof(MSpan), sizeof(MCache)
// 	@param void (*first)(void*, byte*): 函数类型, *first为函数地址, (void*, byte*)为ta的两个参数
// 	@param arg: first 函数所需的参数, f == spanalloc 时, arg 为 MHeap; f == cachealloc 时, 为 nil.
//
// caller: 
// 	1. src/pkg/runtime/mheap.c -> runtime·MHeap_Init() 只有这一处
//
// Initialize f to allocate objects of the given size,
// using the allocator to obtain chunks of memory.
void
runtime·FixAlloc_Init(
	FixAlloc *f, uintptr size, 
	void (*first)(void*, byte*), void *arg, uint64 *stat
)
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

// runtime·FixAlloc_Alloc
//
// param f: MHeap->spanalloc, 或 MHeap->cachealloc
//
// return: 返回值为 void* 类型, 但一般会被转换成 MSpan* 或 MCache* 类型.
//
// caller: 
// 	1. src/pkg/runtime/malloc.goc -> runtime·allocmcache() 参数为 MHeap->cachealloc
// 	初始化内存分配器时被调用, 为 m0 初始化本地缓存.
// 	在此阶段, f 对象基本为经过上面 runtime·FixAlloc_Init() 函数初始化后的结果.
// 	2. src/pkg/runtime/mheap.c -> MHeap_AllocLocked() 参数为 MHeap->spanalloc
// 	3. src/pkg/runtime/mheap.c -> MHeap_Grow() 参数为 MHeap->spanalloc
// 	runtime·mheap arena 部分的内存扩容成功后(实际内存), 调用该方法分配一个 span 进行验证,
// 	完成后还会释放.
void* runtime·FixAlloc_Alloc(FixAlloc *f)
{
	void *v;
	// runtime·printf(
	// 	"sizeof(MSpan): %d, sizeof(MCache): %d\n", 
	// 	sizeof(MSpan), sizeof(MCache)
	// );
	// runtime·printf(
	// 	"inuse: %D, size: %D, nchunk: %d\n", f->inuse, f->size, f->nchunk
	// );

	if(f->size == 0) {
		runtime·printf("runtime: use of FixAlloc_Alloc before FixAlloc_Init\n");
		runtime·throw("runtime: internal error");
	}
	// 如果 list 不为空, 就直接拿一个内存块返回.
	//
	// 注意: 在 runtime·FixAlloc_Init() 与 runtime·FixAlloc_Alloc() 过程中,
	// 都不存在对 list 成员赋值的情况, list 成员一直都是 nil.
	// 实际上, 只有在 runtime·FixAlloc_Free() 中, "释放" span/cache 对象时, 
	// 才会将ta们归还到 list 链表中, 而不是归还给操作系统.
	if(f->list) {
		v = f->list;
		f->list = *(void**)f->list;
		f->inuse += f->size;
		return v;
	}
	// runtime·printf(
	// 	"inuse: %D, size: %D, nchunk: %d\n", f->inuse, f->size, f->nchunk
	// );

	// 如果 list 为空, 需要申请 chunk 空间, 然后从 chunk 中分配 size 大小的内存.

	// 如果 chunk 剩余的空间不足, 就从 OS 再申请 FixAllocChunk 大小的空间, 并重新计算.
	if(f->nchunk < f->size) {
		f->chunk = runtime·persistentalloc(FixAllocChunk, 0, f->stat);
		f->nchunk = FixAllocChunk;
	}
	// runtime·printf(
	// 	"inuse: %D, size: %D, nchunk: %d\n", f->inuse, f->size, f->nchunk
	// );

	v = f->chunk;
	// 从 chunk 中获取 size 大小的空间, 并记录.

	// f 为 MHeap->spanalloc 时, first 为 src/pkg/runtime/mheap.c -> RecordSpan()
	// f 为 MHeap->cachealloc 时, first 为 nil
	if(f->first) {
		// 即 RecordSpan(runtime·mheap, v)
		f->first(f->arg, v);
	}
	// runtime·printf(
	// 	"inuse: %D, size: %D, nchunk: %d\n", f->inuse, f->size, f->nchunk
	// );

	// 注意: 每次分配 chunk, 都是固定的块, 之后需要从该 chunk 中拿出 size 的大小, 
	// 所以这里 chunk 地址需要向后移 size 个 byte, 然后 nchunk 需要减去 size.
	//
	// 由于 v 返回后直接被赋值为 MSpan/MCache 类型的对象, 所以 f->chunk 中都是空闲空间.
	f->chunk += f->size;
	f->nchunk -= f->size;
	f->inuse += f->size;
	// runtime·printf(
	// 	"inuse: %D, size: %D, nchunk: %d\n", f->inuse, f->size, f->nchunk
	// );

	return v;
}

void
runtime·FixAlloc_Free(FixAlloc *f, void *p)
{
	f->inuse -= f->size;
	*(void**)p = f->list;
	f->list = p;
}
