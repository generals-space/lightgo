// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "../../cmd/ld/textflag.h"

// golang原生: main() 函数入口
// 
TEXT _rt0_amd64_linux(SB),NOSPLIT,$-8
	LEAQ	8(SP), SI // argv
	MOVQ	0(SP), DI // argc
	MOVQ	$main(SB), AX // $main() 即下方的 main() 函数
	JMP	AX

TEXT main(SB),NOSPLIT,$-8
	// _rt0_go() 位于 src/pkg/runtime/asm_amd64.s 文件
	MOVQ	$_rt0_go(SB), AX
	JMP	AX
