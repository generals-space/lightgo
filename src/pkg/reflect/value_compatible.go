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

////////////////////////////////////////////////////////////////////////////////
// 	@compatible: 在 v1.5 版本中, 函数原型发生了变动, 成为了
// func makemap(t *maptype, hint int64, h *hmap, bucket unsafe.Pointer) *hmap {}
// 见 [go v1.5 hashmap.go](https://github.com/golang/go/tree/go1.5/src/runtime/hashmap.go#187L)
//
// 为了保证变动最小, 因此扩展出了一个方法 makemap_v1_9(), makemap() 仍然保留.
//
// 	@implementBy: src/pkg/runtime/hashmap.c -> reflect·makemap_v1_9()
func makemap_v1_9(t *rtype, cap int) (m iword)

// 	@compatible: 此方法在 v1.9 版本添加
//
// MakeMapWithSize creates a new map with the specified type
// and initial space for approximately n elements.
func MakeMapWithSize(typ Type, n int) Value {
	if typ.Kind() != Map {
		panic("reflect.MakeMapWithSize of non-map type")
	}
	m := makemap_v1_9(typ.(*rtype), n)
	// return Value{typ.common(), m, flag(Map)}
	return Value{typ.common(), unsafe.Pointer(m), flag(Map) << flagKindShift}
}
