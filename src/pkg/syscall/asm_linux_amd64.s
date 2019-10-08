// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "../../cmd/ld/textflag.h"

//
// System calls for AMD64, Linux
//

// func Syscall(trap int64, a1, a2, a3 int64) (r1, r2, err int64);
// Trap # in AX, args in DI SI DX R10 R8 R9, return in AX DX
// Note that this differs from "standard" ABI convention, which
// would pass 4th arg in CX, not R10.
// 系统调用号由 AX 寄存器传递, 参数列表分别放在 DI SI DX R10 R8 R9 寄存器中,
// 返回值则放在 AX DX 中.
// 注意: 这里与标准API调用约定不太一样, 第4个参数是放在R10而不是CX寄存器中的.
// 
// 参数SB寄存器(Static Base)只是表示当前语句是用来声明函数(或全局变量的)
// 不过这个函数貌似没有被其他地方显式调用, 应该是由编译器来定位的.
TEXT	·Syscall(SB),NOSPLIT,$0-64
	CALL	runtime·entersyscall(SB)
	// 下面的 MOVQ 指令是为参数列表赋值, 从第3个指针地址开始.
	MOVQ	16(SP), DI
	MOVQ	24(SP), SI
	MOVQ	32(SP), DX
	MOVQ	$0, R10
	MOVQ	$0, R8
	MOVQ	$0, R9
	// syscall entry
	// AX 系统调用入口
	MOVQ	8(SP), AX
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	ok
	// 下面的 MOVQ 指令是为返回值赋值
	MOVQ	$-1, 40(SP)	// r1
	MOVQ	$0, 48(SP)	// r2
	NEGQ	AX
	MOVQ	AX, 56(SP)  // errno
	CALL	runtime·exitsyscall(SB)
	RET
ok:
	MOVQ	AX, 40(SP)	// r1
	MOVQ	DX, 48(SP)	// r2
	MOVQ	$0, 56(SP)	// errno
	CALL	runtime·exitsyscall(SB)
	RET

TEXT ·Syscall6(SB),NOSPLIT,$0-88
	CALL	runtime·entersyscall(SB)
	MOVQ	16(SP), DI
	MOVQ	24(SP), SI
	MOVQ	32(SP), DX
	MOVQ	40(SP), R10
	MOVQ	48(SP), R8
	MOVQ	56(SP), R9
	MOVQ	8(SP), AX	// syscall entry
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	ok6
	MOVQ	$-1, 64(SP)	// r1
	MOVQ	$0, 72(SP)	// r2
	NEGQ	AX
	MOVQ	AX, 80(SP)  // errno
	CALL	runtime·exitsyscall(SB)
	RET
ok6:
	MOVQ	AX, 64(SP)	// r1
	MOVQ	DX, 72(SP)	// r2
	MOVQ	$0, 80(SP)	// errno
	CALL	runtime·exitsyscall(SB)
	RET

TEXT ·RawSyscall(SB),NOSPLIT,$0-64
	MOVQ	16(SP), DI
	MOVQ	24(SP), SI
	MOVQ	32(SP), DX
	MOVQ	$0, R10
	MOVQ	$0, R8
	MOVQ	$0, R9
	MOVQ	8(SP), AX	// syscall entry
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	ok1
	MOVQ	$-1, 40(SP)	// r1
	MOVQ	$0, 48(SP)	// r2
	NEGQ	AX
	MOVQ	AX, 56(SP)  // errno
	RET
ok1:
	MOVQ	AX, 40(SP)	// r1
	MOVQ	DX, 48(SP)	// r2
	MOVQ	$0, 56(SP)	// errno
	RET

TEXT ·RawSyscall6(SB),NOSPLIT,$0-88
	MOVQ	16(SP), DI
	MOVQ	24(SP), SI
	MOVQ	32(SP), DX
	MOVQ	40(SP), R10
	MOVQ	48(SP), R8
	MOVQ	56(SP), R9
	MOVQ	8(SP), AX	// syscall entry
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	ok2
	MOVQ	$-1, 64(SP)	// r1
	MOVQ	$0, 72(SP)	// r2
	NEGQ	AX
	MOVQ	AX, 80(SP)  // errno
	RET
ok2:
	MOVQ	AX, 64(SP)	// r1
	MOVQ	DX, 72(SP)	// r2
	MOVQ	$0, 80(SP)	// errno
	RET

TEXT ·Gettimeofday(SB),NOSPLIT,$0-24
	MOVQ	8(SP), DI
	MOVQ	$0, SI
	MOVQ	runtime·__vdso_gettimeofday_sym(SB), AX
	CALL	AX

	CMPQ	AX, $0xfffffffffffff001
	JLS	ok7
	NEGQ	AX
	MOVQ	AX, 16(SP)  // errno
	RET
ok7:
	MOVQ	$0, 16(SP)  // errno
	RET

TEXT ·Time(SB),NOSPLIT,$0-32
	MOVQ	8(SP), DI
	MOVQ	runtime·__vdso_time_sym(SB), AX
	CALL	AX
	MOVQ	AX, 16(SP)  // tt
	MOVQ	$0, 24(SP)  // errno
	RET
