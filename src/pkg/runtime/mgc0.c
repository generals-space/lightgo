// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Garbage collector.

#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "stack.h"
#include "mgc0.h"
#include "mgc0__funcs.h"
#include "race.h"
#include "type.h"
#include "typekind.h"
#include "funcdata.h"
#include "../../cmd/ld/textflag.h"

// 关于 bitmap 中, 每个 word(字)的每个"bit位"的描述.
//
// <!link!>: {32a3d702-70db-4cae-852b-5c12ce491afc}
//
// Bits in per-word bitmap.
// #defines because enum might not be able to hold the values.
//
// Each word in the bitmap describes wordsPerBitmapWord words of heap memory. 
//
// There are 4 bitmap bits dedicated to each heap word,
// so on a 64-bit system there is one bitmap word per 16 heap words.
//
// The bits in the word are packed together by type first, then by heap location, 
// so each 64-bit bitmap word consists of, from top to bottom,
// the 16 bitSpecial bits for the corresponding heap words,
// then the 16 bitMarked bits,
// then the 16 bitNoScan/bitBlockBoundary bits,
// then the 16 bitAllocated bits.
// This layout makes it easier to iterate over the bits of a given type.
//
// The bitmap starts at mheap.arena_start and extends *backward* from there. 
// On a 64-bit system the off'th word in the arena is tracked by
// the off/16+1'th word before mheap.arena_start. 
// (On a 32-bit system, the only difference is that the divisor is 8.)
//
// To pull out the bits corresponding to a given pointer p, we use:
// 
//	off = p - (uintptr*)mheap.arena_start;  // word offset
//	b = (uintptr*)mheap.arena_start - off/wordsPerBitmapWord - 1;
//	shift = off % wordsPerBitmapWord
//	bits = *b >> shift; // 注意这里对b用*做取值操作.
//	/* then test bits & bitAllocated, bits & bitMarked, etc. */
//
// FFFF FFFF FFFF FFFF
// 使用 printf() 打印时, 格式需要使用"%D", 才能打印64位长整型(与常规c语言的"%ld"不太一样)
//
// 0000 0000 0000 0001(1)
#define bitAllocated		((uintptr)1<<(bitShift*0))
// 0000 0000 0001 0000(65536) (与 bitBlockBoundary 取值相同)
// when bitAllocated is set
#define bitNoScan		((uintptr)1<<(bitShift*1))
// 貌似是标记过的不会被回收, 没被标记的才会?
//
// 0000 0001 0000 0000(4294967296)
// when bitAllocated is set
#define bitMarked		((uintptr)1<<(bitShift*2))
// 0001 0000 0000 0000(281474976710656)
// when bitAllocated is set - has finalizer or being profiled
#define bitSpecial		((uintptr)1<<(bitShift*3))
// 0000 0000 0001 0000(65536) (与 bitNoScan 取值相同)
// when bitAllocated is NOT set
#define bitBlockBoundary	((uintptr)1<<(bitShift*1))
// bitMask 是3种已知的"bit位"组合, 用来先对目标块的"bit位"进行清零的. 
// 最典型的例子就是: *bitp & ~(bitMask<<shift)
//
// 0001 0001 0000 0001
#define bitMask (bitBlockBoundary | bitAllocated | bitMarked | bitSpecial)

// Holding worldsema grants an M the right to try to stop the world.
// 哪个M对象获取到了worldsema, 谁都拥有STW的权力
// The procedure is:
//
//	runtime·semacquire(&runtime·worldsema);
//	m->gcing = 1;
//	runtime·stoptheworld();
//
//	... do stuff ...
//
//	m->gcing = 0;
//	runtime·semrelease(&runtime·worldsema);
//	runtime·starttheworld();
//
uint32 runtime·worldsema = 1;

// 常用的对象构造语法: (Obj){p, n, ti}
// p: 该对象实际存储数据的位置
// n: 该对象存储的数据的大小(bytes)
// ti: 该对象的类型信息
typedef struct Obj Obj;
struct Obj
{
	// p: 该对象实际存储数据的位置
	//
	// data pointer
	byte	*p;
	// n: 该对象存储的数据的大小(bytes)
	//
	// size of data in bytes
	uintptr	n;
	// ti: 该对象的类型信息
	//
	// type info
	uintptr	ti;
};

// The size of Workbuf is N*PageSize.
typedef struct Workbuf Workbuf;
struct Workbuf
{
#define SIZE (2*PageSize-sizeof(LFNode)-sizeof(uintptr))
	LFNode  node; // must be first
	// obj 数组中的实际长度
	uintptr nobj;
	// obj 数组
	Obj     obj[SIZE/sizeof(Obj) - 1];
	uint8   _padding[SIZE%sizeof(Obj) + sizeof(Obj)];
#undef SIZE
};

typedef struct Finalizer Finalizer;
struct Finalizer
{
	FuncVal *fn;
	void *arg;
	uintptr nret;
	Type *fint;
	PtrType *ot;
};

typedef struct FinBlock FinBlock;
struct FinBlock
{
	FinBlock *alllink;
	FinBlock *next;
	int32 cnt;
	int32 cap;
	Finalizer fin[1];
};

extern byte data[];
extern byte edata[];
extern byte bss[];
extern byte ebss[];

extern byte gcdata[];
extern byte gcbss[];

// 用来执行 finalizer 的协程 ~~链表~~, 不对, 好像只有一个
static G *fing; 
// 等待执行的finalizer链表
//
// list of finalizers that are to be executed
static FinBlock *finq; 
static FinBlock *finc; // cache of free blocks
static FinBlock *allfin; // list of all blocks
static Lock finlock;
static int32 fingwait;

static void runfinq(void);
static Workbuf* getempty(Workbuf*);
static Workbuf* getfull(Workbuf*);
static void	putempty(Workbuf*);
static Workbuf* handoff(Workbuf*);
static void	gchelperstart(void);

static struct {
	uint64	full;  // lock-free list of full blocks
	uint64	empty; // lock-free list of empty blocks

	// prevents false-sharing between full/empty and nproc/nwait
	byte	pad0[CacheLineSize];

	// 参与执行gc的协程数量, 其值由 runtime·gcprocs() 设置.
	// 取值为 min(GOMAXPROCS, ncpu, MaxGcproc), 其中
	// 1. GOMAXPROCS 由开发者通过环境变量设置;
	// 2. ncpu 为物理机实际 cpu 核数(在 runtime 启动时自动探测);
	// 3. MaxGcproc 
	// 参与执行gc的协程要运行的是 runtime·gchelper() 函数.
	uint32	nproc;
	volatile uint32	nwait;
	// 参考 alldone 成员的解释, 每个参与gc的协程在完成自己部分的标记清理工作后,
	// 都会对 ndone 进行加1的原子操作, 然后与 nproc-1 比较(减去的1是gc主协程),
	// 如果相等, 那么表示所以gc协程都已完成, 就唤醒在 alldone 处等待的主协程.
	volatile uint32	ndone;
	volatile uint32 debugmarkdone;
	// Note与Lock一样(...完全一样), 是分别基于futex/sema实现的锁吧
	// 在STW后, 实际执行gc操作的协程可能不只一个,
	// 数量大致在 [1-nproc) 之间, ta们并行执行gc操作.
	// gc主线程将在这个地址休眠, 直到所有gc协程全部完成标记清理工作后, 再继续执行.
	Note	alldone; 
	ParFor	*markfor;
	ParFor	*sweepfor;

	Lock;
	byte	*chunk;
	uintptr	nchunk;

	// 此数组下存储着 nroot 个 Obj 对象, 每个 obj 对象都是 arena 区域中的一个地址.
	// ta们是 gc 的"标记清除"流程中, 真正的目标.
	Obj	*roots; 
	// roots 数组中的成员数量
	uint32	nroot;
	uint32	rootcap; // nroot <= rootcap, 为roots可容纳的最大root数量.
} work;

enum {
	GC_DEFAULT_PTR = GC_NUM_INSTR,
	GC_CHAN,

	GC_NUM_INSTR2
};

static struct {
	struct {
		uint64 sum;
		uint64 cnt;
	} ptr;
	uint64 nbytes;
	struct {
		uint64 sum;
		uint64 cnt;
		uint64 notype;
		uint64 typelookup;
	} obj;
	uint64 rescan;
	uint64 rescanbytes;
	uint64 instr[GC_NUM_INSTR2];
	uint64 putempty;
	uint64 getfull;
	struct {
		uint64 foundbit;
		uint64 foundword;
		uint64 foundspan;
	} flushptrbuf;
	struct {
		uint64 foundbit;
		uint64 foundword;
		uint64 foundspan;
	} markonly;
} gcstats;

// 标记对象. 主要就是找到该对象在 bitmap 区域的映射地址, 添加 bitMarked 标记.
// (需要目标块已存在 bitAllocated 标记)
//
// 注意: 被标记过的块才是安全的, 反之, 则需要被回收.
//
// 	@param obj: arena 区域中的某个指针(ta在 bitmap 区域一定有对应的描述字(word)信息)
//
// caller:
// 	1. addroots()
//
// markonly marks an object. 
// It returns true if the object has been marked by this function, false otherwise.
// This function doesn't append the object to any buffer.
static bool markonly(void *obj)
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

// PtrTarget is a structure used by intermediate buffers.
// The intermediate buffers hold GC data before it is moved/flushed to
// the work buffer (Workbuf).
// The size of an intermediate buffer is very small, such as 32 or 64 elements.
typedef struct PtrTarget PtrTarget;
struct PtrTarget
{
	void *p;
	uintptr ti;
};

typedef struct BufferList BufferList;
struct BufferList
{
	PtrTarget 	ptrtarget[IntermediateBufferCapacity];
	Obj 		obj[IntermediateBufferCapacity];
	uint32 		busy;
	byte 		pad[CacheLineSize];
};
#pragma dataflag NOPTR

// 为执行gc操作的协程提供各自的缓存数组.
// 由于执行gc操作的协程不只一个, gc()->runtime·gcprocs() 可以得到执行gc的协程数量.
// 这里 bufferList 数组的容量 MaxGcproc 是绝对够用的.
static BufferList bufferList[MaxGcproc];

static Type *itabtype;

static void enqueue(Obj obj, Workbuf **_wbuf, Obj **_wp, uintptr *_nobj);

// caller:
// 	1. scanblock() 只有这一个函数, 但是有多次调用.
//
// flushptrbuf moves data from the PtrTarget buffer to the work buffer.
// The PtrTarget buffer contains blocks irrespective of whether the blocks
// have been marked or scanned,
// while the work buffer contains blocks which have been marked
// and are prepared to be scanned by the garbage collector.
//
// _wp, _wbuf, _nobj are input/output parameters and are specifying the work buffer.
//
// A simplified drawing explaining how the todo-list moves from a structure to another:
//
//     scanblock
//  (find pointers)
//    Obj ------> PtrTarget (pointer targets)
//     ↑          |
//     |          |
//     `----------'
//     flushptrbuf
//  (find block start, mark and enqueue)
static void flushptrbuf(
	PtrTarget *ptrbuf, PtrTarget **ptrbufpos, 
	Obj **_wp, Workbuf **_wbuf, uintptr *_nobj
)
{
	byte *p, *arena_start, *obj;
	uintptr size, *bitp, bits, shift, j, x, xbits, off, nobj, ti, n;
	MSpan *s;
	PageID k;
	Obj *wp;
	Workbuf *wbuf;
	PtrTarget *ptrbuf_end;

	arena_start = runtime·mheap.arena_start;

	wp = *_wp;
	wbuf = *_wbuf;
	nobj = *_nobj;

	ptrbuf_end = *ptrbufpos;
	n = ptrbuf_end - ptrbuf;
	*ptrbufpos = ptrbuf;

	if(CollectStats) {
		runtime·xadd64(&gcstats.ptr.sum, n);
		runtime·xadd64(&gcstats.ptr.cnt, 1);
	}

	// If buffer is nearly full, get a new one.
	if(wbuf == nil || nobj+n >= nelem(wbuf->obj)) {
		if(wbuf != nil) {
			wbuf->nobj = nobj;
		} 

		wbuf = getempty(wbuf);
		wp = wbuf->obj;
		nobj = 0;

		if(n >= nelem(wbuf->obj)) {
			runtime·throw("ptrbuf has to be smaller than WorkBuf");
		}
	}

	// TODO(atom): This block is a branch of an if-then-else statement.
	//             The single-threaded branch may be added in a next CL.
	{
		// Multi-threaded version.

		while(ptrbuf < ptrbuf_end) {
			obj = ptrbuf->p;
			ti = ptrbuf->ti;
			ptrbuf++;

			// obj belongs to interval [mheap.arena_start, mheap.arena_used).
			if(Debug > 1) {
				if(obj < runtime·mheap.arena_start || obj >= runtime·mheap.arena_used) {
					runtime·throw("object is outside of mheap");
				}
			}

			// obj may be a pointer to a live object.
			// Try to find the beginning of the object.

			// Round down to word boundary.
			if(((uintptr)obj & ((uintptr)PtrSize-1)) != 0) {
				obj = (void*)((uintptr)obj & ~((uintptr)PtrSize-1));
				ti = 0;
			}

			// Find bits for this word.
			off = (uintptr*)obj - (uintptr*)arena_start;
			bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
			shift = off % wordsPerBitmapWord;
			xbits = *bitp;
			bits = xbits >> shift;

			// Pointing at the beginning of a block?
			if((bits & (bitAllocated|bitBlockBoundary)) != 0) {
				if(CollectStats) {
					runtime·xadd64(&gcstats.flushptrbuf.foundbit, 1);
				}
				goto found;
			}

			ti = 0;

			// Pointing just past the beginning?
			// Scan backward a little to find a block boundary.
			for(j=shift; j-->0; ) {
				if(((xbits>>j) & (bitAllocated|bitBlockBoundary)) != 0) {
					obj = (byte*)obj - (shift-j)*PtrSize;
					shift = j;
					bits = xbits>>shift;
					if(CollectStats) {
						runtime·xadd64(&gcstats.flushptrbuf.foundword, 1);
					}
					goto found;
				}
			}

			// Otherwise consult span table to find beginning.
			// (Manually inlined copy of MHeap_LookupMaybe.)
			k = (uintptr)obj>>PageShift;
			x = k;
			if(sizeof(void*) == 8) {
				x -= (uintptr)arena_start>>PageShift;
			}
			s = runtime·mheap.spans[x];
			if(s == nil || k < s->start || obj >= s->limit || s->state != MSpanInUse) {
				continue;
			}
			p = (byte*)((uintptr)s->start<<PageShift);
			if(s->sizeclass == 0) {
				obj = p;
			} else {
				size = s->elemsize;
				int32 i = ((byte*)obj - p)/size;
				obj = p+i*size;
			}

			// Now that we know the object header, reload bits.
			off = (uintptr*)obj - (uintptr*)arena_start;
			bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
			shift = off % wordsPerBitmapWord;
			xbits = *bitp;
			bits = xbits >> shift;
			if(CollectStats) {
				runtime·xadd64(&gcstats.flushptrbuf.foundspan, 1);
			}

		found:
			// Now we have bits, bitp, and shift correct for
			// obj pointing at the base of the object.
			// Only care about allocated and not marked.
			if((bits & (bitAllocated|bitMarked)) != bitAllocated) {
				continue;
			}
			if(work.nproc == 1) {
				*bitp |= bitMarked<<shift;
			}
			else {
				for(;;) {
					x = *bitp;
					if(x & (bitMarked<<shift)) {
						goto continue_obj;
					}
					if(runtime·casp((void**)bitp, (void*)x, (void*)(x|(bitMarked<<shift)))) {
						break;
					}
				}
			}

			// If object has no pointers, don't need to scan further.
			if((bits & bitNoScan) != 0) {
				continue;
			}

			// Ask span about size class.
			// (Manually inlined copy of MHeap_Lookup.)
			x = (uintptr)obj >> PageShift;
			if(sizeof(void*) == 8) {
				x -= (uintptr)arena_start>>PageShift;
			}
			s = runtime·mheap.spans[x];

			PREFETCH(obj);

			*wp = (Obj){obj, s->elemsize, ti};
			wp++;
			nobj++;
		continue_obj:;
		} // while(ptrbuf < ptrbuf_end){} 结束

		// If another proc wants a pointer, give it some.
		if(work.nwait > 0 && nobj > handoffThreshold && work.full == 0) {
			wbuf->nobj = nobj;
			wbuf = handoff(wbuf);
			nobj = wbuf->nobj;
			wp = wbuf->obj + nobj;
		}
	}

	*_wp = wp;
	*_wbuf = wbuf;
	*_nobj = nobj;
}

// caller:
// 	1. scanblock() 只有这一个函数, 但是有多次调用.
//
static void flushobjbuf(
	Obj *objbuf, Obj **objbufpos, 
	Obj **_wp, Workbuf **_wbuf, uintptr *_nobj
)
{
	uintptr nobj, off;
	Obj *wp, obj;
	Workbuf *wbuf;
	Obj *objbuf_end;

	wp = *_wp;
	wbuf = *_wbuf;
	nobj = *_nobj;

	objbuf_end = *objbufpos;
	*objbufpos = objbuf;

	while(objbuf < objbuf_end) {
		obj = *objbuf++;

		// Align obj.b to a word boundary.
		off = (uintptr)obj.p & (PtrSize-1);
		if(off != 0) {
			obj.p += PtrSize - off;
			obj.n -= PtrSize - off;
			obj.ti = 0;
		}

		if(obj.p == nil || obj.n == 0) {
			continue;
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
	}

	// If another proc wants a pointer, give it some.
	if(work.nwait > 0 && nobj > handoffThreshold && work.full == 0) {
		wbuf->nobj = nobj;
		wbuf = handoff(wbuf);
		nobj = wbuf->nobj;
		wp = wbuf->obj + nobj;
	}

	*_wp = wp;
	*_wbuf = wbuf;
	*_nobj = nobj;
}

// Program that scans the whole block and treats every block element
// as a potential pointer
static uintptr defaultProg[2] = {PtrSize, GC_DEFAULT_PTR};

// Hchan program
static uintptr chanProg[2] = {0, GC_CHAN};

// Local variables of a program fragment or loop
typedef struct Frame Frame;
struct Frame {
	uintptr count, elemsize, b;
	uintptr *loop_or_ret;
};

// caller:
// 	1. scanblock() 只有这一处, 且只在 Debug 情况下会调用.
//
// Sanity check(完整性检查, 合理性检查) for the derived type info objti.
static void checkptr(void *obj, uintptr objti)
{
	uintptr *pc1, *pc2, type, tisize, i, j, x;
	byte *objstart;
	Type *t;
	MSpan *s;

	if(!Debug) {
		runtime·throw("checkptr is debug only");
	}

	if(obj < runtime·mheap.arena_start || obj >= runtime·mheap.arena_used) {
		return;
	}
	type = runtime·gettype(obj);
	t = (Type*)(type & ~(uintptr)(PtrSize-1));
	if(t == nil) {
		return;
	}
	x = (uintptr)obj >> PageShift;
	if(sizeof(void*) == 8) {
		x -= (uintptr)(runtime·mheap.arena_start)>>PageShift;
	}
	s = runtime·mheap.spans[x];
	objstart = (byte*)((uintptr)s->start<<PageShift);
	if(s->sizeclass != 0) {
		i = ((byte*)obj - objstart)/s->elemsize;
		objstart += i*s->elemsize;
	}
	tisize = *(uintptr*)objti;
	// Sanity check(完整性检查, 合理性检查) for object size:
	// it should fit into the memory block.
	if((byte*)obj + tisize > objstart + s->elemsize) {
		runtime·printf(
			"object of type '%S' at %p/%p does not fit in block %p/%p\n",
			*t->string, obj, tisize, objstart, s->elemsize
		);
		runtime·throw("invalid gc type info");
	}
	if(obj != objstart) {
		return;
	}
	// If obj points to the beginning of the memory block, check type info as well.
	if(t->string == nil ||
		// Gob allocates unsafe pointers for indirection.
		(runtime·strcmp(t->string->str, (byte*)"unsafe.Pointer") &&
		// Runtime and gc think differently about closures.
		runtime·strstr(t->string->str, (byte*)"struct { F uintptr") != t->string->str)) {
		pc1 = (uintptr*)objti;
		pc2 = (uintptr*)t->gc;
		// A simple best-effort check until first GC_END.
		for(j = 1; pc1[j] != GC_END && pc2[j] != GC_END; j++) {
			if(pc1[j] != pc2[j]) {
				runtime·printf(
					"invalid gc type info for '%s' at %p, type info %p, block info %p\n",
					t->string ? (int8*)t->string->str : (int8*)"?", j, pc1[j], pc2[j]
				);
				runtime·throw("invalid gc type info");
			}
		}
	}
}

// scanblock 扫描一个起始地址为 b, 大小为 n bytes 的块, 找到这个块引用的所有其他的对象.
// 没有使用递归, 而是使用了一个 Workbuf* 结构体指针的工作列表, 然后遍历ta.
// 维护一个列表比递归更简单(在栈分配上), 而且更高效.
// ...应该就是广度优先与深度优先的区别吧.
// ...超长的一个函数
//
// caller: 
// 	1. markroot()
// 	2. runtime·gchelper()
// 	3. gc()
//
// wbuf: current work buffer
// wp:   storage for next queued pointer (write pointer)
// nobj: number of queued objects
//
// scanblock scans a block of n bytes starting at pointer b 
// for references to other objects, 
// scanning any it finds recursively until there are no unscanned objects left. 
// Instead of using an explicit recursion, it keeps a work list
// in the Workbuf* structures and loops in the main function body. 
// Keeping an explicit work list is easier 
// on the stack allocator and more efficient.
// 
static void scanblock(Workbuf *wbuf, Obj *wp, uintptr nobj, bool keepworking)
{
	byte *b, *arena_start, *arena_used;
	uintptr n, i, end_b, elemsize, size, ti, objti, count, type;
	uintptr *pc, precise_type, nominal_size;
	uintptr *chan_ret, chancap;
	void *obj;
	Type *t;
	Slice *sliceptr;
	Frame *stack_ptr, stack_top, stack[GC_STACK_CAPACITY+4];
	BufferList *scanbuffers;
	PtrTarget *ptrbuf, *ptrbuf_end, *ptrbufpos;
	Obj *objbuf, *objbuf_end, *objbufpos;
	Eface *eface;
	Iface *iface;
	Hchan *chan;
	ChanType *chantype;

	if(sizeof(Workbuf) % PageSize != 0) {
		runtime·throw("scanblock: size of Workbuf is suboptimal");
	}

	// Memory arena parameters.
	arena_start = runtime·mheap.arena_start;
	arena_used = runtime·mheap.arena_used;

	stack_ptr = stack+nelem(stack)-1;
	
	precise_type = false;
	nominal_size = 0;

	// Allocate ptrbuf
	{
		scanbuffers = &bufferList[m->helpgc];
		ptrbuf = &scanbuffers->ptrtarget[0];
		ptrbuf_end = &scanbuffers->ptrtarget[0] + nelem(scanbuffers->ptrtarget);
		objbuf = &scanbuffers->obj[0];
		objbuf_end = &scanbuffers->obj[0] + nelem(scanbuffers->obj);
	}

	ptrbufpos = ptrbuf;
	objbufpos = objbuf;

	// (Silence the compiler)
	chan = nil;
	chantype = nil;
	chan_ret = nil;

	// 第1次运行到这里, 会直接跳转到 next_block 块, 执行完成后会跳回来继续执行下面的 for{} 循环
	goto next_block;

	for(;;) {
		// Each iteration scans the block b of length n, queueing pointers in
		// the work buffer.
		if(Debug > 1) {
			runtime·printf("scanblock %p %D\n", b, (int64)n);
		}

		if(CollectStats) {
			runtime·xadd64(&gcstats.nbytes, n);
			runtime·xadd64(&gcstats.obj.sum, nobj);
			runtime·xadd64(&gcstats.obj.cnt, 1);
		}

		if(ti != 0) {
			// 在 addroots() 的所有 addroot() 调用添加的根节点中, 只有3种根节点的 ti 类型不为 0:
			// data, bss, 以及基本上所有的栈空间(在 addstackroots() 中添加)
			//
			// 其中, data, bss 的 ti 类型还没弄明白, 不过各 g 协程的栈空间根节点, ti 参数都是
			// (uintptr)defaultProg | PRECISE | LOOP,
			// 而 PC_BITS == PRECISE | LOOP, 所以下面2行就是为了分离 defaultProg* 指针与其末尾的信息.
			// 注意, 下面的 pc 在分离完 PC_BITS 后, 就得到了 defaultProg 的真正地址,
			// 那么 pc[0] 就会得到 PtrSize, pc++ 后的 pc[0] 则会得到 GC_DEFAULT_PTR,
			// 在下面的 switch..case..中, 就会匹配到对应的语句.
			pc = (uintptr*)(ti & ~(uintptr)PC_BITS);
			precise_type = (ti & PRECISE);
			// runtime·printf(
			// 	"pc[0]: %d, pc[1]: %d, pc[2]: %d, precise: %d, inprecise: %d\n", 
			// 	pc[0], pc[1], pc[2], precise_type, !precise_type
			// );

			stack_top.elemsize = pc[0];
			if(!precise_type) {
				nominal_size = pc[0];
			}
			if(ti & LOOP) {
				// 0 表示无限次循环
				//
				// 0 means an infinite number of iterations
				stack_top.count = 0;
				stack_top.loop_or_ret = pc+1;
			} else {
				stack_top.count = 1;
			}
			if(Debug) {
				// Simple sanity check for provided type info ti:
				// The declared size of the object must be not larger than
				// the actual size (it can be smaller due to inferior pointers).
				// It's difficult to make a comprehensive check due to
				// inferior pointers, reflection, gob, etc.
				if(pc[0] > n) {
					runtime·printf(
						"invalid gc type info: type info size %p, block size %p\n", 
						pc[0], n
					);
					runtime·throw("invalid gc type info");
				}
			}
		} else if(UseSpanType) {
			if(CollectStats) {
				runtime·xadd64(&gcstats.obj.notype, 1);
			}

			type = runtime·gettype(b);
			if(type != 0) {
				if(CollectStats) {
					runtime·xadd64(&gcstats.obj.typelookup, 1);
				}

				t = (Type*)(type & ~(uintptr)(PtrSize-1));
				switch(type & (PtrSize-1)) {
				case TypeInfo_SingleObject:
					pc = (uintptr*)t->gc;
					precise_type = true;  // type information about 'b' is precise
					stack_top.count = 1;
					stack_top.elemsize = pc[0];
					break;
				case TypeInfo_Array:
					pc = (uintptr*)t->gc;
					if(pc[0] == 0) {
						goto next_block;
					}
					precise_type = true;  // type information about 'b' is precise
					// 0 表示无限次循环
					//
					// 0 means an infinite number of iterations
					stack_top.count = 0;
					stack_top.elemsize = pc[0];
					stack_top.loop_or_ret = pc+1;
					break;
				case TypeInfo_Chan:
					chan = (Hchan*)b;
					chantype = (ChanType*)t;
					chan_ret = nil;
					pc = chanProg;
					break;
				default:
					runtime·throw("scanblock: invalid type");
					return;
				}
			} else {
				pc = defaultProg;
			}
		} else {
			pc = defaultProg;
		}

		if(IgnorePreciseGC) {
			pc = defaultProg;
		}

		pc++;
		stack_top.b = (uintptr)b;

		end_b = (uintptr)b + n - PtrSize;

	// 状态机的思想
	for(;;) {
		if(CollectStats) {
			runtime·xadd64(&gcstats.instr[pc[0]], 1);
		}

		obj = nil;
		objti = 0;
		// pc 是gc行为的操作命令, 其中 pc[0] 为操作码(就是下面的 case 部分),
		// 每种操作码接收的参数数量是不同的, 具体的操作码及参数对应关系, 可见
		// src/pkg/reflect/type.go -> appendGCProgram()
		// 
		// 比如 GC_PTR, 在 appendGCProgram() 中, `_GC_PTR`指令的 argcnt = 2(两个参数),
		// 那么下面的 case 中, pc[0] 为 GC_PTR 自身, pc[1]和pc[2]就是这两个参数,
		// 之后指针后移3位, 进行下一个指令的解析.
		switch(pc[0]) {
		case GC_PTR:
			obj = *(void**)(stack_top.b + pc[1]);
			objti = pc[2];
			pc += 3;
			if(Debug) {
				checkptr(obj, objti);
			}
			break;

		case GC_SLICE:
			sliceptr = (Slice*)(stack_top.b + pc[1]);
			if(sliceptr->cap != 0) {
				obj = sliceptr->array;
				// Can't use slice element type for scanning,
				// because if it points to an array embedded
				// in the beginning of a struct,
				// we will scan the whole struct as the slice.
				// So just obtain type info from heap.
			}
			pc += 3;
			break;

		case GC_APTR:
			obj = *(void**)(stack_top.b + pc[1]);
			pc += 2;
			break;

		case GC_STRING:
			obj = *(void**)(stack_top.b + pc[1]);
			markonly(obj);
			pc += 2;
			continue;

		case GC_EFACE:
			eface = (Eface*)(stack_top.b + pc[1]);
			pc += 2;
			if(eface->type == nil) {
				continue;
			}

			// eface->type
			t = eface->type;
			if((void*)t >= arena_start && (void*)t < arena_used) {
				*ptrbufpos++ = (PtrTarget){t, 0};
				if(ptrbufpos == ptrbuf_end) {
					flushptrbuf(ptrbuf, &ptrbufpos, &wp, &wbuf, &nobj);
				}
			}

			// eface->data
			if(eface->data >= arena_start && eface->data < arena_used) {
				if(t->size <= sizeof(void*)) {
					if((t->kind & KindNoPointers)) {
						continue;
					}

					obj = eface->data;
					if((t->kind & ~KindNoPointers) == KindPtr) {
						objti = (uintptr)((PtrType*)t)->elem->gc;
					}
				} else {
					obj = eface->data;
					objti = (uintptr)t->gc;
				}
			}
			break;

		case GC_IFACE:
			iface = (Iface*)(stack_top.b + pc[1]);
			pc += 2;
			if(iface->tab == nil) {
				continue;
			}
			
			// iface->tab
			if((void*)iface->tab >= arena_start && (void*)iface->tab < arena_used) {
				*ptrbufpos++ = (PtrTarget){iface->tab, (uintptr)itabtype->gc};
				if(ptrbufpos == ptrbuf_end) {
					flushptrbuf(ptrbuf, &ptrbufpos, &wp, &wbuf, &nobj);
				}
			}

			// iface->data
			if(iface->data >= arena_start && iface->data < arena_used) {
				t = iface->tab->type;
				if(t->size <= sizeof(void*)) {
					if((t->kind & KindNoPointers)) {
						continue;
					}

					obj = iface->data;
					if((t->kind & ~KindNoPointers) == KindPtr) {
						objti = (uintptr)((PtrType*)t)->elem->gc;
					}
				} else {
					obj = iface->data;
					objti = (uintptr)t->gc;
				}
			}
			break;

		case GC_DEFAULT_PTR:
			while(stack_top.b <= end_b) {
				obj = *(byte**)stack_top.b;
				stack_top.b += PtrSize;
				if(obj >= arena_start && obj < arena_used) {
					*ptrbufpos++ = (PtrTarget){obj, 0};
					if(ptrbufpos == ptrbuf_end) {
						flushptrbuf(ptrbuf, &ptrbufpos, &wp, &wbuf, &nobj);
					}
				}
			}
			goto next_block;

		case GC_END:
			if(--stack_top.count != 0) {
				// Next iteration of a loop if possible.
				stack_top.b += stack_top.elemsize;
				if(stack_top.b + stack_top.elemsize <= end_b+PtrSize) {
					pc = stack_top.loop_or_ret;
					continue;
				}
				i = stack_top.b;
			} else {
				// Stack pop if possible.
				if(stack_ptr+1 < stack+nelem(stack)) {
					pc = stack_top.loop_or_ret;
					stack_top = *(++stack_ptr);
					continue;
				}
				i = (uintptr)b + nominal_size;
			}
			if(!precise_type) {
				// Quickly scan [b+i,b+n) for possible pointers.
				for(; i<=end_b; i+=PtrSize) {
					if(*(byte**)i != nil) {
						// Found a value that may be a pointer.
						// Do a rescan of the entire block.
						enqueue((Obj){b, n, 0}, &wbuf, &wp, &nobj);
						if(CollectStats) {
							runtime·xadd64(&gcstats.rescan, 1);
							runtime·xadd64(&gcstats.rescanbytes, n);
						}
						break;
					}
				}
			}
			goto next_block;

		case GC_ARRAY_START:
			i = stack_top.b + pc[1];
			count = pc[2];
			elemsize = pc[3];
			pc += 4;

			// Stack push.
			*stack_ptr-- = stack_top;
			stack_top = (Frame){count, elemsize, i, pc};
			continue;

		case GC_ARRAY_NEXT:
			if(--stack_top.count != 0) {
				stack_top.b += stack_top.elemsize;
				pc = stack_top.loop_or_ret;
			} else {
				// Stack pop.
				stack_top = *(++stack_ptr);
				pc += 1;
			}
			continue;

		case GC_CALL:
			// Stack push.
			*stack_ptr-- = stack_top;
			stack_top = (Frame){1, 0, stack_top.b + pc[1], pc+3 /*return address*/};
			pc = (uintptr*)((byte*)pc + *(int32*)(pc+2));  // target of the CALL instruction
			continue;

		case GC_REGION:
			obj = (void*)(stack_top.b + pc[1]);
			size = pc[2];
			objti = pc[3];
			pc += 4;

			*objbufpos++ = (Obj){obj, size, objti};
			if(objbufpos == objbuf_end) {
				flushobjbuf(objbuf, &objbufpos, &wp, &wbuf, &nobj);
			}
			continue;

		case GC_CHAN_PTR:
			chan = *(Hchan**)(stack_top.b + pc[1]);
			if(chan == nil) {
				pc += 3;
				continue;
			}
			if(markonly(chan)) {
				chantype = (ChanType*)pc[2];
				if(!(chantype->elem->kind & KindNoPointers)) {
					// Start chanProg.
					chan_ret = pc+3;
					pc = chanProg+1;
					continue;
				}
			}
			pc += 3;
			continue;

		case GC_CHAN:
			// There are no heap pointers in struct Hchan,
			// so we can ignore the leading sizeof(Hchan) bytes.
			if(!(chantype->elem->kind & KindNoPointers)) {
				// Channel's buffer follows Hchan immediately in memory.
				// Size of buffer (cap(c)) is second int in the chan struct.
				chancap = ((uintgo*)chan)[1];
				if(chancap > 0) {
					// TODO(atom): split into two chunks so that only the
					// in-use part of the circular buffer is scanned.
					// (Channel routines zero the unused part, so the current
					// code does not lead to leaks, it's just a little inefficient.)
					*objbufpos++ = (Obj){
						(byte*)chan+runtime·Hchansize, 
						chancap*chantype->elem->size,
						(uintptr)chantype->elem->gc | PRECISE | LOOP
					};
					if(objbufpos == objbuf_end) {
						flushobjbuf(objbuf, &objbufpos, &wp, &wbuf, &nobj);
					}
				}
			}
			if(chan_ret == nil) {
				goto next_block;
			}
			pc = chan_ret;
			continue;

		default:
			runtime·throw("scanblock: invalid GC instruction");
			return;
		}

		if(obj >= arena_start && obj < arena_used) {
			*ptrbufpos++ = (PtrTarget){obj, objti};
			if(ptrbufpos == ptrbuf_end) {
				flushptrbuf(ptrbuf, &ptrbufpos, &wp, &wbuf, &nobj);
			}
		}
	}

	next_block:
		// Done scanning [b, b+n).  Prepare for the next iteration of
		// the loop by setting b, n, ti to the parameters for the next block.

		if(nobj == 0) {
			flushptrbuf(ptrbuf, &ptrbufpos, &wp, &wbuf, &nobj);
			flushobjbuf(objbuf, &objbufpos, &wp, &wbuf, &nobj);

			if(nobj == 0) {
				if(!keepworking) {
					if(wbuf) {
						putempty(wbuf);
					}
					goto endscan; // 这里才是整个函数结束退出的地方
				}
				// Emptied our buffer: refill.
				wbuf = getfull(wbuf);
				if(wbuf == nil) {
					goto endscan; // 这里才是整个函数结束退出的地方
				}
				nobj = wbuf->nobj;
				wp = wbuf->obj + wbuf->nobj;
			}
		}

		// Fetch b from the work buffer.
		--wp;
		b = wp->p;
		n = wp->n;
		ti = wp->ti;
		nobj--;
	} // 这里是第处 for(;;) {} 循环的结束位置.

endscan:;
}

// debug_scanblock debug版本的 scanblock
// 更简单, 单线程, 递归操作, 使用 bitSpecial 作为标记位.
// 不过速度上好像更慢一点.
//
// debug_scanblock is the debug copy of scanblock.
// it is simpler, slower, single-threaded, recursive,
// and uses bitSpecial as the mark bit.
static void debug_scanblock(byte *b, uintptr n)
{
	byte *obj, *p;
	void **vp;
	uintptr size, *bitp, bits, shift, i, xbits, off;
	MSpan *s;

	if(!DebugMark) {
		runtime·throw("debug_scanblock without DebugMark");
	}

	if((intptr)n < 0) {
		runtime·printf("debug_scanblock %p %D\n", b, (int64)n);
		runtime·throw("debug_scanblock");
	}

	// Align b to a word boundary.
	off = (uintptr)b & (PtrSize-1);
	if(off != 0) {
		b += PtrSize - off;
		n -= PtrSize - off;
	}

	vp = (void**)b;
	n /= PtrSize;
	for(i=0; i<n; i++) {
		obj = (byte*)vp[i];

		// Words outside the arena cannot be pointers.
		if((byte*)obj < runtime·mheap.arena_start || (byte*)obj >= runtime·mheap.arena_used) {
			continue;
		}

		// Round down to word boundary.
		obj = (void*)((uintptr)obj & ~((uintptr)PtrSize-1));

		// Consult span table to find beginning.
		s = runtime·MHeap_LookupMaybe(&runtime·mheap, obj);
		if(s == nil) {
			continue;
		}

		p = (byte*)((uintptr)s->start<<PageShift);
		size = s->elemsize;
		if(s->sizeclass == 0) {
			obj = p;
		} else {
			int32 i = ((byte*)obj - p)/size;
			obj = p+i*size;
		}

		// Now that we know the object header, reload bits.
		off = (uintptr*)obj - (uintptr*)runtime·mheap.arena_start;
		bitp = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
		shift = off % wordsPerBitmapWord;
		xbits = *bitp;
		bits = xbits >> shift;

		// Now we have bits, bitp, and shift correct for
		// obj pointing at the base of the object.
		// If not allocated or already marked, done.
		//
		// NOTE: bitSpecial not bitMarked
		if((bits & bitAllocated) == 0 || (bits & bitSpecial) != 0) {
			continue;
		}
		*bitp |= bitSpecial<<shift;
		if(!(bits & bitMarked)) {
			runtime·printf("found unmarked block %p in %p\n", obj, vp+i);
		}

		// If object has no pointers, don't need to scan further.
		if((bits & bitNoScan) != 0) {
			continue;
		}

		debug_scanblock(obj, size);
	}
}

// 	@param obj: work.roots 数组中的某个成员, 其实是 arena 区域中的某个地址.
//
// caller:
// 	1. markroot()
// 	2. scanblock()
//
// Append obj to the work buffer.
// _wbuf, _wp, _nobj are input/output parameters 
// and are specifying the work buffer.
static void enqueue(Obj obj, Workbuf **_wbuf, Obj **_wp, uintptr *_nobj)
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

// 	@param desc: work.markfor, 实际是 markroot(), 是一个函数.
// 	@param i: 为任务索引, work.roots 为全部任务, i 的范围在 [0, (work.nroot-1)]
//
// caller: 
// 	1. gc() 只有这一处, 在 addroots() 之后被设置, 
// 	但实际是在 src/pkg/runtime/parfor.c -> runtime·parfordo() 中, 
// 	作为 desc->body 成员被调用.
static void markroot(ParFor *desc, uint32 i)
{
	Obj *wp;
	Workbuf *wbuf;
	uintptr nobj;

	USED(&desc);
	wp = nil;
	wbuf = nil;
	nobj = 0;
	// runtime·printf("markroot index: %d\n", i);
	enqueue(work.roots[i], &wbuf, &wp, &nobj);
	scanblock(wbuf, wp, nobj, false);
}

// Get an empty work buffer off the work.empty list,
// allocating new buffers as needed.
static Workbuf* getempty(Workbuf *b)
{
	if(b != nil) {
		runtime·lfstackpush(&work.full, &b->node);
	}

	b = (Workbuf*)runtime·lfstackpop(&work.empty);
	if(b == nil) {
		// Need to allocate.
		runtime·lock(&work);
		if(work.nchunk < sizeof *b) {
			work.nchunk = 1<<20;
			work.chunk = runtime·SysAlloc(work.nchunk, &mstats.gc_sys);
			if(work.chunk == nil) {
				runtime·throw("runtime: cannot allocate memory");
			}
		}
		b = (Workbuf*)work.chunk;
		work.chunk += sizeof *b;
		work.nchunk -= sizeof *b;
		runtime·unlock(&work);
	}
	b->nobj = 0;
	return b;
}

static void putempty(Workbuf *b)
{
	if(CollectStats) {
		runtime·xadd64(&gcstats.putempty, 1);
	} 

	runtime·lfstackpush(&work.empty, &b->node);
}

// caller:
// 	1. scanblock() 只有这一处
//
// Get a full work buffer off the work.full list, or return nil.
static Workbuf* getfull(Workbuf *b)
{
	int32 i;

	if(CollectStats) {
		runtime·xadd64(&gcstats.getfull, 1);
	}

	if(b != nil) {
		runtime·lfstackpush(&work.empty, &b->node);
	}

	b = (Workbuf*)runtime·lfstackpop(&work.full);

	if(b != nil || work.nproc == 1) {
		return b;
	}

	runtime·xadd(&work.nwait, +1);
	for(i=0;; i++) {
		if(work.full != 0) {
			runtime·xadd(&work.nwait, -1);
			b = (Workbuf*)runtime·lfstackpop(&work.full);

			if(b != nil) {
				return b;
			} 

			runtime·xadd(&work.nwait, +1);
		}
		
		if(work.nwait == work.nproc) {
			return nil;
		} 

		if(i < 10) {
			m->gcstats.nprocyield++;
			runtime·procyield(20);
		} else if(i < 20) {
			m->gcstats.nosyield++;
			runtime·osyield();
		} else {
			m->gcstats.nsleep++;
			runtime·usleep(100);
		}
	}
}

// caller:
// 	1. enqueue()
static Workbuf* handoff(Workbuf *b)
{
	int32 n;
	Workbuf *b1;

	// Make new buffer with half of b's pointers.
	b1 = getempty(nil);
	n = b->nobj/2;
	b->nobj -= n;
	b1->nobj = n;
	runtime·memmove(b1->obj, b->obj+b->nobj, n*sizeof b1->obj[0]);
	m->gcstats.nhandoff++;
	m->gcstats.nhandoffcnt += n;

	// Put b on full list - let first half of b get stolen.
	runtime·lfstackpush(&work.full, &b->node);
	return b1;
}

// 只是简单地把 obj 对象添加到 work.roots 列表下.
//
// caller:
// 	1. addroots()
// 	2. scaninterfacedata()
// 	3. scanbitvector()
// 	4. addframeroots()
static void addroot(Obj obj)
{
	uint32 cap;
	Obj *new;
	// 为 root 节点扩容
	if(work.nroot >= work.rootcap) {
		cap = PageSize/sizeof(Obj);

		if(cap < 2*work.rootcap) {
			cap = 2*work.rootcap;
		}

		new = (Obj*)runtime·SysAlloc(cap*sizeof(Obj), &mstats.gc_sys);

		if(new == nil) {
			runtime·throw("runtime: cannot allocate memory");
		} 

		if(work.roots != nil) {
			runtime·memmove(new, work.roots, work.rootcap*sizeof(Obj));
			runtime·SysFree(work.roots, work.rootcap*sizeof(Obj), &mstats.gc_sys);
		}
		work.roots = new;
		work.rootcap = cap;
	}
	work.roots[work.nroot] = obj;
	work.nroot++;
}

extern byte pclntab[]; // base for f->ptrsoff

typedef struct BitVector BitVector;
struct BitVector
{
	int32 n;
	uint32 data[];
};

// caller: 
// 	1. scanbitvector() 只有这一处
//
// Scans an interface data value when the interface type indicates that it is a pointer.
static void scaninterfacedata(uintptr bits, byte *scanp, bool afterprologue)
{
	Itab *tab;
	Type *type;

	if(runtime·precisestack && afterprologue) {
		if(bits == BitsIface) {
			tab = *(Itab**)scanp;

			if(tab->type->size <= sizeof(void*) && (tab->type->kind & KindNoPointers)) {
				return;
			}
		} else { 
			// 此时 bits == BitsEface
			type = *(Type**)scanp;

			if(type->size <= sizeof(void*) && 
				(type->kind & KindNoPointers)) {
				return;
			}
		}
	}
	addroot((Obj){scanp+PtrSize, PtrSize, 0});
}

// 这个函数是用来扫描栈空间的, 包括参数列表及本地的局部变量数据.
// 如果参数/局部变量包含指针, 则可能分配在堆区, 否则会分配在栈区本身.
// 所以这里就需要判断参数/局部变量中的数据是否包含指针.
// 而这些变量的元信息都存放在 bitmap 区域, 
// 这里扫描的应该是 bitmap 区域, scanp 表示目标地址.
// 每 BitsPerPointer 个 bits 就可以表示一个该地址是否为一个指针, 
// 这决定了是否继续扫描其指向的地址.
//
// caller: 
// 	1. addframeroots() 只有这一处
//
// Starting from scanp, scans words corresponding to set bits.
static void
scanbitvector(byte *scanp, BitVector *bv, bool afterprologue)
{
	uintptr word, bits;
	uint32 *wordp;
	int32 i, remptrs;

	wordp = bv->data;
	// remptrs(bv->n) 是 wordp(bv->data) 中包含数据的 size 大小,
	// 可以说是 bv->data 尾部地址减去首部地址的差
	for(remptrs = bv->n; remptrs > 0; remptrs -= 32) {
		// word 是数据区域的地址指针
		// uintptr 类型, 这里应该是4字节吧...???
		// 所以 wordp++ 会让其值增加 8 * 4 = 32
		// 应该也正是for循环条件中的, remptrs -= 32 步进长度的原因.
		word = *wordp++;
		if(remptrs < 32) {
			i = remptrs;
		}
		else {
			i = 32;
		}
		i /= BitsPerPointer;

		for(; i > 0; i--) {
			bits = word & 3; // 3 -> 0000 0000 0000 0011
			if(bits != BitsNoPointer && *(void**)scanp != nil) {
				if(bits == BitsPointer) {
					addroot((Obj){scanp, PtrSize, 0});
				}
				else {
					scaninterfacedata(bits, scanp, afterprologue);
				}
			}
			// word 右移, 继续以后两位与 3 做与操作.
			word >>= BitsPerPointer;
			// scanp 增加一个指针的大小
			scanp += PtrSize;
		}
	}
}

// 扫描栈帧, 包括局部变量和函数参数及返回值.
//
// caller: 
// 	1. addstackroots() 只有这一处
//
// Scan a stack frame: local variables and function arguments/results.
static void
addframeroots(Stkframe *frame, void*)
{
	Func *f;
	BitVector *args, *locals;
	uintptr size;
	bool afterprologue;

	f = frame->fn;

	// Scan local variables if stack frame has been allocated.
	// Use pointer information if known.
	// 这部分是扫描本地局部变量(如果此栈帧已分配空间)
	afterprologue = (frame->varp > (byte*)frame->sp);
	if(afterprologue) {
		locals = runtime·funcdata(f, FUNCDATA_GCLocals);
		if(locals == nil) {
			// No locals information, scan everything.
			size = frame->varp - (byte*)frame->sp;
			addroot((Obj){frame->varp - size, size, 0});
		} else if(locals->n < 0) {
			// Locals size information, scan just the locals.
			size = -locals->n;
			addroot((Obj){frame->varp - size, size, 0});
		} else if(locals->n > 0) {
			// Locals bitmap information, scan just the pointers in locals.
			size = (locals->n*PtrSize) / BitsPerPointer;
			scanbitvector(frame->varp - size, locals, afterprologue);
		}
	}

	// Scan arguments.
	// Use pointer information if known.
	// 这里是扫描传入参数.
	args = runtime·funcdata(f, FUNCDATA_GCArgs);
	if(args != nil && args->n > 0) {
		// 如果参数列表不为空
		scanbitvector(frame->argp, args, false);
	}
	else {
		// 如果参数列表为空
		addroot((Obj){frame->argp, frame->arglen, 0});
	}
}

// addroots ...
//
// 由于此时已经经处于 STW 阶段, 所以理论上所有的 g 对象都处于休眠状态.
// 对所有处于 runnable, waiting 和 syscall 状态的g对象都调用此函数.
//
// caller: 
// 	1. addroots() 遍历栈空间时调用
static void addstackroots(G *gp)
{
	M *mp;
	int32 n;
	Stktop *stk;
	uintptr sp, guard, pc, lr;
	void *base;
	uintptr size;

	stk = (Stktop*)gp->stackbase;
	guard = gp->stackguard;
	// g是当前流程所在的协程对象, 这里不对自己的协程进行回收.
	//
	// 不过这里的 if{} 分支虽然没有 return , 而是抛出异常, 不过不影响.
	// 因为主调函数并不需要返回值.
	if(gp == g){
		runtime·throw("can't scan our own stack");
	}
	// 如果执行当前g的m对象是 helpgc 线程, 那么不要执行gc, 是友军.
	if((mp = gp->m) != nil && mp->helpgc) {
		runtime·throw("can't scan gchelper stack");
	}

	if(gp->syscallstack != (uintptr)nil) {
		// 如果gp处于系统调用过程中.

		// Scanning another goroutine that is about to enter
		// or might have just exited a system call. 
		// It may be executing code such as schedlock 
		// and may have needed to start a new stack segment.
		// Use the stack segment and stack pointer at the time of
		// the system call instead, since that won't change underfoot.
		// 
		sp = gp->syscallsp;
		pc = gp->syscallpc;
		lr = 0;
		stk = (Stktop*)gp->syscallstack;
		guard = gp->syscallguard;
	} else {
		// Scanning another goroutine's stack.
		// The goroutine is usually asleep(休眠的) (the world is stopped).
		sp = gp->sched.sp;
		pc = gp->sched.pc;
		lr = gp->sched.lr;

		// For function about to start, context argument is a root too.
		if(gp->sched.ctxt != 0 && 
			runtime·mlookup(gp->sched.ctxt, &base, &size, nil)) {
			addroot((Obj){base, size, 0});
		}
	}
	if(ScanStackByFrames) {
		USED(stk);
		USED(guard);
		runtime·gentraceback(pc, sp, lr, gp, 0, nil, 0x7fffffff, addframeroots, nil, false);
	} else {
		USED(lr);
		USED(pc);
		n = 0;
		// stk 是栈空间顶部 top 区的起始地址
		while(stk) {
			if(sp < guard-StackGuard || (uintptr)stk < sp) {
				runtime·printf(
					"scanstack inconsistent: g%D#%d sp=%p not in [%p,%p]\n", 
					gp->goid, n, sp, guard-StackGuard, stk
				);
				runtime·throw("scanstack");
			}
			addroot((Obj){
				(byte*)sp, 
				(uintptr)stk - sp, 
				(uintptr)defaultProg | PRECISE | LOOP
			});
			sp = stk->gobuf.sp;
			guard = stk->stackguard;
			stk = (Stktop*)stk->stackbase;
			n++;
		}
	}
}

static void addfinroots(void *v)
{
	uintptr size;
	void *base;

	size = 0;
	if(!runtime·mlookup(v, &base, &size, nil) || !runtime·blockspecial(base)) {
		runtime·throw("mark - finalizer inconsistency");
	}

	// do not mark the finalizer block itself. 
	// just mark the things it points at.
	addroot((Obj){base, size, 0});
}

// 这就是添加传说中的根节点吗?
//
// 根节点包含:
//
// 全局变量：程序在编译期就能确定的那些存在于程序整个生命周期的变量。
// 执行栈：每个 goroutine 都包含自己的执行栈，这些执行栈上包含栈上的变量及指向分配的堆内存区块的指针。
// 寄存器：寄存器的值可能表示一个指针，参与计算的这些指针可能指向某些赋值器分配的堆内存区块。
//
// 根节点本来就没几个, work.nroot 基本就在 [5, 11] 范围内.
//
// caller: 
// 	1. gc() 只有这一处.
static void addroots(void)
{
	G *gp;
	FinBlock *fb;
	MSpan *s, **allspans;
	uint32 spanidx;

	work.nroot = 0;

	// gcdata 与 gcbss 都是全局变量
	// data 是数据段, bss 是静态全局变量所在段...这里原来不是堆吗?
	//
	// data & bss
	// TODO(atom): load balancing
	addroot((Obj){data, edata - data, (uintptr)gcdata});
	addroot((Obj){bss, ebss - bss, (uintptr)gcbss});
	// runtime·printf("========= work.nroot: %d\n", work.nroot); // 2

	// 遍历堆空间
	//
	// 注意: 下面的 for{} 循环并没有增加根节点对象.
	//
	// MSpan.types
	allspans = runtime·mheap.allspans;
	for(spanidx=0; spanidx<runtime·mheap.nspan; spanidx++) {
		s = allspans[spanidx];
		if(s->state == MSpanInUse) {
			// The garbage collector ignores type pointers stored in MSpan.types:
			//  - Compiler-generated types are stored outside of heap.
			//  - The reflect package has runtime-generated types cached in its data structures.
			//    The garbage collector relies on finding the references via that cache.
			switch(s->types.compression) {
			case MTypes_Empty:
			case MTypes_Single:
				break; // 这里 break 的是当前的 switch 语句
			case MTypes_Words:
			case MTypes_Bytes:
				markonly((byte*)s->types.data);
				break; // 这里 break 的是当前的 switch 语句
			}
		}
	}
	// runtime·printf("========= work.nroot: %d\n", work.nroot); // 2

	// 遍历栈空间, gc 的目标不只是堆. 除了堆和栈, 还有全局变量区.
	// stacks
	for(gp=runtime·allg; gp!=nil; gp=gp->alllink) {
		switch(gp->status){
		default:
			runtime·printf("unexpected G.status %d\n", gp->status);
			runtime·throw("mark - bad status");
		case Gdead:
			break;
		case Grunning:
			// 此时已经经过 STW, 不应该有 g 还处于运行状态的.
			runtime·throw("mark - world not stopped");
		case Grunnable:
		case Gsyscall:
		case Gwaiting:
			addstackroots(gp);
			break;
		}
	}

	// runtime·printf("========= work.nroot: %d\n", work.nroot); // 5

	runtime·walkfintab(addfinroots);
	// runtime·printf("========= work.nroot: %d\n", work.nroot); // 8

	for(fb=allfin; fb; fb=fb->alllink) {
		addroot((Obj){(byte*)fb->fin, fb->cnt*sizeof(fb->fin[0]), 0});
	}
	// runtime·printf("========= work.nroot: %d\n", work.nroot); // 8

}

static bool handlespecial(byte *p, uintptr size)
{
	FuncVal *fn;
	uintptr nret;
	PtrType *ot;
	Type *fint;
	FinBlock *block;
	Finalizer *f;

	if(!runtime·getfinalizer(p, true, &fn, &nret, &fint, &ot)) {
		runtime·setblockspecial(p, false);
		runtime·MProf_Free(p, size);
		return false;
	}

	runtime·lock(&finlock);
	if(finq == nil || finq->cnt == finq->cap) {
		if(finc == nil) {
			finc = runtime·persistentalloc(PageSize, 0, &mstats.gc_sys);
			finc->cap = (PageSize - sizeof(FinBlock)) / sizeof(Finalizer) + 1;
			finc->alllink = allfin;
			allfin = finc;
		}
		block = finc;
		finc = block->next;
		block->next = finq;
		finq = block;
	}
	f = &finq->fin[finq->cnt];
	finq->cnt++;
	f->fn = fn;
	f->nret = nret;
	f->fint = fint;
	f->ot = ot;
	f->arg = p;
	runtime·unlock(&finlock);
	return true;
}

// sweepspan 找到目标 idx 索引对应的 span 对象, 根据其中数据的 sizeclass 计算 span 中
// 存储的数据量.
// 对于超过32k的大数据, 直接回收到 mheap, 而对于普通小对象, 则要先放到链表中, 再集中回收到
// central 中.
//
// 	@param desc: work.sweepfor 成员;
// 	@param idx: 任务索引(span区域), 范围是[0, runtime·mheap.nspan-1]
//
// caller:
// 	1. gc() 只有这一处. 在 addroots() 之后被设置
// 	但实际是在 src/pkg/runtime/parfor.c -> runtime·parfordo() 中, 
// 	当作 desc->body 被调用.
//
// Sweep frees or collects finalizers for blocks **not marked** in the mark phase.
// It clears the mark bits in preparation for the next GC round.
//
// 被标记过的块才是安全的, 反之, 则需要回收.
static void sweepspan(ParFor *desc, uint32 idx)
{
	int32 cl, n, npages;
	uintptr size;
	byte *p;
	MCache *c;
	byte *arena_start;
	MLink head, *end; 	// 将找到的可释放的小对象组成链表, 一起归还
	int32 nfree; 		// nfree表示找到的可释放的小对象的个数
	byte *type_data;
	byte compression;
	uintptr type_data_inc;
	MSpan *s;

	USED(&desc);
	// 这个 s 才是本次 sweep 的目标
	s = runtime·mheap.allspans[idx];
	if(s->state != MSpanInUse) {
		// 该 span 没有被使用, 没必要回收, 直接返回.
		return;
	}

	arena_start = runtime·mheap.arena_start;
	// p 为 s 的起始地址, 指针类型(start 为页号)
	p = (byte*)(s->start << PageShift);
	cl = s->sizeclass;
	size = s->elemsize;
	if(cl == 0) {
		// size等级为 0, 表示是在 heap 分配的 >32k 的大对象.
		// 这里的 n 应该是 span 包含的...对象的个数?
		n = 1;
	} else {
		// 常规小对象
		// Chunk full of small blocks.
		npages = runtime·class_to_allocnpages[cl];
		n = (npages << PageShift) / size;
	}
	nfree = 0;
	end = &head; // 初始时链表尾部与头部处于同一位置
	c = m->mcache;

	type_data = (byte*)s->types.data;
	type_data_inc = sizeof(uintptr);
	compression = s->types.compression;
	switch(compression) {
	case MTypes_Bytes:
		type_data += 8*sizeof(uintptr);
		type_data_inc = 1;
		break;
	}

	// 从地址 p 开始, size 为步长, 对 n 个对象遍历操作.
	// 目前当前线程拥有这个 span 的控制权, 对其 bitmap 标记的修改无需原子操作
	//
	// Sweep through n objects of given size starting at p.
	// This thread owns the span now, so it can manipulate the block bitmap
	// without atomic operations.
	for(; n > 0; n--, p += size, type_data+=type_data_inc) {
		uintptr off, *bitp, shift, bits;

		// 下面几行计算语句, 根据 obj 对象地址, 反推其在 bitmap 中描述字的地址, 详细解释可见:
		// <!link!>: {32a3d702-70db-4cae-852b-5c12ce491afc}
		off = (uintptr*)p - (uintptr*)arena_start;
		bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
		shift = off % wordsPerBitmapWord;
		bits = *bitp>>shift;

		// 该 block 未被分配, 跳过
		if((bits & bitAllocated) == 0) {
			continue;
		}
		// 该 block 已经被标记过...貌似是标记过的不会被回收, 没被标记的才会?
		if((bits & bitMarked) != 0) {
			if(DebugMark) {
				if(!(bits & bitSpecial)) {
					runtime·printf("found spurious mark on %p\n", p);
				} 
				*bitp &= ~(bitSpecial<<shift);
			}
			*bitp &= ~(bitMarked<<shift);
			continue;
		}

		// Special means it has a finalizer or is being profiled.
		// In DebugMark mode, the bit has been coopted so
		// we have to assume all blocks are special.
		if(DebugMark || (bits & bitSpecial) != 0) {
			if(handlespecial(p, size)) {
				continue;
			}
		}

		// Mark freed; restore block boundary bit.
		*bitp = (*bitp & ~(bitMask<<shift)) | (bitBlockBoundary<<shift);

		if(cl == 0) {
			// 大对象, 直接归还给heap
			//
			// Free large span.
			runtime·unmarkspan(p, 1<<PageShift);
			// needs zeroing
			*(uintptr*)p = (uintptr)0xdeaddeaddeaddeadll;
			runtime·MHeap_Free(&runtime·mheap, s, 1);
			c->local_nlargefree++;
			c->local_largefree += size;
		} else {
			// Free small object.
			switch(compression) {
			case MTypes_Words:
				*(uintptr*)type_data = 0;
				break;
			case MTypes_Bytes:
				*(byte*)type_data = 0;
				break;
			}
			// mark as "needs to be zeroed"
			if(size > sizeof(uintptr)) {
				((uintptr*)p)[1] = (uintptr)0xdeaddeaddeaddeadll;
			}
			// 将可回收的小对象 object 组成链表
			end->next = (MLink*)p;
			end = (MLink*)p;
			nfree++;
		}
	}

	// 如果 nfree 大于0, 表示找了可回收的 object(存放在p链表中).
	// 将可回收的 object 链表归还给 span
	if(nfree) {
		c->local_nsmallfree[cl] += nfree;
		c->local_cachealloc -= nfree * size;
		runtime·MCentral_FreeSpan(
			&runtime·mheap.central[cl], s, nfree, head.next, end
		);
	}
}

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
void
runtime·memorydump(void)
{
	uint32 spanidx;

	for(spanidx=0; spanidx<runtime·mheap.nspan; spanidx++) {
		dumpspan(spanidx);
	}
}

// 执行这个函数的都是参与gc的几个协程, 数量在 [1-nproc] 之间.
//
// caller: 
// 	1. proc.c -> stopm(), 只有这一处.
// 	stopm() 这个函数有一个休眠等待的过程, 被唤醒时, 如果 m->helpgc > 0,
// 	就会调用这个函数, 来辅助执行 gc.
void
runtime·gchelper(void)
{
	// 调用这个做一些准备工作, 比如验证 m->helpgc 的合法性, 判断当前g是否为g0等
	gchelperstart();

	// parallel mark for over gc roots
	runtime·parfordo(work.markfor);

	// help other threads scan secondary blocks
	scanblock(nil, nil, 0, true);

	if(DebugMark) {
		// wait while the main thread executes mark(debug_scanblock)
		while(runtime·atomicload(&work.debugmarkdone) == 0) {
			runtime·usleep(10);
		}
	}

	runtime·parfordo(work.sweepfor);

	bufferList[m->helpgc].busy = 0;
	// ndone+1 后如果等于 nproc-1, 则说明这是最后一个完成gc的线程, 
	// 可以唤醒 alldone 处等待的协程了.
	if(runtime·xadd(&work.ndone, +1) == work.nproc-1) {
		runtime·notewakeup(&work.alldone);
	}
}

// Structure of arguments passed to function gc().
// This allows the arguments to be passed via runtime·mcall.
// 传入gc()函数的的参数结构体(注意不是runtime·gc()),
// 这允许通过runtime·mcall()传递参数.
struct gc_args
{
	// start time of GC in ns (just before stoptheworld)
	int64 start_time; 
};

static void gc(struct gc_args *args);
static void mgc(G *gp);


static FuncVal runfinqv = {runfinq};
extern int32 gcpercent;

// runtime·gc 先进入 STW 阶段, 然后调用 runtime·mcall(mgc) 在 g0 上进行实际 gc 行为,
// 完成后, 再解除 STW, 重新启用调度器.
//
// 	@param force: 是否为强制 gc(其实应该叫例行 gc) 普通 gc 是看空间使用量, 到达一定比率开始执行.
// 	如果2分钟内没有被动 gc, 则进行主动的例行 gc.
//
// caller:
// 	1. src/pkg/runtime/malloc.goc -> runtime·mallocgc() force 参数 = false
// 	分配可gc对象的同时, 检查空间使用情况, 满足条件时调用本函数进行一次 gc.
// 	2. src/pkg/runtime/mheap.c -> forcegchelper() force 参数 = true
// 	2分钟内没有进行 gc 时, 则进行例行 gc(也叫强制 gc)
// 	在进程启动初期就创建一个协程完成这项工作(runtime·MHeap_Scavenger())
// 	3. runfinq()
// 	还没想通使用场景
// 	4. src/pkg/runtime/mheap.c -> runtime∕debug·freeOSMemory()
// 	调试使用
// 	5. GC() runtime 标准库中的 GC() 函数, 由开发者主动调用
void runtime·gc(int32 force)
{
	struct gc_args a;
	int32 i;

	// 在 empty/full 这两个 uint64 类型的成员没有在 uint64 边界上对齐时, 
	// 原子操作其实并不原子...
	// 这个问题在之前就出现了.
	// 7 == 0111
	// work对象在当前文件的开头定义(第200行左右)
	//
	// The atomic operations are not atomic if the uint64s
	// are not aligned on uint64 boundaries. 
	// This has been a problem in the past.
	if((((uintptr)&work.empty) & 7) != 0) {
		runtime·throw("runtime: gc work buffer is misaligned");
	}
	if((((uintptr)&work.full) & 7) != 0) {
		runtime·throw("runtime: gc work buffer is misaligned");
	}

	// 如下情况退出GC
	// 1. enablegc == 0 表示 runtime 还未启动完成, 该值在 runtime·schedinit() 末尾才被赋值为 1
	// 2. 很多持有lock锁的库中会调用malloc, 为了避免优先级混乱的问题, 在持有锁的函数中就不要调用gc了.
	// malloc下面的mallocgc可以在没有锁的情况下进行gc操作.
	//
	// The gc is turned off (via enablegc) until the bootstrap has completed.
	// Also, malloc gets called in the guts of a number of libraries
	// that might be holding locks. 
	// To avoid priority inversion problems, 
	// don't bother trying to run gc while holding a lock. 
	// The next mallocgc without a lock will do the gc instead.
	if(!mstats.enablegc || g == m->g0 || m->locks > 0 || runtime·panicking) {
		return;
	}

	// 首次调用(gcpercent 初始值就是 GcpercentUnknown)
	//
	// first time through
	if(gcpercent == GcpercentUnknown) {
		runtime·lock(&runtime·mheap);
		// 第一次执行, 调用 readgogc() 从环境变量中取 GOGC 设置. 
		// GOGC == off 时得到 -1, 其他的默认情况下返回 100
		if(gcpercent == GcpercentUnknown) {
			gcpercent = readgogc();
		} 

		runtime·unlock(&runtime·mheap);
	}
	// 当 GOGC == off 时, gcpercent 的值将取-1, 相当于关闭GC.
	if(gcpercent < 0) {
		return;
	} 

	// gc STW 前尝试获取全局锁
	runtime·semacquire(&runtime·worldsema, false);

	// 如果不是强制(例行) gc, 而此时还没有 next_gc 的时间时, 就退出了...
	// 这种情况适用于第1种主调函数
	if(!force && mstats.heap_alloc < mstats.next_gc) {
		// typically threads which lost the race to grab worldsema exit here when gc is done.
		runtime·semrelease(&runtime·worldsema);
		return;
	}

	// Ok, we're doing it! Stop everybody else
	a.start_time = runtime·nanotime();
	m->gcing = 1;
	runtime·stoptheworld();

	// GODEBUG="gotrace=2" 会引发两次回收
	//
	// Run gc on the g0 stack. 
	// We do this so that the g stack we're currently running on will no longer change.
	// Cuts the root set down a bit
	// (g0 stacks are not scanned, and we don't need to scan gc's internal state).
	// Also an enabler for copyable stacks.
	for(i = 0; i < (runtime·debug.gctrace > 1 ? 2 : 1); i++) {
		// 调用 runtime·mcall(mgc), 在 g0 上进行实际的 gc 操作.
		//
		// switch to g0, call gc(&a), then switch back
		g->param = &a;
		g->status = Gwaiting;
		g->waitreason = "garbage collection";
		// mgc上下面定义的函数名, 是实际执行GC的函数.
		// runtime·mcall 是汇编代码, 原型为void mcall(void (*fn)(G*))
		// 作用是切换到 m->g0 的上下文, 并调用mgc(g).
		// 注意参数g应该就是此处的局部变量g.
		runtime·mcall(mgc);
		// record a new start time in case we're going around again
		a.start_time = runtime·nanotime();
	}

	// gc 完成后.
	//
	// all done
	m->gcing = 0;
	m->locks++;
	// gc 后释放全局锁, 
	runtime·semrelease(&runtime·worldsema);
	runtime·starttheworld();
	m->locks--;

	// 现在 gc 已经完成, 如果有需要, 则启动 finalizer 线程.
	// finq 等待执行的finalizer链表
	//
	// now that gc is done, kick off finalizer thread if needed
	if(finq != nil) {
		runtime·lock(&finlock);
		// fing 用来执行finalizer的线程对象, 指针类型
		// 如果finq不空, 但fing为nil, 就创建一个G协程, 来运行runfinqv.
		// 而funfinqv其实就是runqinq函数.
		//
		// kick off or wake up goroutine to run queued finalizers
		if(fing == nil) {
			fing = runtime·newproc1(&runfinqv, nil, 0, 0, runtime·gc);
		}
		else if(fingwait) {
			// 如果fingwait, 表示此时fing中的g对象都为Gwaiting状态.
			// 这里设置其值为0, 并调用runtime·ready()将其转换为Grunnable状态.
			// ...runtime·newproc1()创建的g对象的状态就是Grunnable.
			fingwait = 0;
			// 不过runtime·ready()貌似是只设置单个g对象的状态, 难道fing就只有一个?
			runtime·ready(fing);
		}
		runtime·unlock(&finlock);
	}
	// 使已经在排队的 finalizer 有机会执行...
	// 话说 startworld 并不表示重新开始调度吗???
	//
	// give the queued finalizers, if any, a chance to run
	runtime·gosched();
}

// 	@param gp: 主调函数所在的 g 对象
//
// caller: 
// 	1. runtime·gc() 在进入 STW 阶段后, 调用 runtime·mcall(mgc) 在 g0 上执行该函数
static void mgc(G *gp)
{
	gc(gp->param);
	gp->param = nil;
	gp->status = Grunning;
	runtime·gogo(&gp->sched);
}

extern updatememstats(GCStats *stats);
// 确定参与 gc 的协程数量, 然后调用 addroots() 添加根节点.
//
// caller:
// 	1. mgc() 只有这一处, mgc()则是由 runtime·gc()调用的, 该函数是在 g0 上执行的.
static void gc(struct gc_args *args)
{
	int64 t0, t1, t2, t3, t4;
	uint64 heap0, heap1, obj0, obj1, ninstr;
	GCStats stats;
	M *mp;
	uint32 i;
	Eface eface;

	t0 = args->start_time;

	// 清空 gcstats, 记录本次 gc 的数据
	if(CollectStats) {
		runtime·memclr((byte*)&gcstats, sizeof(gcstats));
	} 

	for(mp=runtime·allm; mp; mp=mp->alllink) {
		runtime·settype_flush(mp);
	}

	heap0 = 0;
	obj0 = 0;
	if(runtime·debug.gctrace) {
		updatememstats(nil);
		heap0 = mstats.heap_alloc;
		obj0 = mstats.nmalloc - mstats.nfree;
	}

	// ...这还要啥disable啊, 现在就是gc啊
	//
	// disable gc during mallocs in parforalloc
	m->locks++;
	if(work.markfor == nil) {
		work.markfor = runtime·parforalloc(MaxGcproc);
	}
	if(work.sweepfor == nil) {
		work.sweepfor = runtime·parforalloc(MaxGcproc);
	}
	m->locks--;

	if(itabtype == nil) {
		// get C pointer to the Go type "itab"
		runtime·gc_itab_ptr(&eface);
		itabtype = ((PtrType*)eface.type)->elem;
	}

	work.nwait = 0;
	work.ndone = 0;
	work.debugmarkdone = 0;
	// 确定并⾏回收的 goroutine 数量 = min(GOMAXPROCS, ncpu, MaxGcproc).
	work.nproc = runtime·gcprocs();
	addroots();
	// 将 work.markfor 成员的 body 字段, 设置为 markroot 函数
	runtime·parforsetup(work.markfor, work.nproc, work.nroot, nil, false, markroot);
	// 将 work.sweepfor 成员的 body 字段, 设置为 sweepspan 函数
	runtime·parforsetup(work.sweepfor, work.nproc, runtime·mheap.nspan, nil, true, sweepspan);
	// 如果执行 gc 操作的协程数量大于1, 那么在进行之后的操作之前, 需要保证这些协程全部完成才行.
	// 所以下面有 notesleep(alldone) 的语句, 而在 sleep 之前, 则需要使用 clear 对目标锁对象清零.
	if(work.nproc > 1) {
		runtime·noteclear(&work.alldone);
		runtime·helpgc(work.nproc);
	}

	t1 = runtime·nanotime();

	gchelperstart();
	// 并行 mark, 执行上面设置的 markroot() 函数
	runtime·parfordo(work.markfor);
	// ...这参数都是nil, 效果是啥???
	// 对了, 在parfordo中循环调用的 markfor, 也就是 markroot 函数中, 最后一句就是 scanblock.
	// 那么, 这里的scanblock...是查漏补缺的? 但好像也没什么实际作用啊.
	scanblock(nil, nil, 0, true);

	if(DebugMark) {
		for(i=0; i<work.nroot; i++){
			debug_scanblock(work.roots[i].p, work.roots[i].n);
		}
		runtime·atomicstore(&work.debugmarkdone, 1);
	}

	t2 = runtime·nanotime();

	// 并行 sweep, 执行上面设置的 sweepspan() 函数
	runtime·parfordo(work.sweepfor);
	bufferList[m->helpgc].busy = 0;

	t3 = runtime·nanotime();
	
	// 如果执行GC的协程数量多于1个, 那就等待直到ta们全部完成.
	// wakeup 的操作在 runtime·gchelper() 函数中.
	// ...如果只有1个就不用了? 只有一个协程时运行到这里说明sweepfor已经完成了是吗???
	if(work.nproc > 1) {
		runtime·notesleep(&work.alldone);
	}

	// 统计数据
	cachestats();
	// 计算下一次GC触发的时间...好像不能说是时间, 而是空间吧.
	// next_gc总是与heap_alloc进行比较, 应该是每增长百分之gcpercent就进行一次GC吧.
	mstats.next_gc = mstats.heap_alloc+mstats.heap_alloc*gcpercent/100;

	t4 = runtime·nanotime();
	mstats.last_gc = t4; // 设置最近一次gc的时间(绝对时间)
	mstats.pause_ns[mstats.numgc%nelem(mstats.pause_ns)] = t4 - t0;
	mstats.pause_total_ns += t4 - t0;
	mstats.numgc++; // gc次数加1
	if(mstats.debuggc){
		runtime·printf("pause %D\n", t4-t0);
	}

	// 打印gc调试信息
	// GODEBUG=gctrace=1 
	if(runtime·debug.gctrace) {
		updatememstats(&stats);
		heap1 = mstats.heap_alloc;
		obj1 = mstats.nmalloc - mstats.nfree;

		stats.nprocyield += work.sweepfor->nprocyield;
		stats.nosyield += work.sweepfor->nosyield;
		stats.nsleep += work.sweepfor->nsleep;

		runtime·printf(
			"gc%d(%d): %D+%D+%D ms, %D -> %D MB %D -> %D (%D-%D) objects,"
			" %D(%D) handoff, %D(%D) steal, %D/%D/%D yields\n",
			mstats.numgc, work.nproc, (t2-t1)/1000000, (t3-t2)/1000000, (t1-t0+t4-t3)/1000000,
			heap0>>20, heap1>>20, obj0, obj1, mstats.nmalloc, mstats.nfree,
			stats.nhandoff, stats.nhandoffcnt,
			work.sweepfor->nsteal, work.sweepfor->nstealcnt,
			stats.nprocyield, stats.nosyield, stats.nsleep
		);
		if(CollectStats) {
			runtime·printf(
				"scan: %D bytes, %D objects, %D untyped, %D types from MSpan\n",
				gcstats.nbytes, gcstats.obj.cnt, gcstats.obj.notype, gcstats.obj.typelookup
			);
			if(gcstats.ptr.cnt != 0) {
				runtime·printf(
					"avg ptrbufsize: %D (%D/%D)\n",
					gcstats.ptr.sum/gcstats.ptr.cnt, gcstats.ptr.sum, gcstats.ptr.cnt
				);
			}
			if(gcstats.obj.cnt != 0) {
				runtime·printf(
					"avg nobj: %D (%D/%D)\n",
					gcstats.obj.sum/gcstats.obj.cnt, gcstats.obj.sum, gcstats.obj.cnt
				);
			}
			runtime·printf(
				"rescans: %D, %D bytes\n", gcstats.rescan, gcstats.rescanbytes
			);

			runtime·printf("instruction counts:\n");
			ninstr = 0;
			for(i=0; i<nelem(gcstats.instr); i++) {
				runtime·printf("\t%d:\t%D\n", i, gcstats.instr[i]);
				ninstr += gcstats.instr[i];
			}
			runtime·printf("\ttotal:\t%D\n", ninstr);

			runtime·printf(
				"putempty: %D, getfull: %D\n", gcstats.putempty, gcstats.getfull
			);

			runtime·printf(
				"markonly base lookup: bit %D word %D span %D\n", 
				gcstats.markonly.foundbit, 
				gcstats.markonly.foundword, 
				gcstats.markonly.foundspan
			);
			runtime·printf(
				"flushptrbuf base lookup: bit %D word %D span %D\n", 
				gcstats.flushptrbuf.foundbit, 
				gcstats.flushptrbuf.foundword, 
				gcstats.flushptrbuf.foundspan
			);
		}
	}

	runtime·MProf_GC();
}

// 做一些准备工作, 比如验证 m->helpgc 的合法性, 判断当前g是否为g0等
//
// caller: 
// 	1. runtime·gchelper()
// 	2. gc()
static void gchelperstart(void)
{
	if(m->helpgc < 0 || m->helpgc >= MaxGcproc) {
		runtime·throw("gchelperstart: bad m->helpgc");
	}
	if(runtime·xchg(&bufferList[m->helpgc].busy, 1)) {
		runtime·throw("gchelperstart: already busy");
	}
	if(g != m->g0) {
		runtime·throw("gchelper not running on g0 stack");
	}
}

// caller:
// 	1. 这个函数的调用有点复杂
static void runfinq(void)
{
	Finalizer *f;
	FinBlock *fb, *next;
	byte *frame;
	uint32 framesz, framecap, i;
	Eface *ef, ef1;

	frame = nil;
	framecap = 0;
	for(;;) {
		runtime·lock(&finlock);
		fb = finq;
		finq = nil;
		if(fb == nil) {
			fingwait = 1;
			runtime·park(runtime·unlock, &finlock, "finalizer wait");
			continue;
		}
		runtime·unlock(&finlock);
		if(raceenabled) {
			runtime·racefingo();
		} 
		for(; fb; fb=next) {
			next = fb->next;
			for(i=0; i<fb->cnt; i++) {
				f = &fb->fin[i];
				framesz = sizeof(Eface) + f->nret;
				if(framecap < framesz) {
					runtime·free(frame);
					// The frame does not contain pointers interesting for GC,
					// all not yet finalized objects are stored in finc.
					// If we do not mark it as FlagNoScan,
					// the last finalized object is not collected.
					frame = runtime·mallocgc(framesz, 0, FlagNoScan|FlagNoInvokeGC);
					framecap = framesz;
				}
				if(f->fint == nil) {
					runtime·throw("missing type in runfinq");
				} 
				if(f->fint->kind == KindPtr) {
					// direct use of pointer
					*(void**)frame = f->arg;
				} else if(((InterfaceType*)f->fint)->mhdr.len == 0) {
					// convert to empty interface
					ef = (Eface*)frame;
					ef->type = f->ot;
					ef->data = f->arg;
				} else {
					// convert to interface with methods, via empty interface.
					ef1.type = f->ot;
					ef1.data = f->arg;
					if(!runtime·ifaceE2I2((InterfaceType*)f->fint, ef1, (Iface*)frame)) {
						runtime·throw("invalid type conversion in runfinq");
					}
				}
				reflect·call(f->fn, frame, framesz);
				f->fn = nil;
				f->arg = nil;
				f->ot = nil;
			}
			fb->cnt = 0;
			fb->next = finc;
			finc = fb;
		}
		// trigger another gc to clean up the finalized objects, if possible
		runtime·gc(1);
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
