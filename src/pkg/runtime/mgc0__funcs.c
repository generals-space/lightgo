#include "runtime.h"
#include "arch_amd64.h"

#include "malloc.h"

#include "mgc0.h"
#include "mgc0__funcs.h"
#include "type.h"

#include "mgc0__others.h"

// 获取 GOGC 环境变量, 用于主调函数设置 gcpercent 全局变量.
//
// caller: 
// 	1. runtime·gc()
// 	2. runtime∕debug·setGCPercent()
//
int32 readgogc(void)
{
	byte *p;

	p = runtime·getenv("GOGC");
	if(p == nil || p[0] == '\0'){
		return 100;
	} 
	if(runtime·strcmp(p, (byte*)"off") == 0) {
		return -1;
	}
	return runtime·atoi(p);
}

// caller:
// 	1. runtime·memorydump()
static void dumpspan(uint32 idx)
{
	int32 sizeclass, n, npages, i, column;
	uintptr size;
	byte *p;
	byte *arena_start;
	MSpan *s;
	bool allocated, special;

	s = runtime·mheap.allspans[idx];
	if(s->state != MSpanInUse) {
		return;
	}
	arena_start = runtime·mheap.arena_start;
	p = (byte*)(s->start << PageShift);
	sizeclass = s->sizeclass;
	size = s->elemsize;
	if(sizeclass == 0) {
		n = 1;
	} else {
		npages = runtime·class_to_allocnpages[sizeclass];
		n = (npages << PageShift) / size;
	}
	
	runtime·printf("%p .. %p:\n", p, p+n*size);
	column = 0;
	for(; n>0; n--, p+=size) {
		uintptr off, *bitp, shift, bits;

		off = (uintptr*)p - (uintptr*)arena_start;
		bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
		shift = off % wordsPerBitmapWord;
		bits = *bitp>>shift;

		allocated = ((bits & bitAllocated) != 0);
		special = ((bits & bitSpecial) != 0);

		for(i=0; i<size; i+=sizeof(void*)) {
			if(column == 0) {
				runtime·printf("\t");
			}
			if(i == 0) {
				runtime·printf(allocated ? "(" : "[");
				runtime·printf(special ? "@" : "");
				runtime·printf("%p: ", p+i);
			} else {
				runtime·printf(" ");
			}

			runtime·printf("%p", *(void**)(p+i));

			if(i+sizeof(void*) >= size) {
				runtime·printf(allocated ? ") " : "] ");
			}

			column++;
			if(column == 8) {
				runtime·printf("\n");
				column = 0;
			}
		}
	}
	runtime·printf("\n");
}

// A debugging function to dump the contents of memory
void runtime·memorydump(void)
{
	uint32 spanidx;

	for(spanidx=0; spanidx<runtime·mheap.nspan; spanidx++) {
		dumpspan(spanidx);
	}
}

// 标记从地址v开始, 大小为 n 的内存块为已分配状态.
// 不过, 只设置了 v 在 bitmap 区域中最开头的块的标记, 没有设置后续的描述字.
// 变量 n 只是被用做边界检查, 在设置目标字的"bit位"时, 并没有用到ta.
//
// 	@param v: arena 区域中目标对象的起始地址指针.
//
// caller:
// 	1. src/pkg/runtime/malloc.goc -> runtime·mallocgc() 只有这一处
// 	为新对象在堆上分配空间时调用.
//
// mark the block at v of size n as allocated.
// If noscan is true, mark it as not needing scanning.
void runtime·markallocated(void *v, uintptr n, bool noscan)
{
	uintptr *b, obits, bits, off, shift;

	if(0) {
		runtime·printf("markallocated %p+%p\n", v, n);
	}

	// 如果 v+n 的区域超出了 arena 部分, 则抛出异常.
	if((byte*)v+n > (byte*)runtime·mheap.arena_used || (byte*)v < runtime·mheap.arena_start) {
		runtime·throw("markallocated: bad pointer");
	}

	// 下面几行计算语句, 根据 obj 对象地址, 反推其在 bitmap 中描述字的地址, 详细解释可见:
	// <!link!>: {32a3d702-70db-4cae-852b-5c12ce491afc}
	//
	// word offset
	off = (uintptr*)v - (uintptr*)runtime·mheap.arena_start;
	// b 为 v 对象在 bitmap 区域中描述字(word)信息所在的地址指针.
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	// 无限循环, 直到设置成功才结束(主要是应对多协程运行时, casp 的原子操作).
	for(;;) {
		// 取出在地址 b 处的描述字(word)的值(obits = old/original bits).
		//
		// 为了应对多协程, 从 b 指针处取值都要放到 for{} 循环中, 否则获取到的值与实际值会有出入.
		obits = *b;
		// (obits & ~(bitMask<<shift)) 在不改变其他word的"bit位"的条件下, 对目标字进行清零,
		// | (bitAllocated<<shift) 为目标"bit位"设置 bitAllocated 标记.
		bits = (obits & ~(bitMask<<shift)) | (bitAllocated<<shift);
		// 如果 noscan 为 true, 则设置 bitNoScan 标记
		if(noscan) {
			bits |= bitNoScan<<shift;
		}

		// 如果只有一个P, 可以直接为地址b赋值; 如果有多个, 则需要使用原子操作.
		if(runtime·gomaxprocs == 1) {
			*b = bits;
			break;
		} else {
			// 先比较地址 b 处的值是否与 obits 相同, 如相同则将其替换为 bits(类似于乐观锁)
			//
			// more than one goroutine is potentially running: use atomic op
			if(runtime·casp((void**)b, (void*)obits, (void*)bits)) {
				break;
			}
		}
	}
}

// 标记从地址v开始, 大小为n的内存块为空闲状态.
// 操作流程与 runtime·markallocated 基本相同.
//
// mark the block at v of size n as freed.
void runtime·markfreed(void *v, uintptr n)
{
	uintptr *b, obits, bits, off, shift;

	if(0) {
		runtime·printf("markfreed %p+%p\n", v, n);
	}
	// 超出范围
	if((byte*)v+n > (byte*)runtime·mheap.arena_used || 
		(byte*)v < runtime·mheap.arena_start) {
		runtime·throw("markfreed: bad pointer");
	}

	off = (uintptr*)v - (uintptr*)runtime·mheap.arena_start;  // word offset
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	for(;;) {
		obits = *b;
		bits = (obits & ~(bitMask<<shift)) | (bitBlockBoundary<<shift);
		if(runtime·gomaxprocs == 1) {
			*b = bits;
			break;
		} else {
			// more than one goroutine is potentially running: use atomic op
			if(runtime·casp((void**)b, (void*)obits, (void*)bits)) {
				break;
			}
		}
	}
}

// check that the block at v of size n is marked freed.
void runtime·checkfreed(void *v, uintptr n)
{
	uintptr *b, bits, off, shift;

	if(!runtime·checking) {
		return;
	}

	if((byte*)v+n > (byte*)runtime·mheap.arena_used || (byte*)v < runtime·mheap.arena_start) {
		return;	// not allocated, so okay
	}
	// 下面几行计算语句, 根据 obj 对象地址, 反推其在 bitmap 中描述字的地址, 详细解释可见:
	// <!link!>: {32a3d702-70db-4cae-852b-5c12ce491afc}
	//
	off = (uintptr*)v - (uintptr*)runtime·mheap.arena_start;  // word offset
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	bits = *b>>shift;
	if((bits & bitAllocated) != 0) {
		runtime·printf("checkfreed %p+%p: off=%p have=%p\n", v, n, off, bits & bitMask);
		runtime·throw("checkfreed: not freed");
	}
}

// 标记在内存 span 的地址 v 处, 分配了 n 个大小为 size 的块.
//
// 	@param v: arena 区域中某个已分配空间的指针地址
// 	@param size: 当前 span 可容纳的对象大小(不是 sizeclass)
// 	@param n: 当前 span 可容纳的对象个数
//
// caller: 
// 	1. src/pkg/runtime/malloc.goc -> runtime·mallocgc() 
// 	从堆上分配超过32k的大对象成功后, 调用本函数, 标记目标区域为已分配.
// 	此时传入的 size, n 参数都为 0
// 	2. src/pkg/runtime/mcentral.c -> MCentral_Grow()
//
// mark the span of memory at v as having n blocks of the given size.
// if leftover is true, there is left over space at the end of the span.
void runtime·markspan(void *v, uintptr size, uintptr n, bool leftover)
{
	uintptr *b, off, shift;
	byte *p;

	// 超出范围
	if((byte*)v+size*n > (byte*)runtime·mheap.arena_used || 
		(byte*)v < runtime·mheap.arena_start) {
		runtime·throw("markspan: bad pointer");
	}

	p = v;
	// mark a boundary just past end of last block too
	if(leftover) {
		n++;
	}

	for(; n-- > 0; p += size) {
		// Okay to use non-atomic ops here, because we control the entire span, 
		// and each bitmap word has bits for only one span, 
		// so no other goroutines are changing these bitmap words.
		off = (uintptr*)p - (uintptr*)runtime·mheap.arena_start;  // word offset
		b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
		shift = off % wordsPerBitmapWord;
		*b = (*b & ~(bitMask<<shift)) | (bitBlockBoundary<<shift);
	}
}

// 从地址v开始的n bytes的内存块在bitmap中的标识位清空为0.
//
// caller:
// 	1. sweepspan()
//
// unmark the span of memory at v of length n bytes.
void runtime·unmarkspan(void *v, uintptr n)
{
	uintptr *p, *b, off;

	if((byte*)v+n > (byte*)runtime·mheap.arena_used || 
		(byte*)v < runtime·mheap.arena_start) {
		runtime·throw("markspan: bad pointer");
	}

	p = v;

	// word offset
	// 还是寻找在bitmap中的映射
	off = p - (uintptr*)runtime·mheap.arena_start;
	if(off % wordsPerBitmapWord != 0) {
		runtime·throw("markspan: unaligned pointer");
	} 
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	// n原来是byte数量, 现在变成了指针数量.
	n /= PtrSize;

	// 地址v对应的bitmap地址应该按字对齐.
	if(n%wordsPerBitmapWord != 0) {
		runtime·throw("unmarkspan: unaligned length");
	}
	// Okay to use non-atomic ops here, 
	// because we control the entire span, 
	// and each bitmap word has bits for only one span, 
	// so no other goroutines are changing these bitmap words.
	// 将bitmap中每个字都设置为0
	n /= wordsPerBitmapWord;
	while(n-- > 0) {
		*b-- = 0;
	} 
}

bool runtime·blockspecial(void *v)
{
	uintptr *b, off, shift;

	if(DebugMark) {
		return true;
	} 

	off = (uintptr*)v - (uintptr*)runtime·mheap.arena_start;
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	return (*b & (bitSpecial<<shift)) != 0;
}

// caller:
// 	1. src/pkg/runtime/mgc0__sweep.c -> handlespecial()
void runtime·setblockspecial(void *v, bool s)
{
	uintptr *b, off, shift, bits, obits;

	if(DebugMark) {
		return;
	} 

	off = (uintptr*)v - (uintptr*)runtime·mheap.arena_start;
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	for(;;) {
		obits = *b;
		if(s) {
			bits = obits | (bitSpecial<<shift);
		} 
		else {
			bits = obits & ~(bitSpecial<<shift);
		} 
		if(runtime·gomaxprocs == 1) {
			*b = bits;
			break;
		} else {
			// more than one goroutine is potentially running: use atomic op
			if(runtime·casp((void**)b, (void*)obits, (void*)bits)) {
				break;
			} 
		}
	}
}
