#include "runtime.h"
#include "arch_amd64.h"

#include "malloc.h"

#include "mgc0.h"
#include "type.h"
#include "mgc0__others.h"

// caller:
// 	1. sweepspan() 只有这一处
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
void sweepspan(ParFor *desc, uint32 idx)
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
