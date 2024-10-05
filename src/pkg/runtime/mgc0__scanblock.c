#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"

#include "mgc0.h"

#include "mgc0__stats.h"
#include "type.h"
#include "typekind.h"
#include "mgc0__others.h"

extern struct GCSTATS gcstats;
extern struct WORK work;
extern Workbuf* getempty(Workbuf *b);
extern void putempty(Workbuf *b);
extern Workbuf* getfull(Workbuf *b);
extern Workbuf* handoff(Workbuf *b);

extern void work_enqueue(Obj obj, Workbuf **_wbuf, Obj **_wp, uintptr *_nobj);

// caller:
// 	1. scanblock() 只有这一个函数, 但是有多次调用.
//
// flushptrbuf moves data from the PtrTarget buffer to the work buffer.
// The PtrTarget buffer contains blocks irrespective of(不管) whether the blocks
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
//  (find block start, mark and work_enqueue)
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

// Program that scans the whole block and treats every block element
// as a potential pointer
uintptr defaultProg[2] = {PtrSize, GC_DEFAULT_PTR};

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
// scanblock scans a block of n bytes starting at pointer b for references to
// other objects, 
// scanning any it finds recursively until there are no unscanned objects left. 
// Instead of using an explicit recursion, it keeps a work list in the Workbuf* 
// structures and loops in the main function body. 
// Keeping an explicit work list is easier on the stack allocator and
// more efficient.
// 
void scanblock(Workbuf *wbuf, Obj *wp, uintptr nobj, bool keepworking)
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
							work_enqueue((Obj){b, n, 0}, &wbuf, &wp, &nobj);
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
		{
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
		}

	} // 这里是最外层 for(;;) {} 循环的结束位置.

endscan:;
}

// debug_scanblock debug版本的 scanblock
// 更简单, 单线程, 递归操作, 使用 bitSpecial 作为标记位.
// 不过速度上好像更慢一点.
//
// debug_scanblock is the debug copy of scanblock.
// it is simpler, slower, single-threaded, recursive,
// and uses bitSpecial as the mark bit.
void debug_scanblock(byte *b, uintptr n)
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
