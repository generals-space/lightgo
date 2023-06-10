package reflect

import (
	"unsafe"
)

// Interface returns v's current value as an interface{}.
// It is equivalent to:
//	var i interface{} = (v's underlying value)
// It panics if the Value was obtained by accessing
// unexported struct fields.
func (v Value) Interface() (i interface{}) {
	return valueInterface(v, true)
}

func valueInterface(v Value, safe bool) interface{} {
	if v.flag == 0 {
		panic(&ValueError{"reflect.Value.Interface", 0})
	}
	if safe && v.flag&flagRO != 0 {
		// Do not allow access to unexported values via Interface,
		// because they might be pointers that should not be
		// writable or methods or function that should not be callable.
		panic("reflect.Value.Interface: cannot return value obtained from unexported field or method")
	}
	if v.flag&flagMethod != 0 {
		v = makeMethodValue("Interface", v)
	}

	k := v.kind()
	if k == Interface {
		// Special case: return the element inside the interface.
		// Empty interface has one layout, all interfaces with
		// methods have a second layout.
		if v.NumMethod() == 0 {
			return *(*interface{})(v.val)
		}
		return *(*interface {
			M()
		})(v.val)
	}

	// Non-interface value.
	var eface emptyInterface
	eface.typ = v.typ
	eface.word = v.iword()

	// Don't need to allocate if v is not addressable or fits in one word.
	if v.flag&flagAddr != 0 && v.typ.size > ptrSize {
		// eface.word is a pointer to the actual data,
		// which might be changed.  We need to return
		// a pointer to unchanging data, so make a copy.
		ptr := unsafe_New(v.typ)
		memmove(ptr, unsafe.Pointer(eface.word), v.typ.size)
		eface.word = iword(ptr)
	}

	return *(*interface{})(unsafe.Pointer(&eface))
}

// InterfaceData returns the interface v's value as a uintptr pair.
// It panics if v's Kind is not Interface.
func (v Value) InterfaceData() [2]uintptr {
	v.mustBe(Interface)
	// We treat this as a read operation, so we allow
	// it even for unexported data, because the caller
	// has to import "unsafe" to turn it into something
	// that can be abused.
	// Interface value is always bigger than a word; assume flagIndir.
	return *(*[2]uintptr)(v.val)
}
