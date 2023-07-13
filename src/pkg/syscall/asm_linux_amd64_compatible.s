// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "../../cmd/ld/textflag.h"

// 	@compatible: 该方法在 v1.3 版本初次添加, 其实与 ·Gettimeofday() 是同一个
// 	@implementOf: src/pkg/syscall/syscall_linux_amd64_compatible.go -> gettimeofday()
TEXT ·gettimeofday(SB),NOSPLIT,$0-16
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
