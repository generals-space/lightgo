#include "runtime.h"
#include "arch_amd64.h"

#include "malloc.h"

#include "mgc0.h"
#include "mgc0__stats.h"
#include "type.h"
#include "mgc0__others.h"

extern struct GCSTATS gcstats;
extern struct WORK work;

// Get an empty work buffer off the work.empty list,
// allocating new buffers as needed.
Workbuf* getempty(Workbuf *b)
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

void putempty(Workbuf *b)
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
Workbuf* getfull(Workbuf *b)
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
// 	1. work_enqueue()
Workbuf* handoff(Workbuf *b)
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
