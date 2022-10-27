// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
	Package unsafe contains operations that 
	step around the type safety of Go programs.
	unsafe包包含了绕过go程序类型安全机制的操作.
	step around: 绕过

	但是这个文件好像什么也没定义啊...
	另外, 调用unsafe包的, 没有.c文件, 所有的调用者都是.go文件.
*/
package unsafe

// ArbitraryType is here for the purposes of documentation only and is not actually
// part of the unsafe package. 
// It represents the type of an arbitrary Go expression.
type ArbitraryType int

// Pointer 常用的 unsafe.Pointer() 方法, 其实是一个类型转换, 而非函数调用.
//
// Pointer represents a pointer to an arbitrary type. 
// There are four special operations
// available for type Pointer that are not available for other types.
//	1) A pointer value of any type can be converted to a Pointer.
//	2) A Pointer can be converted to a pointer value of any type.
//	3) A uintptr can be converted to a Pointer.
//	4) A Pointer can be converted to a uintptr.
// Pointer therefore allows a program to defeat the type system and read and write
// arbitrary memory. It should be used with extreme care.
type Pointer *ArbitraryType

// Sizeof 没找到具体实现代码. 结果与 reflect 中 rtype.Size() 方法应该是相同的.
//
// Sizeof returns the size in bytes occupied by the value v. 
// The size is that of the "top level" of the value only. 
// For instance, if v is a slice, it returns the size of the slice descriptor, 
// not the size of the memory referenced by the slice.
func Sizeof(v ArbitraryType) uintptr

// Offsetof returns the offset within the struct of the field represented by v,
// which must be of the form structValue.field. 
// In other words, it returns the number of bytes 
// between the start of the struct and the start of the field.
func Offsetof(v ArbitraryType) uintptr

// Alignof returns the alignment of the value v.  It is the maximum value m such
// that the address of a variable with the type of v will always be zero mod m.
// If v is of the form structValue.field,
// it returns the alignment of field f within struct object obj.
func Alignof(v ArbitraryType) uintptr
