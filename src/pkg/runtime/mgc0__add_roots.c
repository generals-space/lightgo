#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "stack.h"

#include "mgc0.h"
#include "mgc0__stats.h"
#include "type.h"

#include "typekind.h"
#include "funcdata.h"

#include "mgc0__others.h"

extern byte data[];
extern byte edata[];
extern byte bss[];
extern byte ebss[];

extern byte gcdata[];
extern byte gcbss[];

extern struct WORK work;

extern FinBlock *allfin;

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
static void scanbitvector(byte *scanp, BitVector *bv, bool afterprologue)
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
static void addframeroots(Stkframe *frame, void*)
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

// Program that scans the whole block and treats every block element
// as a potential pointer
extern uintptr defaultProg[2];

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
void addroots(void)
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
				// 不执行操作
			case MTypes_Single:
				// 这里 break 的是当前的 switch 语句
				break; 
			case MTypes_Words:
				// 不执行操作
			case MTypes_Bytes:
				markonly((byte*)s->types.data);
				// 这里 break 的是当前的 switch 语句
				break; 
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
