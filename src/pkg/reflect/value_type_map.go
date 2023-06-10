package reflect

import (
	"unsafe"
)

// MapIndex returns the value associated with key in the map v.
// It panics if v's Kind is not Map.
// It returns the zero Value if key is not found in the map or if v represents a nil map.
// As in Go, the key's value must be assignable to the map's key type.
func (v Value) MapIndex(key Value) Value {
	v.mustBe(Map)
	tt := (*mapType)(unsafe.Pointer(v.typ))

	// Do not require key to be exported, so that DeepEqual
	// and other programs can use all the keys returned by
	// MapKeys as arguments to MapIndex.  If either the map
	// or the key is unexported, though, the result will be
	// considered unexported.  This is consistent with the
	// behavior for structs, which allow read but not write
	// of unexported fields.
	key = key.assignTo("reflect.Value.MapIndex", tt.key, nil)

	word, ok := mapaccess(v.typ, v.iword(), key.iword())
	if !ok {
		return Value{}
	}
	typ := tt.elem
	fl := (v.flag | key.flag) & flagRO
	if typ.size > ptrSize {
		fl |= flagIndir
	}
	fl |= flag(typ.Kind()) << flagKindShift
	return Value{typ, unsafe.Pointer(word), fl}
}

// MapKeys returns a slice containing all the keys present in the map,
// in unspecified order.
// It panics if v's Kind is not Map.
// It returns an empty slice if v represents a nil map.
func (v Value) MapKeys() []Value {
	v.mustBe(Map)
	tt := (*mapType)(unsafe.Pointer(v.typ))
	keyType := tt.key

	fl := v.flag & flagRO
	fl |= flag(keyType.Kind()) << flagKindShift
	if keyType.size > ptrSize {
		fl |= flagIndir
	}

	m := v.iword()
	mlen := int(0)
	if m != nil {
		mlen = maplen(m)
	}
	it := mapiterinit(v.typ, m)
	a := make([]Value, mlen)
	var i int
	for i = 0; i < len(a); i++ {
		keyWord, ok := mapiterkey(it)
		if !ok {
			break
		}
		a[i] = Value{keyType, unsafe.Pointer(keyWord), fl}
		mapiternext(it)
	}
	return a[:i]
}
