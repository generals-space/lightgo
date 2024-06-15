#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "stack.h"
#include "mgc0.h"
#include "mgc0__stats.h"
#include "mgc0__funcs.h"

// Initialized from $GOGC. GOGC=off means no gc.
// 从环境变量GOGC进行初始化. GOGC=off表示不进行gc
// 
// Next gc is after we've allocated an extra amount of
// memory proportional to the amount already in use.
// If gcpercent=100 and we're using 4M, we'll gc again when we get to 8M. 
// This keeps the gc cost in linear proportion to the allocation cost. 
// Adjusting gcpercent just changes the linear constant 
// (and also the amount of extra memory used).
// 
// gcpercent表示, 下一次gc时, 需要等到额外分配的内存大小, 占当前已分配空间的比例.
// 比如, 如果 gcpercent == 100, 此次gc时内存已经分配了 4M, 
// 那么下次gc触发条件是内存分配达到 8M, 即额外增长 100% 的时候, 再触发.
// 这样可以保证gc的开销与内存分配开销成线性比例.
// 调整此参数只会影响线性常数(当然也包括新增内存的空间大小)
// 
int32 gcpercent = GcpercentUnknown;

// caller: 
// 	1. updatememstats()
// 	2. gc()
void cachestats(void)
{
	MCache *c;
	P *p, **pp;

	for(pp=runtime·allp; p=*pp; pp++) {
		c = p->mcache;
		if(c==nil) {
			continue;
		}
		runtime·purgecachedstats(c);
	}
}

void updatememstats(GCStats *stats)
{
	M *mp;
	MSpan *s;
	MCache *c;
	P *p, **pp;
	int32 i;
	uint64 stacks_inuse, smallfree;
	uint64 *src, *dst;

	if(stats) {
		runtime·memclr((byte*)stats, sizeof(*stats));
	}
	stacks_inuse = 0;
	for(mp=runtime·allm; mp; mp=mp->alllink) {
		stacks_inuse += mp->stackinuse*FixedStack;
		if(stats) {
			src = (uint64*)&mp->gcstats;
			dst = (uint64*)stats;
			for(i=0; i<sizeof(*stats)/sizeof(uint64); i++) {
				dst[i] += src[i];
			}
			runtime·memclr((byte*)&mp->gcstats, sizeof(mp->gcstats));
		}
	}
	mstats.stacks_inuse = stacks_inuse;
	mstats.mcache_inuse = runtime·mheap.cachealloc.inuse;
	mstats.mspan_inuse = runtime·mheap.spanalloc.inuse;
	mstats.sys = mstats.heap_sys + mstats.stacks_sys + mstats.mspan_sys +
		mstats.mcache_sys + mstats.buckhash_sys + mstats.gc_sys + mstats.other_sys;
	
	// Calculate memory allocator stats.
	// During program execution we only count number of frees and amount of freed memory.
	// Current number of alive object in the heap and amount of alive heap memory
	// are calculated by scanning all spans.
	// Total number of mallocs is calculated as number of frees plus number of alive objects.
	// Similarly, total amount of allocated memory is calculated as amount of freed memory
	// plus amount of alive heap memory.
	mstats.alloc = 0;
	mstats.total_alloc = 0;
	mstats.nmalloc = 0;
	mstats.nfree = 0;
	for(i = 0; i < nelem(mstats.by_size); i++) {
		mstats.by_size[i].nmalloc = 0;
		mstats.by_size[i].nfree = 0;
	}

	// Flush MCache's to MCentral.
	for(pp=runtime·allp; p=*pp; pp++) {
		c = p->mcache;
		if(c==nil) {
			continue;
		}
		runtime·MCache_ReleaseAll(c);
	}

	// Aggregate local stats.
	cachestats();

	// Scan all spans and count number of alive objects.
	for(i = 0; i < runtime·mheap.nspan; i++) {
		s = runtime·mheap.allspans[i];
		if(s->state != MSpanInUse) {
			continue;
		}
		if(s->sizeclass == 0) {
			mstats.nmalloc++;
			mstats.alloc += s->elemsize;
		} else {
			mstats.nmalloc += s->ref;
			mstats.by_size[s->sizeclass].nmalloc += s->ref;
			mstats.alloc += s->ref*s->elemsize;
		}
	}

	// Aggregate by size class.
	smallfree = 0;
	mstats.nfree = runtime·mheap.nlargefree;
	for(i = 0; i < nelem(mstats.by_size); i++) {
		mstats.nfree += runtime·mheap.nsmallfree[i];
		mstats.by_size[i].nfree = runtime·mheap.nsmallfree[i];
		mstats.by_size[i].nmalloc += runtime·mheap.nsmallfree[i];
		smallfree += runtime·mheap.nsmallfree[i] * runtime·class_to_size[i];
	}
	mstats.nmalloc += mstats.nfree;

	// Calculate derived stats.
	mstats.total_alloc = mstats.alloc + runtime·mheap.largefree + smallfree;
	mstats.heap_alloc = mstats.alloc;
	mstats.heap_objects = mstats.nmalloc - mstats.nfree;
}

// 	@implementOf: src/pkg/runtime/mem.go -> ReadMemStats()
void runtime·ReadMemStats(MStats *stats)
{
	// Have to acquire worldsema to stop the world,
	// because stoptheworld can only be used by one goroutine at a time,
	// and there might be a pending garbage collection already calling it.
	runtime·semacquire(&runtime·worldsema, false);
	m->gcing = 1;
	runtime·stoptheworld();
	updatememstats(nil);
	*stats = mstats;
	m->gcing = 0;
	m->locks++;
	runtime·semrelease(&runtime·worldsema);
	runtime·starttheworld();
	m->locks--;
}

// 	@implementOf: src/pkg/runtime/debug/garbage.go -> readGCStats()
void runtime∕debug·readGCStats(Slice *pauses)
{
	uint64 *p;
	uint32 i, n;

	// Calling code in runtime/debug should make the slice large enough.
	if(pauses->cap < nelem(mstats.pause_ns)+3) {
		runtime·throw("runtime: short slice passed to readGCStats");
	}

	// Pass back: pauses, last gc (absolute time), number of gc, total pause ns.
	p = (uint64*)pauses->array;
	runtime·lock(&runtime·mheap);
	n = mstats.numgc;
	if(n > nelem(mstats.pause_ns)) {
		n = nelem(mstats.pause_ns);
	}
	
	// The pause buffer is circular.
	// The most recent pause is at pause_ns[(numgc-1)%nelem(pause_ns)],
	// and then backward from there to go back farther in time.
	// We deliver the times most recent first (in p[0]).
	for(i=0; i<n; i++) {
		p[i] = mstats.pause_ns[(mstats.numgc-1-i)%nelem(mstats.pause_ns)];
	}

	p[n] = mstats.last_gc;
	p[n+1] = mstats.numgc;
	p[n+2] = mstats.pause_total_ns;	
	runtime·unlock(&runtime·mheap);
	pauses->len = n+3;
}

// 	@implementOf: src/pkg/runtime/debug/garbage.go -> setGCPercent()
void runtime∕debug·setGCPercent(intgo in, intgo out)
{
	runtime·lock(&runtime·mheap);

	if(gcpercent == GcpercentUnknown) {
		gcpercent = readgogc();
	} 

	out = gcpercent;

	if(in < 0) {
		in = -1;
	} 

	gcpercent = in;
	runtime·unlock(&runtime·mheap);
	FLUSH(&out);
}

// caller:
// 	1. src/pkg/runtime/malloc.goc -> runtime·MHeap_SysAlloc() 只有这一处
void runtime·MHeap_MapBits(MHeap *h)
{
	// Caller has added extra mappings to the arena.
	// Add extra mappings of bitmap words as needed.
	// We allocate extra bitmap pieces in chunks of bitmapChunk.
	enum {
		bitmapChunk = 8192
	};
	uintptr n;

	n = (h->arena_used - h->arena_start) / wordsPerBitmapWord;
	n = ROUND(n, bitmapChunk);
	if(h->bitmap_mapped >= n) {
		return;
	}

	runtime·SysMap(h->arena_start - n, n - h->bitmap_mapped, &mstats.gc_sys);
	h->bitmap_mapped = n;
}
