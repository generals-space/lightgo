// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package reflect

import (
	"unsafe"
)

const bigEndian = false // can be smarter if we find a big-endian machine
const ptrSize = unsafe.Sizeof((*byte)(nil))
const cannotSet = "cannot set value obtained from unexported struct field"

// TODO: This will have to go away when the new gc goes in.
func memmove(adst, asrc unsafe.Pointer, n uintptr) {
	dst := uintptr(adst)
	src := uintptr(asrc)
	switch {
	case src < dst && src+n > dst:
		// byte copy backward
		// careful: i is unsigned
		for i := n; i > 0; {
			i--
			*(*byte)(unsafe.Pointer(dst + i)) = *(*byte)(unsafe.Pointer(src + i))
		}
	case (n|src|dst)&(ptrSize-1) != 0:
		// byte copy forward
		for i := uintptr(0); i < n; i++ {
			*(*byte)(unsafe.Pointer(dst + i)) = *(*byte)(unsafe.Pointer(src + i))
		}
	default:
		// word copy forward
		for i := uintptr(0); i < n; i += ptrSize {
			*(*uintptr)(unsafe.Pointer(dst + i)) = *(*uintptr)(unsafe.Pointer(src + i))
		}
	}
}

// reflect.Type 是一个接口, 而 reflect.Value 是一个结构体.
//
// Value is the reflection interface to a Go value.
//
// Not all methods apply to all kinds of values.  Restrictions,
// if any, are noted in the documentation for each method.
// Use the Kind method to find out the kind of value before
// calling kind-specific methods.  Calling a method
// inappropriate to the kind of type causes a run time panic.
//
// The zero Value represents no value.
// Its IsValid method returns false, its Kind method returns Invalid,
// its String method returns "<invalid Value>", and all other methods panic.
// Most functions and methods never return an invalid value.
// If one does, its documentation states the conditions explicitly.
//
// A Value can be used concurrently by multiple goroutines provided that
// the underlying Go value can be used concurrently for the equivalent
// direct operations.
type Value struct {
	// typ holds the type of the value represented by a Value.
	typ *rtype

	// 	@compatible: 该成员字段在 v1.3 版本由 val 被改为 ptr, 这里做了一个全局替换.
	//
	// 当前 Value 对象中, 实际存储数据的指针.
	// 该 val 指针可以被转换为其实际的类型, 如 (*int64)(v.ptr)
	//
	// Value 对象中有 SetXXX 函数族, 可以对 val 成员赋值.
	//
	// val holds the 1-word representation of the value.
	// If flag's flagIndir bit is set, then val is a pointer to the data.
	// Otherwise val is a word holding the actual data.
	// When the data is smaller than a word, it begins at
	// the first byte (in the memory address sense) of val.
	// We use unsafe.Pointer so that the garbage collector
	// knows that val could be a pointer.
	ptr unsafe.Pointer

	// flag holds metadata about the value.
	// The lowest bits are flag bits:
	//	- flagRO: obtained via unexported field, so read-only
	//	- flagIndir: val holds a pointer to the data
	//	- flagAddr: v.CanAddr is true (implies flagIndir)
	//	- flagMethod: v is a method value.
	// The next five bits give the Kind of the value.
	// This repeats typ.Kind() except for method values.
	// The remaining 23+ bits give a method number for method values.
	// If flag.kind() != Func, code can assume that flagMethod is unset.
	// If typ.size > ptrSize, code can assume that flagIndir is set.
	flag

	// A method value represents a curried method invocation
	// like r.Read for some receiver r.  The typ+val+flag bits describe
	// the receiver r, but the flag's Kind bits say Func (methods are
	// functions), and the top bits of the flag give the method number
	// in r's type's method table.
}

// Go 语言的 interface{} 类型在语言内部就是通过 emptyInterface{} 表示的.
//
// emptyInterface is the header for an interface{} value.
type emptyInterface struct {
	typ  *rtype
	word iword
}

// nonEmptyInterface is the header for a interface value with methods.
type nonEmptyInterface struct {
	// see ../runtime/iface.c:/Itab
	itab *struct {
		ityp   *rtype // static interface type
		typ    *rtype // dynamic concrete type
		link   unsafe.Pointer
		bad    int32
		unused int32
		fun    [100000]unsafe.Pointer // method table
	}
	word iword
}

// Elem returns the value that the interface v contains
// or that the pointer v points to.
// It panics if v's Kind is not Interface or Ptr.
// It returns the zero Value if v is nil.
func (v Value) Elem() Value {
	k := v.kind()
	switch k {
	case Interface:
		var (
			typ *rtype
			val unsafe.Pointer
		)
		if v.typ.NumMethod() == 0 {
			eface := (*emptyInterface)(v.ptr)
			if eface.typ == nil {
				// nil interface value
				return Value{}
			}
			typ = eface.typ
			val = unsafe.Pointer(eface.word)
		} else {
			iface := (*nonEmptyInterface)(v.ptr)
			if iface.itab == nil {
				// nil interface value
				return Value{}
			}
			typ = iface.itab.typ
			val = unsafe.Pointer(iface.word)
		}
		fl := v.flag & flagRO
		fl |= flag(typ.Kind()) << flagKindShift
		if typ.size > ptrSize {
			fl |= flagIndir
		}
		return Value{typ, val, fl}

	case Ptr:
		val := v.ptr
		if v.flag&flagIndir != 0 {
			val = *(*unsafe.Pointer)(val)
		}
		// The returned value's address is v's value.
		if val == nil {
			return Value{}
		}
		tt := (*ptrType)(unsafe.Pointer(v.typ))
		typ := tt.elem
		fl := v.flag&flagRO | flagIndir | flagAddr
		fl |= flag(typ.Kind() << flagKindShift)
		return Value{typ, val, fl}
	}
	panic(&ValueError{"reflect.Value.Elem", k})
}

////////////////////////////////////////////////////////////////////////////////
//

// IsNil returns true if v is a nil value.
// It panics if v's Kind is not Chan, Func, Interface, Map, Ptr, or Slice.
func (v Value) IsNil() bool {
	k := v.kind()
	switch k {
	case Chan, Func, Map, Ptr:
		if v.flag&flagMethod != 0 {
			return false
		}
		ptr := v.ptr
		if v.flag&flagIndir != 0 {
			ptr = *(*unsafe.Pointer)(ptr)
		}
		return ptr == nil
	case Interface, Slice:
		// Both interface and slice are nil if first word is 0.
		// Both are always bigger than a word; assume flagIndir.
		return *(*unsafe.Pointer)(v.ptr) == nil
	}
	panic(&ValueError{"reflect.Value.IsNil", k})
}

// Kind 返回的值应该与该 Value 对象对应的 Type 对象的 Kind() 值相同.
//
// Kind returns v's Kind.
// If v is the zero Value (IsValid returns false), Kind returns Invalid.
func (v Value) Kind() Kind {
	return v.kind()
}

var uint8Type = TypeOf(uint8(0)).(*rtype)

// Index returns v's i'th element.
// It panics if v's Kind is not Array, Slice, or String or i is out of range.
func (v Value) Index(i int) Value {
	k := v.kind()
	switch k {
	case Array:
		tt := (*arrayType)(unsafe.Pointer(v.typ))
		if i < 0 || i > int(tt.len) {
			panic("reflect: array index out of range")
		}
		typ := tt.elem
		fl := v.flag & (flagRO | flagIndir | flagAddr) // bits same as overall array
		fl |= flag(typ.Kind()) << flagKindShift
		offset := uintptr(i) * typ.size

		var val unsafe.Pointer
		switch {
		case fl&flagIndir != 0:
			// Indirect.  Just bump pointer.
			val = unsafe.Pointer(uintptr(v.ptr) + offset)
		case bigEndian:
			// Direct.  Discard leading bytes.
			val = unsafe.Pointer(uintptr(v.ptr) << (offset * 8))
		default:
			// Direct.  Discard leading bytes.
			val = unsafe.Pointer(uintptr(v.ptr) >> (offset * 8))
		}
		return Value{typ, val, fl}

	case Slice:
		// Element flag same as Elem of Ptr.
		// Addressable, indirect, possibly read-only.
		fl := flagAddr | flagIndir | v.flag&flagRO
		s := (*SliceHeader)(v.ptr)
		if i < 0 || i >= s.Len {
			panic("reflect: slice index out of range")
		}
		tt := (*sliceType)(unsafe.Pointer(v.typ))
		typ := tt.elem
		fl |= flag(typ.Kind()) << flagKindShift
		val := unsafe.Pointer(s.Data + uintptr(i)*typ.size)
		return Value{typ, val, fl}

	case String:
		fl := v.flag&flagRO | flag(Uint8<<flagKindShift)
		s := (*StringHeader)(v.ptr)
		if i < 0 || i >= s.Len {
			panic("reflect: string index out of range")
		}
		val := *(*byte)(unsafe.Pointer(s.Data + uintptr(i)))
		return Value{uint8Type, unsafe.Pointer(uintptr(val)), fl}
	}
	panic(&ValueError{"reflect.Value.Index", k})
}

// Cap returns v's capacity.
// It panics if v's Kind is not Array, Chan, or Slice.
func (v Value) Cap() int {
	k := v.kind()
	switch k {
	case Array:
		return v.typ.Len()
	case Chan:
		return int(chancap(v.iword()))
	case Slice:
		// Slice is always bigger than a word; assume flagIndir.
		return (*SliceHeader)(v.ptr).Cap
	}
	panic(&ValueError{"reflect.Value.Cap", k})
}

// Len returns v's length.
// It panics if v's Kind is not Array, Chan, Map, Slice, or String.
func (v Value) Len() int {
	k := v.kind()
	switch k {
	case Array:
		tt := (*arrayType)(unsafe.Pointer(v.typ))
		return int(tt.len)
	case Chan:
		return chanlen(v.iword())
	case Map:
		return maplen(v.iword())
	case Slice:
		// Slice is bigger than a word; assume flagIndir.
		return (*SliceHeader)(v.ptr).Len
	case String:
		// String is bigger than a word; assume flagIndir.
		return (*StringHeader)(v.ptr).Len
	}
	panic(&ValueError{"reflect.Value.Len", k})
}
//
////////////////////////////////////////////////////////////////////////////////

// Pointer returns v's value as a uintptr.
// It returns uintptr instead of unsafe.Pointer so that
// code using reflect cannot obtain unsafe.Pointers
// without importing the unsafe package explicitly.
// It panics if v's Kind is not Chan, Func, Map, Ptr, Slice, or UnsafePointer.
//
// If v's Kind is Func, the returned pointer is an underlying
// code pointer, but not necessarily enough to identify a
// single function uniquely. The only guarantee is that the
// result is zero if and only if v is a nil func Value.
func (v Value) Pointer() uintptr {
	k := v.kind()
	switch k {
	case Chan, Map, Ptr, UnsafePointer:
		p := v.ptr
		if v.flag&flagIndir != 0 {
			p = *(*unsafe.Pointer)(p)
		}
		return uintptr(p)
	case Func:
		if v.flag&flagMethod != 0 {
			// As the doc comment says, the returned pointer is an
			// underlying code pointer but not necessarily enough to
			// identify a single function uniquely. All method expressions
			// created via reflect have the same underlying code pointer,
			// so their Pointers are equal. The function used here must
			// match the one used in makeMethodValue.
			f := methodValueCall
			return **(**uintptr)(unsafe.Pointer(&f))
		}
		p := v.ptr
		if v.flag&flagIndir != 0 {
			p = *(*unsafe.Pointer)(p)
		}
		// Non-nil func value points at data block.
		// First word of data block is actual code.
		if p != nil {
			p = *(*unsafe.Pointer)(p)
		}
		return uintptr(p)

	case Slice:
		return (*SliceHeader)(v.ptr).Data
	}
	panic(&ValueError{"reflect.Value.Pointer", k})
}

// Type returns v's type.
func (v Value) Type() Type {
	f := v.flag
	if f == 0 {
		panic(&ValueError{"reflect.Value.Type", Invalid})
	}
	if f&flagMethod == 0 {
		// Easy case
		return v.typ
	}

	// Method value.
	// v.typ describes the receiver, not the method type.
	i := int(v.flag) >> flagMethodShift
	if v.typ.Kind() == Interface {
		// Method on interface.
		tt := (*interfaceType)(unsafe.Pointer(v.typ))
		if i < 0 || i >= len(tt.methods) {
			panic("reflect: internal error: invalid method index")
		}
		m := &tt.methods[i]
		return m.typ
	}
	// Method on concrete type.
	ut := v.typ.uncommon()
	if ut == nil || i < 0 || i >= len(ut.methods) {
		panic("reflect: internal error: invalid method index")
	}
	m := &ut.methods[i]
	return m.mtyp
}

// Indirect returns the value that v points to.
// If v is a nil pointer, Indirect returns a zero Value.
// If v is not a pointer, Indirect returns v.
func Indirect(v Value) Value {
	if v.Kind() != Ptr {
		return v
	}
	return v.Elem()
}

// ValueOf Value 对象打印出来如 1, "this is a string", {general}, &{general} 等.
//
// ValueOf returns a new Value initialized to the concrete value
// stored in the interface i.  ValueOf(nil) returns the zero Value.
func ValueOf(i interface{}) Value {
	if i == nil {
		return Value{}
	}

	// TODO(rsc): Eliminate this terrible hack.
	// In the call to packValue, eface.typ doesn't escape,
	// and eface.word is an integer.  So it looks like
	// i (= eface) doesn't escape.  But really it does,
	// because eface.word is actually a pointer.
	escapes(i)

	// For an interface value with the noAddr bit set,
	// the representation is identical to an empty interface.
	eface := *(*emptyInterface)(unsafe.Pointer(&i))
	typ := eface.typ
	fl := flag(typ.Kind()) << flagKindShift
	if typ.size > ptrSize {
		fl |= flagIndir
	}
	return Value{typ, unsafe.Pointer(eface.word), fl}
}

// Zero returns a Value representing the zero value for the specified type.
// The result is different from the zero value of the Value struct,
// which represents no value at all.
// For example, Zero(TypeOf(42)) returns a Value with Kind Int and value 0.
// The returned value is neither addressable nor settable.
func Zero(typ Type) Value {
	if typ == nil {
		panic("reflect: Zero(nil)")
	}
	t := typ.common()
	fl := flag(t.Kind()) << flagKindShift
	if t.size <= ptrSize {
		return Value{t, nil, fl}
	}
	return Value{t, unsafe_New(typ.(*rtype)), fl | flagIndir}
}

// assignTo returns a value v that can be assigned directly to typ.
// It panics if v is not assignable to typ.
// For a conversion to an interface type, target is a suggested scratch space to use.
func (v Value) assignTo(context string, dst *rtype, target *interface{}) Value {
	if v.flag&flagMethod != 0 {
		v = makeMethodValue(context, v)
	}

	switch {
	case directlyAssignable(dst, v.typ):
		// Overwrite type so that they match.
		// Same memory layout, so no harm done.
		v.typ = dst
		fl := v.flag & (flagRO | flagAddr | flagIndir)
		fl |= flag(dst.Kind()) << flagKindShift
		return Value{dst, v.ptr, fl}

	case implements(dst, v.typ):
		if target == nil {
			target = new(interface{})
		}
		x := valueInterface(v, false)
		if dst.NumMethod() == 0 {
			*target = x
		} else {
			ifaceE2I(dst, x, unsafe.Pointer(target))
		}
		return Value{dst, unsafe.Pointer(target), flagIndir | flag(Interface)<<flagKindShift}
	}

	// Failed.
	panic(context + ": value of type " + v.typ.String() + " is not assignable to type " + dst.String())
}

// implemented in ../pkg/runtime
func chancap(ch iword) int
func chanclose(ch iword)
func chanlen(ch iword) int
func chanrecv(t *rtype, ch iword, nb bool) (val iword, selected, received bool)
func chansend(t *rtype, ch iword, val iword, nb bool) bool

func makechan(typ *rtype, size uint64) (ch iword)
func makemap(t *rtype) (m iword)
func mapaccess(t *rtype, m iword, key iword) (val iword, ok bool)
func mapassign(t *rtype, m iword, key, val iword, ok bool)
func mapiterinit(t *rtype, m iword) *byte
func mapiterkey(it *byte) (key iword, ok bool)
func mapiternext(it *byte)
func maplen(m iword) int

func call(fn, arg unsafe.Pointer, n uint32)
func ifaceE2I(t *rtype, src interface{}, dst unsafe.Pointer)

// caller:
// 	1. ValueOf() 只有这一处
//
// Dummy annotation marking that the value x escapes,
// for use in cases where the reflect code is so clever that
// the compiler cannot follow.
func escapes(x interface{}) {
	if dummy.b {
		dummy.x = x
	}
}

var dummy struct {
	b bool
	x interface{}
}
