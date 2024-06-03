// Copyright 2012 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Garbage collector (GC)

// GC instruction opcodes.
//
// The opcode(操作码) of an instruction is followed by zero or more
// arguments to the instruction.
//
// Meaning of arguments:
//   off      Offset (in bytes) from the start of the current object
//   objgc    Pointer to GC info of an object
//   objgcrel Offset to GC info of an object
//   len      Length of an array
//   elemsize Size (in bytes) of an element
//   size     Size (in bytes)
//
// NOTE: There is a copy of these in ../reflect/type.go.
// They must be kept in sync.
//
//
// 如下枚举值是gc行为的操作码, 在 src/pkg/runtime/mgc0.c -> scanblock() 中进行了处理.
// 每种操作码接收的参数数量是不同的, 具体的操作码及参数对应关系, 可见
// src/pkg/reflect/type.go -> appendGCProgram()
// 
// 比如 GC_PTR, 在 appendGCProgram() 中, `_GC_PTR`指令的 argcnt = 2(两个参数),
// 那么 scanblock() 的 case 中, pc[0] 为 GC_PTR 自身, pc[1]和pc[2]就是这两个参数,
// 之后指针后移3位, 进行下一个指令的解析.
enum {
	GC_END,         // End of object, loop or subroutine. Args: none
	GC_PTR,         // A typed pointer. Args: (off, objgc)
	GC_APTR,        // Pointer to an arbitrary object. Args: (off)
	GC_ARRAY_START, // Start an array with a fixed length. Args: (off, len, elemsize)
	GC_ARRAY_NEXT,  // The next element of an array. Args: none
	GC_CALL,        // Call a subroutine. Args: (off, objgcrel)
	GC_CHAN_PTR,    // Go channel. Args: (off, ChanType*)
	GC_STRING,      // Go string. Args: (off)
	GC_EFACE,       // interface{}. Args: (off)
	GC_IFACE,       // interface{...}. Args: (off)
	GC_SLICE,       // Go slice. Args: (off, objgc)
	GC_REGION,      // A region/part of the current object. Args: (off, size, objgc)

	GC_NUM_INSTR,   // Number of instruction opcodes
};

enum {
	// Size of GC's fixed stack.
	//
	// The current GC implementation permits:
	//  - at most 1 stack allocation because of GC_CALL
	//  - at most GC_STACK_CAPACITY allocations because of GC_ARRAY_START
	GC_STACK_CAPACITY = 8,	
};

enum {
	Debug = 0,
	DebugMark = 0,  // run second pass to check mark
	CollectStats = 0,
	ScanStackByFrames = 0,
	IgnorePreciseGC = 0,

	// wordsPerBitmapWord = 16, 表示 bitmap 中每个 word 对应 arena 中 word 的个数.
	//
	// word(字)是一种逻辑单位, 与指针(pointer)同级, 占用空间 4 bits.
	// bitmap 区域中只要一个 word(4 bits)就可以描述一个 arena 中的指针(8 bits * 8).
	// bitmap:arena == 1:16
	//
	// Four bits per word (see #defines below).
	wordsPerBitmapWord = sizeof(void*)*8/4,
	// bitShift = 16
	bitShift = sizeof(void*)*8/4,

	handoffThreshold = 4,
	IntermediateBufferCapacity = 64,

	// Bits in type information
	PRECISE = 1,
	LOOP = 2,
	PC_BITS = PRECISE | LOOP,

	// Pointer map
	BitsPerPointer = 2,
	// 每个指针的信息用 BitsPerPointer 个 bit 即可表示, 可有如下4种情况.
	// 在 scanbitvector() 函数中有对相关变量与 3(0000 0000 0000 0011)做与操作,
	// 得到结果后与如下4种情况做比较的操作. 
	BitsNoPointer = 0, 	// 非指针对象
	BitsPointer = 1,	// 指针对象
	BitsIface = 2,		// 接口类型对象
	BitsEface = 3,		// 接口数据对象
};
