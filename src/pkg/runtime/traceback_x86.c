// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build amd64 386

#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "funcdata.h"

void runtime·sigpanic(void);

// This code is also used for the 386 tracebacks.
// Use uintptr for an appropriate word-sized integer.
// 本文件中的代码也可用于 386 架构的回溯追踪.
// 将 unitprt 类型用于合适的 字长 整数

// Generic traceback. 
// Handles runtime stack prints (pcbuf == nil),
// the runtime.Callers function (pcbuf != nil), 
// as well as the garbage collector (callback != nil). 
// A little clunky to merge these, but avoids
// duplicating the code and all its subtlety.
// 通用回溯
// 用于处理runtime的栈关系打印(需要 pcbuf == nil)
// 或是用于实现 runtime.Callers() 函数(需要 pcbuf != nil),
// 也可以用于gc回收(需要callback != nil)
// 虽然将这此合并起来比较麻烦, 但也避免了重复代码, 也避免了一些细微的, 易出错的地方.
//
// caller:
// 	1. src/pkg/runtime/mgc0.c -> addstackroots()
// 	2. src/pkg/runtime/proc.c -> runtime·sigprof()
// 	3. runtime·traceback()
// 	4. runtime·callers()
int32
runtime·gentraceback(
	uintptr pc0, uintptr sp0, uintptr lr0, G *gp, int32 skip, uintptr *pcbuf, 
	int32 max, void (*callback)(Stkframe*, void*), void *v, bool printall
)
{
	int32 i, n, nprint, line;
	uintptr tracepc;
	bool waspanic, printing;
	Func *f, *flr;
	Stkframe frame;
	Stktop *stk;
	String file;
	// 参数 lr0 几乎没什么作用啊...
	USED(lr0);

	nprint = 0;
	runtime·memclr((byte*)&frame, sizeof frame);
	frame.pc = pc0;
	frame.sp = sp0;
	waspanic = false;
	// 是否是用于栈调用关系打印(用于打印时需要pcbuf=nil, callback=nil)
	printing = pcbuf==nil && callback==nil;
	
	// If the PC is zero, it's likely a nil function call.
	// Start in the caller's frame.
	// 这里是参数 pc0
	// 如果 pc0 为0, 
	if(frame.pc == 0) {
		frame.pc = *(uintptr*)frame.sp;
		frame.sp += sizeof(uintptr);
	}

	f = runtime·findfunc(frame.pc);
	if(f == nil) {
		if(callback != nil) {
			runtime·printf("runtime: unknown pc %p\n", frame.pc);
			runtime·throw("unknown pc");
		}
		return 0;
	}
	// 到这是要构建一个 frame 对象?
	frame.fn = f;

	n = 0;
	// gp是当前协程对象, stackbase 指向栈空间顶部top部分的起始地址
	stk = (Stktop*)gp->stackbase;
	// 从当前 frame 向上追溯, max 为最大层数.
	while(n < max) {
		// Typically:
		//	pc is the PC of the running function.
		//	sp is the stack pointer at that program counter.
		//	fp is the frame pointer (caller's stack pointer) at that program counter, or nil if unknown.
		//	stk is the stack containing sp.
		//	The caller's program counter is lr, unless lr is zero, 
		// in which case it is *(uintptr*)sp.
		// 典型情况:
		// pc: 当前正在执行的函数的pc计数器(函数内部某条指令的地址)
		// sp: pc所在函数的sp地址
		// fp: pc所在函数的fp地址, 指向其主调函数的sp
		// stk: 包含sp的栈
		// 主调函数的pc为lr, 除非lr = 0(这种情况下主调函数的pc将为 *(uintptr*)sp)
		if(frame.pc == (uintptr)runtime·lessstack) {
			// Hit top of stack segment. Unwind to next segment.
			frame.pc = stk->gobuf.pc;
			frame.sp = stk->gobuf.sp;
			frame.lr = 0;
			frame.fp = 0;
			frame.fn = nil;
			if(printing && runtime·showframe(nil, gp))
				runtime·printf("----- stack segment boundary -----\n");
			stk = (Stktop*)stk->stackbase;

			f = runtime·findfunc(frame.pc);
			if(f == nil) {
				runtime·printf("runtime: unknown pc %p after stack split\n", frame.pc);
				if(callback != nil) runtime·throw("unknown pc");
			}
			frame.fn = f;
			continue;
		}
		f = frame.fn;

		// Found an actual function.
		// Derive frame pointer and link register.
		if(frame.fp == 0) {
			frame.fp = frame.sp + runtime·funcspdelta(f, frame.pc);
			frame.fp += sizeof(uintptr); // caller PC
		}
		// f是否已经是处于栈顶的函数(再没有更上层的主调函数了)
		// 比如 runtime·goexit()
		if(runtime·topofstack(f)) {
			frame.lr = 0;
			flr = nil;
		} else {
			if(frame.lr == 0) frame.lr = ((uintptr*)frame.fp)[-1];

			flr = runtime·findfunc(frame.lr);
			if(flr == nil) {
				runtime·printf("runtime: unexpected return pc for %s called from %p\n", 
					runtime·funcname(f), frame.lr);
				if(callback != nil) runtime·throw("unknown caller pc");
			}
		}
		
		frame.varp = (byte*)frame.fp - sizeof(uintptr);

		// Derive size of arguments.
		// Most functions have a fixed-size argument block,
		// so we can use metadata about the function f.
		// Not all, though: there are some variadic functions
		// in package runtime and reflect, and for those we use call-specific
		// metadata recorded by f's caller.
		if(callback != nil || printing) {
			frame.argp = (byte*)frame.fp;
			if(f->args != ArgsSizeUnknown)
				frame.arglen = f->args;
			else if(flr == nil)
				frame.arglen = 0;
			else if(frame.lr == (uintptr)runtime·lessstack)
				frame.arglen = stk->argsize;
			else if((i = runtime·funcarglen(flr, frame.lr)) >= 0)
				frame.arglen = i;
			else {
				runtime·printf("runtime: unknown argument frame size for %s called from %p [%s]\n",
					runtime·funcname(f), frame.lr, flr ? runtime·funcname(flr) : "?");
				if(callback != nil) runtime·throw("invalid stack");
				frame.arglen = 0;
			}
		}

		if(skip > 0) {
			skip--;
			goto skipped;
		}
		// 用于栈信息追溯而打印时, skip一般为0, 
		// 否则直接在上面就 goto 到 skipped 处了.
		if(pcbuf != nil) pcbuf[n] = frame.pc;

		// callback 在gc时会用到
		if(callback != nil) callback(&frame, v);

		if(printing) {
			if(printall || runtime·showframe(f, gp)) {
				// Print during crash.
				//	main(0x1, 0x2, 0x3)
				//		/home/rsc/go/src/runtime/x.go:23 +0xf
				//		
				tracepc = frame.pc;	// back up to CALL instruction for funcline.
				if(n > 0 && frame.pc > f->entry && !waspanic)
					tracepc--;
				runtime·printf("%s(", runtime·funcname(f));
				for(i = 0; i < frame.arglen/sizeof(uintptr); i++) {
					if(i >= 5) {
						runtime·prints(", ...");
						break;
					}
					if(i != 0) runtime·prints(", ");
					runtime·printhex(((uintptr*)frame.argp)[i]);
				}
				runtime·prints(")\n");
				line = runtime·funcline(f, tracepc, &file);
				runtime·printf("\t%S:%d", file, line);
				if(frame.pc > f->entry)
					runtime·printf(" +%p", (uintptr)(frame.pc - f->entry));

				if(m->throwing > 0 && gp == m->curg)
					runtime·printf(" fp=%p", frame.fp);
				runtime·printf("\n");
				nprint++;
			}
		}
		n++;
	
	skipped:
		waspanic = f->entry == (uintptr)runtime·sigpanic;

		// Do not unwind past the bottom of the stack.
		if(flr == nil) break;

		// Unwind to next frame.
		frame.fn = flr;
		frame.pc = frame.lr;
		frame.lr = 0;
		frame.sp = frame.fp;
		frame.fp = 0;
	}
	// ...这不就是 printing 么? 为什么还要重新写一遍表达式?
	if(pcbuf == nil && callback == nil) n = nprint;

	return n;
}

// 打印指定协程对象gp的父级函数的调用栈信息???
// 包括函数名称, 所在文件及行号, 还有函数地址.
void
runtime·printcreatedby(G *gp)
{
	int32 line;
	uintptr pc, tracepc;
	Func *f;
	String file;

	// Show what created goroutine, except main goroutine (goid 1).
	// gp->gopc 为当前协程对象gp的主调函数的地址
	// 注意如下的if语句中其实也是一个执行流程, pc-> f -> gp -> goid 原本都是未知的.
	if((pc = gp->gopc) != 0 && (f = runtime·findfunc(pc)) != nil &&
		runtime·showframe(f, gp) && gp->goid != 1) {

		runtime·printf("created by %s\n", runtime·funcname(f));
		// back up to CALL instruction for funcline.
		tracepc = pc;
		// 86/64 平台上, PCQuantum 值为1
		// 函数对象的 entry 成员本来就是 pc 值, 什么时候会出现这种情况???
		if(pc > f->entry) tracepc -= PCQuantum;

		line = runtime·funcline(f, tracepc, &file);
		runtime·printf("\t%S:%d", file, line);

		if(pc > f->entry) runtime·printf(" +%p", (uintptr)(pc - f->entry));
		
		runtime·printf("\n");
	}
}

void
runtime·traceback(uintptr pc, uintptr sp, uintptr lr, G *gp)
{
	USED(lr);

	if(gp->status == Gsyscall) {
		// Override signal registers if blocked in system call.
		pc = gp->syscallpc;
		sp = gp->syscallsp;
	}
	
	// Print traceback. By default, omits runtime frames.
	// If that means we print nothing at all, 
	// repeat forcing all frames printed.
	//
	// 打印栈调用信息, 默认不打印runtime的函数帧
	// 如果打印的数量(即返回值为0), 那么把最后一个参数改成true再来一遍.
	// skip 在参数 gp 之后, 这里值为0.
	// 最后一个参数为 printall.
	if(runtime·gentraceback(pc, sp, 0, gp, 0, nil, 100, nil, nil, false) == 0) {
		runtime·gentraceback(pc, sp, 0, gp, 0, nil, 100, nil, nil, true);
	}
	// 打印gp协程当前正在运行的g的主调函数的栈信息, 包括函数名称, 声明时所在的文件, 行号和pc地址等.
	runtime·printcreatedby(gp);
}

// 返回在主调函数所在的调用链上, 函数的个数...???
// pcbuf应该是一个uintptr[2]数组
//
// caller: 
// 	1. runtime.c -> runtime·Caller()
// 	2. src/pkg/runtime/proc.c -> mcommoninit()
//
int32
runtime·callers(int32 skip, uintptr *pcbuf, int32 m)
{
	uintptr pc, sp;
	// 获取主调函数, 即runtime·callers()的sp, pc地址
	sp = runtime·getcallersp(&skip);
	pc = (uintptr)runtime·getcallerpc(&skip);

	return runtime·gentraceback(pc, sp, 0, g, skip, pcbuf, m, nil, nil, false);
}
