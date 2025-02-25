package reflect

import (
	"math"
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

////////////////////////////////////////////////////////////////////////////////
// 	@compatible: 此方法在 v1.13 版本添加
//
// IsZero reports whether v is the zero value for its type.
// It panics if the argument is invalid.
func (v Value) IsZero() bool {
	switch v.kind() {
	case Bool:
		return !v.Bool()
	case Int, Int8, Int16, Int32, Int64:
		return v.Int() == 0
	case Uint, Uint8, Uint16, Uint32, Uint64, Uintptr:
		return v.Uint() == 0
	case Float32, Float64:
		return math.Float64bits(v.Float()) == 0
	case Complex64, Complex128:
		c := v.Complex()
		return math.Float64bits(real(c)) == 0 && math.Float64bits(imag(c)) == 0
	case Array:
		for i := 0; i < v.Len(); i++ {
			if !v.Index(i).IsZero() {
				return false
			}
		}
		return true
	case Chan, Func, Interface, Map, Ptr, Slice, UnsafePointer:
		return v.IsNil()
	case String:
		return v.Len() == 0
	case Struct:
		for i := 0; i < v.NumField(); i++ {
			if !v.Field(i).IsZero() {
				return false
			}
		}
		return true
	default:
		// This should never happens, but will act as a safeguard for
		// later, as a default value doesn't makes sense here.
		panic(&ValueError{"reflect.Value.IsZero", v.Kind()})
	}
}
