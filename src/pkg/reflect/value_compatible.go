package reflect

import (
	"unsafe"
)

// 	@compatible: 该函数在 v1.5 版本初次出现.
//
// arrayAt returns the i-th element of p, a C-array whose elements are
// eltSize wide (in bytes).
func arrayAt(p unsafe.Pointer, i int, eltSize uintptr) unsafe.Pointer {
	return unsafe.Pointer(uintptr(p) + uintptr(i)*eltSize)
}

// 	@compatible: 该函数在 v1.5 版本初次出现.
//
// [golang v1.5 mbarrier](https://github.com/golang/go/tree/go1.5/src/runtime/mbarrier.go#L180)
//
func typedmemmove(typ *rtype, dst, src unsafe.Pointer) {
	// 	@todo
	// 原版的 typedmemmove() 与写屏障有关, 这里直接先用 memmove() 代替了.
	memmove(dst, src, typ.size)
}
