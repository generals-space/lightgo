#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "mgc0.h"
#include "mgc0__funcs.h"

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
