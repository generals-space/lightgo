// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Memory allocator, based on tcmalloc.
// http://goog-perftools.sourceforge.net/doc/tcmalloc.html

// The main allocator works in runs of(一连串的) pages.
// Small allocation sizes (up to and including 32 kB) are
// rounded to one of about 100 size classes, each of which
// has its own free list of objects of exactly that size.
// Any free page of memory can be split into a set of objects
// of one size class, which are then managed using free list
// allocators.
//
// The allocator's data structures are:
//
//	FixAlloc: a free-list allocator for fixed-size objects,
//		used to manage storage used by the allocator.
//	MHeap: the malloc heap, managed at page (4096-byte) granularity.
//	MSpan: a run of pages managed by the MHeap.
//	MCentral: a shared free list for a given size class.
//	MCache: a per-thread (in Go, per-M) cache for small objects.
//	MStats: allocation statistics.
//
// Allocating a small object proceeds up a hierarchy of caches:
//
//	1. Round the size up to one of the small size classes
//	   and look in the corresponding MCache free list.
//	   If the list is not empty, allocate an object from it.
//	   This can all be done without acquiring a lock.
//
//	2. If the MCache free list is empty, replenish it by
//	   taking a bunch of objects from the MCentral free list.
//	   Moving a bunch amortizes the cost of acquiring the MCentral lock.
//
//	3. If the MCentral free list is empty, replenish it by
//	   allocating a run of pages from the MHeap and then
//	   chopping that memory into a objects of the given size.
//	   Allocating many objects amortizes the cost of locking
//	   the heap.
//
//	4. If the MHeap is empty or has no page runs large enough,
//	   allocate a new group of pages (at least 1MB) from the
//	   operating system.  Allocating a large run of pages
//	   amortizes the cost of talking to the operating system.
//
// Freeing a small object proceeds up the same hierarchy:
//
//	1. Look up the size class for the object and add it to
//	   the MCache free list.
//
//	2. If the MCache free list is too long or the MCache has
//	   too much memory, return some to the MCentral free lists.
//
//	3. If all the objects in a given span have returned to
//	   the MCentral list, return that span to the page heap.
//
//	4. If the heap has too much memory, return some to the
//	   operating system.
//
//	TODO(rsc): Step 4 is not implemented.
//
// Allocating and freeing a large object uses the page heap
// directly, bypassing the MCache and MCentral free lists.
//
// The small objects on the MCache and MCentral free lists
// may or may not be zeroed.  They are zeroed if and only if
// the second word of the object is zero.  The spans in the
// page heap are always zeroed.  When a span full of objects
// is returned to the page heap, the objects that need to be
// are zeroed first.  There are two main benefits to delaying the
// zeroing this way:
//
//	1. stack frames allocated from the small object lists
//	   can avoid zeroing altogether.
//	2. the cost of zeroing when reusing a small object is
//	   charged to the mutator, not the garbage collector.
//
// This C code was written with an eye toward translating to Go
// in the future.  Methods have the form Type_Method(Type *t, ...).

typedef struct MCentral	MCentral;
typedef struct MHeap	MHeap;
typedef struct MSpan	MSpan;
typedef struct MStats	MStats;
typedef struct MLink	MLink;
typedef struct MTypes	MTypes;
typedef struct GCStats	GCStats;

enum
{
	// 1 << 12 = 4k, 正好是一页的大小
	PageShift	= 12,
	PageSize	= 1<<PageShift,
	PageMask	= PageSize - 1,
};
typedef	uintptr	PageID;		// address >> PageShift

enum
{
	// Computed constant. 
	// The definition of MaxSmallSize and the algorithm in msize.c produce some number of different allocation size classes. 
	// NumSizeClasses is that number. 
	// It's needed here because there are static arrays of this length; 
	// when msize runs its size choosing algorithm it double-checks that NumSizeClasses agrees.
	NumSizeClasses = 61,

	// 小对象大小上限, 32k, 超过这个值就不会在span链表中分配而是直接到heap分配
	// 1 << 10 = 1024, 32 << 10 = 32k
	//
	// Tunable constants.
	MaxSmallSize = 32<<10,

	// FixAlloc 对象 chunk 成员的固定大小, 16k.
	//
	// Chunk size for FixAlloc
	FixAllocChunk = 16<<10,

	// MHeap 中定长列表的最大页长度, 
	// 用于修饰 MHeap 中的free字段 MSpan 列表(即MaxMHeapList为MSpan数组的长度值)
	// 我想应该是这样的, MHeap中有free成员, 类型为 MSpan 数组, 用于为固定大小的小对象分配空间.
	// 
	// Maximum page length for fixed-size list in MHeap.
	MaxMHeapList = 1<<(20 - PageShift),
	// 堆增长的块大小, 1 << 20 = 1M
	// ...难道说堆空间不足时向OS申请内存的单位为1M? 会不会有点小?
	//
	// Chunk size for heap growth
	HeapAllocChunk = 1<<20,

	// Number of bits in page to span calculations (4k pages).
	// On Windows 64-bit we limit the arena to 32GB or 35 bits (see below for reason).
	// On other 64-bit platforms, we limit the arena to 128GB, or 37 bits.
	// On 32-bit, we don't bother limiting anything, so we use the full 32-bit address.
#ifdef _64BIT
#ifdef GOOS_windows
	// Windows counts memory used by page table into committed memory
	// of the process, so we can't reserve too much memory.
	// See http://golang.org/issue/5402 and http://golang.org/issue/5236.
	MHeapMap_Bits = 35 - PageShift,
#else
	MHeapMap_Bits = 37 - PageShift,
#endif
#else
	MHeapMap_Bits = 32 - PageShift,
#endif

	// Max number of threads to run garbage collection.
	// 2, 3, and 4 are all plausible maximums depending
	// on the hardware details of the machine. 
	// The garbage collector scales well to 8 cpus.
	// 可执行GC的最大线程数.
	MaxGcproc = 8,
};

// Maximum memory allocation size, a hint for callers.
// This must be a #define instead of an enum because it
// is so large.
#ifdef _64BIT
#define	MaxMem	(1ULL<<(MHeapMap_Bits+PageShift))	/* 128 GB or 32 GB */
#else
#define	MaxMem	((uintptr)-1)
#endif

// 这是一个普通单向链表
//
// A generic linked list of blocks. 
// (Typically the block is bigger than sizeof(MLink).)
struct MLink
{
	// 这是一个普通单向链表
	MLink *next;
};

// SysAlloc obtains a large chunk of zeroed memory from the
// operating system, typically on the order of a hundred kilobytes
// or a megabyte.
//
// SysUnused notifies the operating system that the contents
// of the memory region are no longer needed and can be reused
// for other purposes.
// SysUsed notifies the operating system that the contents
// of the memory region are needed again.
//
// SysFree returns it unconditionally; this is only used if
// an out-of-memory error has been detected midway through
// an allocation.  It is okay if SysFree is a no-op.
//
// SysReserve reserves address space without allocating memory.
// If the pointer passed to it is non-nil, the caller wants the
// reservation there, but SysReserve can still choose another
// location if that one is unavailable.
//
// SysMap maps previously reserved address space for use.

void*	runtime·SysAlloc(uintptr nbytes, uint64 *stat);
void	runtime·SysFree(void *v, uintptr nbytes, uint64 *stat);
void	runtime·SysUnused(void *v, uintptr nbytes);
void	runtime·SysUsed(void *v, uintptr nbytes);
void	runtime·SysMap(void *v, uintptr nbytes, uint64 *stat);
void*	runtime·SysReserve(void *v, uintptr nbytes);

// FixAlloc is a simple free-list allocator for fixed size objects.
// Malloc uses a FixAlloc wrapped around SysAlloc to manages its MCache and MSpan objects.
// FixAlloc 是一个简单的空闲列表分配器, 用于为固定大小的对象分配空间.
// 其中的 list 成员为链表类型, 就是用来分配空间的.
// Malloc 引入 FixAlloc 对象封装 SysAlloc, 只用来管理ta的 MCache 和 MSpan 对象.
// ...实际上也只有 heap 的 spanalloc 和 cachealloc 两个成员是 FixAlloc 对象.
// 
// Memory returned by FixAlloc_Alloc is not zeroed.
// The caller is responsible for locking around FixAlloc calls.
// Callers can keep state in the object but the first word is
// smashed by freeing and reallocating.
// FixAlloc_Alloc()返回的内存空间并未清零.
// 调用者需要在调用 FixAlloc_XXX 相关函数时加锁.
// 
struct FixAlloc
{
	// 分别为 sizeof(MSpan), sizeof(MCache) 的大小
	uintptr	size;
	// 在初始化 MHeap->spanalloc 时, 被赋值为 src/pkg/runtime/mheap.c -> RecordSpan()
	// 在初始化 MHeap->cachealloc 时, 此值为 nil
	//
	// called first time p is returned
	void	(*first)(void *arg, byte *p);
	void*	arg;
	// 这个链表里可能为 MSpan, 也可能为 MCache
	MLink*	list;
	byte*	chunk;
	uint32	nchunk;
	uintptr	inuse;	// in-use bytes now
	uint64*	stat;
};

void	runtime·FixAlloc_Init(FixAlloc *f, uintptr size, void (*first)(void*, byte*), void *arg, uint64 *stat);
void*	runtime·FixAlloc_Alloc(FixAlloc *f);
void	runtime·FixAlloc_Free(FixAlloc *f, void *p);


// Statistics.
// Shared with Go: if you edit this structure, also edit type MemStats in mem.go.
struct MStats
{
	// General statistics.
	
	// 已分配且正在使用的字节数
	//
	// bytes allocated and still in use 
	uint64	alloc;
	// 所有已分配的字节数, 包括已释放, 处于空闲状态的空间.
	//
	// bytes allocated (even if freed) 
	uint64	total_alloc;

	// sys 从系统获取的字节数, 其值近似等于下面的 xxx_sys 对象的总和
	//
	// bytes obtained from system 
	// (should be sum of xxx_sys below, no locking, approximate)
	uint64	sys;
	uint64	nlookup;	// number of pointer lookups
	uint64	nmalloc;	// number of mallocs
	uint64	nfree;		// number of frees

	// Statistics about malloc heap.
	// protected by mheap.Lock

	// bytes allocated and still in use
	uint64	heap_alloc;	
	uint64	heap_sys;	// bytes obtained from system
	uint64	heap_idle;	// bytes in idle spans
	uint64	heap_inuse;	// bytes in non-idle spans
	uint64	heap_released;	// bytes released to the OS
	uint64	heap_objects;	// total number of allocated objects

	// Statistics about allocation of low-level fixed-size structures.
	// Protected by FixAlloc locks.
	uint64	stacks_inuse;	// bootstrap stacks
	uint64	stacks_sys;
	uint64	mspan_inuse;	// MSpan structures
	uint64	mspan_sys;
	uint64	mcache_inuse;	// MCache structures
	uint64	mcache_sys;
	uint64	buckhash_sys;	// profiling bucket hash table
	uint64	gc_sys;
	uint64	other_sys;

	// Statistics about garbage collector.
	// Protected by mheap or stopping the world during GC.

	// next GC (in heap_alloc time)
	// 下一次GC触发的时间...好像不能说是时间, 而是空间吧.
	// 这个值总是用来与heap_alloc做比较
	uint64	next_gc;
	// last GC (in absolute time)
	// GC完成时通过runtime·nanotime()获取到的系统时间.
	uint64  last_gc;
	uint64	pause_total_ns;
	uint64	pause_ns[256];
	// 进程启动以来, 执行过的 gc 次数.
	uint32	numgc;
	bool	enablegc;
	bool	debuggc;

	// Statistics about allocation size classes.
	struct {
		uint32 size;
		uint64 nmalloc;
		uint64 nfree;
	} by_size[NumSizeClasses];
};

#define mstats runtime·memStats	/* name shared with Go */
extern MStats mstats;

// Size classes.  Computed and initialized by InitSizes.
//
// SizeToClass(0 <= n <= MaxSmallSize) returns the size class,
//	1 <= sizeclass < NumSizeClasses, for n.
//	Size class 0 is reserved to mean "not small".
//
// class_to_size[i] = largest size in class i
// class_to_allocnpages[i] = number of pages to allocate when
//	making new objects in class i

int32	runtime·SizeToClass(int32);
extern	int32	runtime·class_to_size[NumSizeClasses];
extern	int32	runtime·class_to_allocnpages[NumSizeClasses];
extern	int8	runtime·size_to_class8[1024/8 + 1];
extern	int8	runtime·size_to_class128[(MaxSmallSize-1024)/128 + 1];
extern	void	runtime·InitSizes(void);


// Per-thread (in Go, per-M) cache for small objects.
// No locking needed because it is per-thread (per-M).
typedef struct MCacheList MCacheList;
struct MCacheList
{
	// 当前 sizeclass 级别的小块空间链表的链表头
	//
	// 还有一个同级成员, 表示该链表的长度
	MLink *list;
	uint32 nlist;
};

struct MCache
{
	// The following members are accessed on every malloc,
	// so they are grouped here for better caching.
	// trigger heap sample after allocating this many bytes
	int32 next_sample;
	// 标记此 cache 对象已经分配出去(不管是正在使用或是已经空闲...???)的大小
	//
	// bytes allocated (or freed) from cache since last lock of heap
	intptr local_cachealloc;
	// 不同级别的 MCache 数组, 每个成员表示一个级别的链表头
	//
	// The rest is not accessed on every malloc.
	MCacheList list[NumSizeClasses];
	// Local allocator stats, flushed during GC.
	uintptr local_nlookup;		// number of pointer lookups
	uintptr local_largefree;	// bytes freed for large objects (>MaxSmallSize)
	uintptr local_nlargefree;	// number of frees for large objects (>MaxSmallSize)
	uintptr local_nsmallfree[NumSizeClasses];	// number of frees for small objects (<=MaxSmallSize)
};

void	runtime·MCache_Refill(MCache *c, int32 sizeclass);
void	runtime·MCache_Free(MCache *c, void *p, int32 sizeclass, uintptr size);
void	runtime·MCache_ReleaseAll(MCache *c);

// MTypes describes the types of blocks allocated within a span.
// The compression field describes the layout of the data.
// MTypes结构描述了在span中分配的块的类型, 
// 其中compression成员描述了data成员的布局(compression可取MTypes_XXX)
// MTypes_Empty:
//     All blocks are free, or no type information is available for allocated blocks.
//     The data field has no meaning.
// MTypes_Single:
//     The span contains just one block.
//     The data field holds the type information.
//     The sysalloc field has no meaning.
// MTypes_Words:
//     The span contains multiple blocks.
//     The data field points to an array of type [NumBlocks]uintptr,
//     and each element of the array holds the type of the corresponding
//     block.
// MTypes_Bytes:
//     The span contains at most seven different types of blocks.
//     The data field points to the following structure:
//         struct {
//             type  [8]uintptr       // type[0] is always 0
//             index [NumBlocks]byte
//         }
//     The type of the i-th block is: data.type[data.index[i]]
enum
{
	MTypes_Empty = 0,
	MTypes_Single = 1,
	MTypes_Words = 2,
	MTypes_Bytes = 3,
};
struct MTypes
{
	byte	compression;	// one of MTypes_*
	uintptr	data;
};

// An MSpan is a run of pages.
enum
{
	MSpanInUse = 0,
	MSpanFree,
	MSpanListHead,
	MSpanDead,
};

struct MSpan
{
	// 双向链表
	MSpan	*next;		// in a span linked list
	MSpan	*prev;		// in a span linked list

	// 起始页的页号...是可以推算出来的. 
	// start << PageShift可以用来表示真正的内存地址.
	//
	// starting page number
	PageID	start;
	// 该span中页的数量, 同一sizeclass的MSpan对象中npages的值相同.
	//
	// number of pages in span
	uintptr	npages;
	// list of free objects
	// 该span中空闲的对象链表
	MLink	*freelist;
	// number of allocated objects in this span
	// 当前span中已分配的对象的数量.
	uint32	ref;
	int32	sizeclass;	// size class
	uintptr	elemsize;	// computed from sizeclass or from npages
	uint32	state;		// MSpanInUse etc
	int64   unusedsince;	// First time spotted by GC in MSpanFree state
	uintptr npreleased;	// number of pages released to the OS
	byte	*limit;		// end of data in span
	MTypes	types;		// types of allocated objects in this span
};

void	runtime·MSpan_Init(MSpan *span, PageID start, uintptr npages);

// Every MSpan is in one doubly-linked list,
// either one of the MHeap's free lists or one of the
// MCentral's span lists.  We use empty MSpan structures as list heads.
void	runtime·MSpanList_Init(MSpan *list);
bool	runtime·MSpanList_IsEmpty(MSpan *list);
void	runtime·MSpanList_Insert(MSpan *list, MSpan *span);
void	runtime·MSpanList_Remove(MSpan *span);	// from whatever list it is in


// Central list of free objects of a given size.
struct MCentral
{
	Lock;
	int32 sizeclass;
	MSpan nonempty;
	MSpan empty;
	int32 nfree;
};

void	runtime·MCentral_Init(MCentral *c, int32 sizeclass);
int32	runtime·MCentral_AllocList(MCentral *c, MLink **first);
void	runtime·MCentral_FreeList(MCentral *c, MLink *first);
void	runtime·MCentral_FreeSpan(MCentral *c, MSpan *s, int32 n, MLink *start, MLink *end);

// heap 本身只存储 free[] 数组和 large, 应该就是arena区域.
// 但是其他全局数据也存储在这里, 应该是挂的指针, 比如spans, bitmap
//
//                        arena_used(初始arena的使用为0, 所以其地址与 arena_start 在同一位置)
// span     bitmap        arena_start                 arena_end
//   ↓        ↓                ↓                          ↓
//   +--------|----------------+--------------------------+
//   |  span  |     bitmap     |           arena          |
//   +--------|----------------+--------------------------+
//
// Main malloc heap.
// The heap itself is the "free[]" and "large" arrays,
// but all the other global data is here too.
struct MHeap
{
	Lock;

	// free数组的每个成员都是 span 链表.
	// 各成员中, h->free[n]中拥有n个页, 最后一个成员成员可容纳的页数即是MaxMHeapList.
	//
	// free lists of given length
	MSpan free[MaxMHeapList];
	MSpan large;			// free lists length >= MaxMHeapList
	MSpan **allspans;		// MSpan指针类型, 存储所有span对象指针
	uint32	nspan;			// 这是用来表示堆中span中的总个数?
	// allspans 成员列表能存储的 MSpan 的指针的个数, 可以说是其容量.
	// 在为 allspans 分配空间时会同时为此成员赋值.
	uint32	nspancap;

	// 该值为内存分布图中的 span 区域的起始(虚拟)地址.
	//
	// 在 src/pkg/runtime/malloc.goc -> runtime·mallocinit() 中被赋值.
	//
	// span lookup
	MSpan**	spans;
	uintptr	spans_mapped;

	// 该值为内存分布图中的 bitmap 区域的起始(虚拟)地址.
	//
	// 在 src/pkg/runtime/malloc.goc -> runtime·mallocinit() 中被赋值.
	//
	// range of addresses we might see in the heap
	byte *bitmap;
	uintptr bitmap_mapped;
	byte *arena_start;
	byte *arena_used;
	byte *arena_end;

	// central free lists for small size classes.
	// the padding makes sure that the MCentrals are spaced CacheLineSize bytes apart, 
	// so that each MCentral.Lock gets its own cache line.
	struct {
		MCentral;
		byte pad[CacheLineSize];
	} central[NumSizeClasses];

	FixAlloc spanalloc;	// allocator for Span*
	FixAlloc cachealloc;	// allocator for MCache*

	// Malloc stats.
	uint64 largefree;	// bytes freed for large objects (>MaxSmallSize)
	uint64 nlargefree;	// number of frees for large objects (>MaxSmallSize)
	uint64 nsmallfree[NumSizeClasses];	// number of frees for small objects (<=MaxSmallSize)
};
// C中的extern, 应该类似于go中的全局变量
extern MHeap runtime·mheap;

void	runtime·MHeap_Init(MHeap *h);
MSpan*	runtime·MHeap_Alloc(MHeap *h, uintptr npage, int32 sizeclass, int32 acct, int32 zeroed);
void	runtime·MHeap_Free(MHeap *h, MSpan *s, int32 acct);
MSpan*	runtime·MHeap_Lookup(MHeap *h, void *v);
MSpan*	runtime·MHeap_LookupMaybe(MHeap *h, void *v);
void	runtime·MGetSizeClassInfo(int32 sizeclass, uintptr *size, int32 *npages, int32 *nobj);
void*	runtime·MHeap_SysAlloc(MHeap *h, uintptr n);
void	runtime·MHeap_MapBits(MHeap *h);
void	runtime·MHeap_MapSpans(MHeap *h);
void	runtime·MHeap_Scavenger(void);

void*	runtime·mallocgc(uintptr size, uintptr typ, uint32 flag);
void*	runtime·persistentalloc(uintptr size, uintptr align, uint64 *stat);
int32	runtime·mlookup(void *v, byte **base, uintptr *size, MSpan **s);
void	runtime·gc(int32 force);
void	runtime·markallocated(void *v, uintptr n, bool noptr);
void	runtime·checkallocated(void *v, uintptr n);
void	runtime·markfreed(void *v, uintptr n);
void	runtime·checkfreed(void *v, uintptr n);
extern	int32	runtime·checking;
void	runtime·markspan(void *v, uintptr size, uintptr n, bool leftover);
void	runtime·unmarkspan(void *v, uintptr size);
bool	runtime·blockspecial(void*);
void	runtime·setblockspecial(void*, bool);
void	runtime·purgecachedstats(MCache*);
void*	runtime·cnew(Type*);
void*	runtime·cnewarray(Type*, intgo);

void	runtime·settype_flush(M*);
void	runtime·settype_sysfree(MSpan*);
uintptr	runtime·gettype(void*);

enum
{
	// flags to malloc
	// GC doesn't have to scan object
	// GC不需要扫描目标对象(object指分配了空间的对象, Flag标记了object的状态)
	FlagNoScan	= 1<<0,
	FlagNoProfiling	= 1<<1,	// must not profile
	// must not free or scan for pointers
	// 禁止释放或是扫描目标指针
	FlagNoGC	= 1<<2,	
	FlagNoZero	= 1<<3, // don't zero memory
	// 不引入GC...即此次分配的空间不需要通过GC回收吧, 而是需要通过 runtime·free() 手动释放.
	//
	// don't invoke GC
	FlagNoInvokeGC	= 1<<4, 
};

void	runtime·MProf_Malloc(void*, uintptr);
void	runtime·MProf_Free(void*, uintptr);
void	runtime·MProf_GC(void);
int32	runtime·gcprocs(void);
void	runtime·helpgc(int32 nproc);
void	runtime·gchelper(void);

void	runtime·walkfintab(void (*fn)(void*));

enum
{
	TypeInfo_SingleObject = 0,
	TypeInfo_Array = 1,
	TypeInfo_Chan = 2,

	// Enables type information at the end of blocks allocated from heap
	// 这个值为1时, 在为对象分配空间时, 尾部会多出一个指针大小的空间, 用于存储type信息
	// ...目前还没看到type信息的类型列表.
	DebugTypeAtBlockEnd = 0,
};

// defined in mgc0.go
void	runtime·gc_m_ptr(Eface*);
void	runtime·gc_itab_ptr(Eface*);

void	runtime·memorydump(void);
