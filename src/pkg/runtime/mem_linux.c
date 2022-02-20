// Copyright 2010 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "defs_GOOS_GOARCH.h"
#include "os_GOOS.h"
#include "malloc.h"

enum
{
	_PAGE_SIZE = 4096,
};

// 判断从地址v开始处, 大小为n的内存空间是否空闲...?
// 返回0或1
static int32
addrspace_free(void *v, uintptr n)
{
	int32 errval;
	uintptr chunk;
	uintptr off;
	static byte vec[4096];

	for(off = 0; off < n; off += chunk) {
		chunk = _PAGE_SIZE * sizeof vec;

		if(chunk > (n - off)) chunk = n - off;
		errval = runtime·mincore((int8*)v + off, chunk, vec);

		// errval is 0 if success, or -(error_code) if error.
		if (errval == 0 || errval != -ENOMEM) return 0;
	}
	return 1;
}

// 从OS申请从地址v开始处, 大小为n的空间.
//
// 该函数的所有参数几乎都是主调函数直接传入 runtime·mmap() 的, 
// 猜测 runtime·mmap() 虽然能够分配内存, 但不一定是在地址v处, 
// mmap_fixed()实现了可以在地址v处分配内存的功能.
//
// caller:
// 	1. runtime·SysReserve()
// 	2. runtime·SysMap()
//
static void *
mmap_fixed(byte *v, uintptr n, int32 prot, int32 flags, int32 fd, uint32 offset)
{
	void *p;

	p = runtime·mmap(v, n, prot, flags, fd, offset);
	if(p != v && addrspace_free(v, n)) {
		// On some systems, mmap ignores v without MAP_FIXED, 
		// so retry if the address space is free.
		// 在某些系统里, 如果flags参数不包含MAP_FIXED标识, mmap操作会忽略传入的v参数,
		// 所以
		if(p > (void*)4096) runtime·munmap(p, n);
		p = runtime·mmap(v, n, prot, flags|MAP_FIXED, fd, offset);
	}
	return p;
}

// 向OS申请(mmap)大小为 n 的内存空间, 并返回该块内存地址.
// 申请成功后将 n 加到 stat 指向的数值变量, 用于记录.
//
// param: stat一般是 &mstats.other_sys
//
// caller:
// 	1. src/pkg/runtime/mheap.c -> RecordSpan() 
void*
runtime·SysAlloc(uintptr n, uint64 *stat)
{
	void *p;

	p = runtime·mmap(nil, n, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if(p < (void*)4096) {
		if(p == (void*)EACCES) {
			runtime·printf("runtime: mmap: access denied\n");
			runtime·printf("if you're running SELinux, enable execmem for this process.\n");
			runtime·exit(2);
		}
		if(p == (void*)EAGAIN) {
			runtime·printf("runtime: mmap: too much locked memory (check 'ulimit -l').\n");
			runtime·exit(2);
		}
		return nil;
	}
	runtime·xadd64(stat, n);
	return p;
}

void
runtime·SysUnused(void *v, uintptr n)
{
	runtime·madvise(v, n, MADV_DONTNEED);
}

// 除了windows, 其他类unix系统如MacOS, netbsd, openbsd等都是如下的USED(), 
// 实际上相当于什么都不做.
void
runtime·SysUsed(void *v, uintptr n)
{
	USED(v);
	USED(n);
}

// 与runtime·SysAlloc()相对, 释放地址v处开始大小为n的内存空间, 并且从stat的数值中减去n, 用于记录
void
runtime·SysFree(void *v, uintptr n, uint64 *stat)
{
	runtime·xadd64(stat, -(uint64)n);
	runtime·munmap(v, n);
}

// 为内存分配器申请预留的虚拟内存地址(并未实际分配, 这个值可以非常大)
//
// param *v: 主调函数计算出来的虚拟内存起始地址;
// param n: 需要申请的内存大小, 256MB spans + 8GB bitmap + 128GB arena;
//
// caller: 
// 	1. src/pkg/runtime/malloc.goc -> runtime·mallocinit() 初始化内存分配器
void*
runtime·SysReserve(void *v, uintptr n)
{
	void *p;

	// On 64-bit, people with ulimit -v set complain 
	// if we reserve too much address space. 
	// Instead, assume that the reservation is okay 
	// if we can reserve at least 64K and check the assumption in SysMap.
	// Only user-mode Linux (UML) rejects these requests.
	if(sizeof(void*) == 8 && (uintptr)v >= 0xffffffffU) {
		p = mmap_fixed(v, 64<<10, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
		if (p != v) {
			if(p >= (void*)4096) runtime·munmap(p, 64<<10);
			return nil;
		}
		runtime·munmap(p, 64<<10);
		return v;
	}

	p = runtime·mmap(v, n, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if((uintptr)p < 4096) return nil;
	return p;
}

void
runtime·SysMap(void *v, uintptr n, uint64 *stat)
{
	void *p;
	
	runtime·xadd64(stat, n);

	// On 64-bit, we don't actually have v reserved, so tread carefully.
	if(sizeof(void*) == 8 && (uintptr)v >= 0xffffffffU) {
		p = mmap_fixed(v, n, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
		if(p == (void*)ENOMEM) runtime·throw("runtime: out of memory");
		if(p != v) {
			runtime·printf("runtime: address space conflict: map(%p) = %p\n", v, p);
			runtime·throw("runtime: address space conflict");
		}
		return;
	}

	p = runtime·mmap(v, n, PROT_READ|PROT_WRITE, MAP_ANON|MAP_FIXED|MAP_PRIVATE, -1, 0);
	if(p == (void*)ENOMEM) runtime·throw("runtime: out of memory");
	if(p != v) runtime·throw("runtime: cannot map pages in arena address space");
}
