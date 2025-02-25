// Copyright 2016 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package reflect

import "unsafe"

// Swapper 接受 slice 参数, 并返回该 slice 类型对应的 swap 函数.
// 这个函数可以接收两个参数 i, j, 并交换ta们在切片中的位置, 用于切片类型的通用排序.
//
// 	@compatible: 该函数在 v1.8 版本初次出现.
//
// 	@param slice: 必须是切片类型
//
// caller:
// 	1. src/pkg/sort/sort_compatible.go -> Slice()
// 	2. src/pkg/sort/sort_compatible.go -> SliceStable()
//
// Swapper returns a function that swaps the elements in the provided slice.
//
// Swapper panics if the provided interface is not a slice.
func Swapper(slice interface{}) func(i, j int) {
	v := ValueOf(slice)
	if v.Kind() != Slice {
		panic(&ValueError{Method: "Swapper", Kind: v.Kind()})
	}
	// 如果目标 slice 切片只有 1 个甚至 0 个元素, 根本不需要交换, 直接返回一个糊弄的函数.
	// Fast path for slices of size 0 and 1. Nothing to swap.
	switch v.Len() {
	case 0:
		return func(i, j int) { panic("reflect: slice index out of range") }
	case 1:
		return func(i, j int) {
			if i != 0 || j != 0 {
				panic("reflect: slice index out of range")
			}
		}
	}

	typ := v.Type().Elem().(*rtype)
	size := typ.Size()
	hasPtr := typ.kind&kindNoPointers == 0

	// Some common & small cases, without using memmove:
	if hasPtr {
		if size == ptrSize {
			ps := *(*[]unsafe.Pointer)(v.ptr)
			return func(i, j int) { ps[i], ps[j] = ps[j], ps[i] }
		}
		if typ.Kind() == String {
			ss := *(*[]string)(v.ptr)
			return func(i, j int) { ss[i], ss[j] = ss[j], ss[i] }
		}
	} else {
		switch size {
		case 8:
			is := *(*[]int64)(v.ptr)
			return func(i, j int) { is[i], is[j] = is[j], is[i] }
		case 4:
			is := *(*[]int32)(v.ptr)
			return func(i, j int) { is[i], is[j] = is[j], is[i] }
		case 2:
			is := *(*[]int16)(v.ptr)
			return func(i, j int) { is[i], is[j] = is[j], is[i] }
		case 1:
			is := *(*[]int8)(v.ptr)
			return func(i, j int) { is[i], is[j] = is[j], is[i] }
		}
	}

	s := (*SliceHeader)(v.ptr)
	tmp := unsafe_New(typ) // swap scratch space

	return func(i, j int) {
		if uint(i) >= uint(s.Len) || uint(j) >= uint(s.Len) {
			panic("reflect: slice index out of range")
		}
		// 	@compatible: 该函数在 v1.8 版本初次出现.
		// 	这里做了些修改, arrayAt() 第1个参数需要用 unsafe.Pointer() 转换一下.
		val1 := arrayAt(unsafe.Pointer(s.Data), i, size)
		val2 := arrayAt(unsafe.Pointer(s.Data), j, size)
		typedmemmove(typ, tmp, val1)
		typedmemmove(typ, val1, val2)
		typedmemmove(typ, val2, tmp)
	}
}
