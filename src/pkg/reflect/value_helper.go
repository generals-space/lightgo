package reflect

import (
	"unsafe"
	"strconv"
)

// A ValueError occurs when a Value method is invoked on
// a Value that does not support it.  Such cases are documented
// in the description of each method.
type ValueError struct {
	Method string
	Kind   Kind
}

func (e *ValueError) Error() string {
	if e.Kind == 0 {
		return "reflect: call of " + e.Method + " on zero Value"
	}
	return "reflect: call of " + e.Method + " on " + e.Kind.String() + " Value"
}

// An iword is the word that would be stored in an interface
// to represent a given value v. 
// Specifically, if v is bigger than a pointer, its word is a pointer to v's data.
// Otherwise, its word holds the data stored in its leading bytes
// (so is not a pointer).
// Because the value sometimes holds a pointer, we use unsafe.Pointer
// to represent it,
// so that if iword appears in a struct, the garbage collector knows
// that might be a pointer.
type iword unsafe.Pointer

func (v Value) iword() iword {
	if v.flag&flagIndir != 0 && v.typ.size <= ptrSize {
		// Have indirect but want direct word.
		return loadIword(v.val, v.typ.size)
	}
	return iword(v.val)
}

// loadIword loads n bytes at p from memory into an iword.
func loadIword(p unsafe.Pointer, n uintptr) iword {
	// Run the copy ourselves instead of calling memmove
	// to avoid moving w to the heap.
	var w iword
	switch n {
	default:
		panic("reflect: internal error: loadIword of " + strconv.Itoa(int(n)) + "-byte value")
	case 0:
	case 1:
		*(*uint8)(unsafe.Pointer(&w)) = *(*uint8)(p)
	case 2:
		*(*uint16)(unsafe.Pointer(&w)) = *(*uint16)(p)
	case 3:
		*(*[3]byte)(unsafe.Pointer(&w)) = *(*[3]byte)(p)
	case 4:
		*(*uint32)(unsafe.Pointer(&w)) = *(*uint32)(p)
	case 5:
		*(*[5]byte)(unsafe.Pointer(&w)) = *(*[5]byte)(p)
	case 6:
		*(*[6]byte)(unsafe.Pointer(&w)) = *(*[6]byte)(p)
	case 7:
		*(*[7]byte)(unsafe.Pointer(&w)) = *(*[7]byte)(p)
	case 8:
		*(*uint64)(unsafe.Pointer(&w)) = *(*uint64)(p)
	}
	return w
}

// storeIword stores n bytes from w into p.
func storeIword(p unsafe.Pointer, w iword, n uintptr) {
	// Run the copy ourselves instead of calling memmove
	// to avoid moving w to the heap.
	switch n {
	default:
		panic("reflect: internal error: storeIword of " + strconv.Itoa(int(n)) + "-byte value")
	case 0:
	case 1:
		*(*uint8)(p) = *(*uint8)(unsafe.Pointer(&w))
	case 2:
		*(*uint16)(p) = *(*uint16)(unsafe.Pointer(&w))
	case 3:
		*(*[3]byte)(p) = *(*[3]byte)(unsafe.Pointer(&w))
	case 4:
		*(*uint32)(p) = *(*uint32)(unsafe.Pointer(&w))
	case 5:
		*(*[5]byte)(p) = *(*[5]byte)(unsafe.Pointer(&w))
	case 6:
		*(*[6]byte)(p) = *(*[6]byte)(unsafe.Pointer(&w))
	case 7:
		*(*[7]byte)(p) = *(*[7]byte)(unsafe.Pointer(&w))
	case 8:
		*(*uint64)(p) = *(*uint64)(unsafe.Pointer(&w))
	}
}

////////////////////////////////////////////////////////////////////////////////

// StringHeader is the runtime representation of a string.
// It cannot be used safely or portably and its representation may
// change in a later release.
// Moreover, the Data field is not sufficient to guarantee the data
// it references will not be garbage collected, so programs must keep
// a separate, correctly typed pointer to the underlying data.
type StringHeader struct {
	Data uintptr
	Len  int
}

// SliceHeader is the runtime representation of a slice.
// It cannot be used safely or portably and its representation may
// change in a later release.
// Moreover, the Data field is not sufficient to guarantee the data
// it references will not be garbage collected, so programs must keep
// a separate, correctly typed pointer to the underlying data.
type SliceHeader struct {
	Data uintptr
	Len  int
	Cap  int
}
