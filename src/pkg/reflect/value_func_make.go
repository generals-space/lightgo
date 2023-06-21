package reflect

import (
	"unsafe"
)

// makeInt returns a Value of type t equal to bits (possibly truncated),
// where t is a signed or unsigned int type.
func makeInt(f flag, bits uint64, t Type) Value {
	typ := t.common()
	if typ.size > ptrSize {
		// Assume ptrSize >= 4, so this must be uint64.
		ptr := unsafe_New(typ)
		*(*uint64)(unsafe.Pointer(ptr)) = bits
		return Value{typ, ptr, f | flagIndir | flag(typ.Kind())<<flagKindShift}
	}
	var w iword
	switch typ.size {
	case 1:
		*(*uint8)(unsafe.Pointer(&w)) = uint8(bits)
	case 2:
		*(*uint16)(unsafe.Pointer(&w)) = uint16(bits)
	case 4:
		*(*uint32)(unsafe.Pointer(&w)) = uint32(bits)
	case 8:
		*(*uint64)(unsafe.Pointer(&w)) = uint64(bits)
	}
	return Value{typ, unsafe.Pointer(w), f | flag(typ.Kind())<<flagKindShift}
}

// makeFloat returns a Value of type t equal to v (possibly truncated to float32),
// where t is a float32 or float64 type.
func makeFloat(f flag, v float64, t Type) Value {
	typ := t.common()
	if typ.size > ptrSize {
		// Assume ptrSize >= 4, so this must be float64.
		ptr := unsafe_New(typ)
		*(*float64)(unsafe.Pointer(ptr)) = v
		return Value{typ, ptr, f | flagIndir | flag(typ.Kind())<<flagKindShift}
	}

	var w iword
	switch typ.size {
	case 4:
		*(*float32)(unsafe.Pointer(&w)) = float32(v)
	case 8:
		*(*float64)(unsafe.Pointer(&w)) = v
	}
	return Value{typ, unsafe.Pointer(w), f | flag(typ.Kind())<<flagKindShift}
}

// makeComplex returns a Value of type t equal to v (possibly truncated to complex64),
// where t is a complex64 or complex128 type.
func makeComplex(f flag, v complex128, t Type) Value {
	typ := t.common()
	if typ.size > ptrSize {
		ptr := unsafe_New(typ)
		switch typ.size {
		case 8:
			*(*complex64)(unsafe.Pointer(ptr)) = complex64(v)
		case 16:
			*(*complex128)(unsafe.Pointer(ptr)) = v
		}
		return Value{typ, ptr, f | flagIndir | flag(typ.Kind())<<flagKindShift}
	}

	// Assume ptrSize <= 8 so this must be complex64.
	var w iword
	*(*complex64)(unsafe.Pointer(&w)) = complex64(v)
	return Value{typ, unsafe.Pointer(w), f | flag(typ.Kind())<<flagKindShift}
}

func makeString(f flag, v string, t Type) Value {
	ret := New(t).Elem()
	ret.SetString(v)
	ret.flag = ret.flag&^flagAddr | f
	return ret
}

func makeBytes(f flag, v []byte, t Type) Value {
	ret := New(t).Elem()
	ret.SetBytes(v)
	ret.flag = ret.flag&^flagAddr | f
	return ret
}

func makeRunes(f flag, v []rune, t Type) Value {
	ret := New(t).Elem()
	ret.setRunes(v)
	ret.flag = ret.flag&^flagAddr | f
	return ret
}
