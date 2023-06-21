package reflect

import (
	"unsafe"
)

func (t *rtype) PkgPath() string {
	return t.uncommonType.PkgPath()
}

func (t *rtype) Name() string {
	return t.uncommonType.Name()
}

////////////////////////////////////////////////////////////////////////////////

// method 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> Method{}
//
// Method on non-interface type
type method struct {
	name    *string        // name of method
	pkgPath *string        // nil for exported Names; otherwise import path
	mtyp    *rtype         // method type (without receiver)
	typ     *rtype         // .(*FuncType) underneath (with receiver)
	ifn     unsafe.Pointer // fn used in interface call (one-word receiver)
	tfn     unsafe.Pointer // fn used for normal method call
}

// uncommonType 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> UncommonType{}
//
// uncommonType is present only for types with names or methods
// (if T is a named type, the uncommonTypes for T and *T have methods).
// Using a pointer to this struct reduces the overall size required
// to describe an unnamed type with no methods.
type uncommonType struct {
	name    *string  // name of type
	pkgPath *string  // import path; nil for built-in types like int, string
	methods []method // methods associated with type
}

func (t *uncommonType) uncommon() *uncommonType {
	return t
}

func (t *uncommonType) PkgPath() string {
	if t == nil || t.pkgPath == nil {
		return ""
	}
	return *t.pkgPath
}

func (t *uncommonType) Name() string {
	if t == nil || t.name == nil {
		return ""
	}
	return *t.name
}

func (t *uncommonType) Method(i int) (m Method) {
	if t == nil || i < 0 || i >= len(t.methods) {
		panic("reflect: Method index out of range")
	}
	p := &t.methods[i]
	if p.name != nil {
		m.Name = *p.name
	}
	fl := flag(Func) << flagKindShift
	if p.pkgPath != nil {
		m.PkgPath = *p.pkgPath
		fl |= flagRO
	}
	mt := p.typ
	m.Type = mt
	fn := unsafe.Pointer(&p.tfn)
	m.Func = Value{mt, fn, fl}
	m.Index = i
	return
}

func (t *uncommonType) NumMethod() int {
	if t == nil {
		return 0
	}
	return len(t.methods)
}

func (t *uncommonType) MethodByName(name string) (m Method, ok bool) {
	if t == nil {
		return
	}
	var p *method
	for i := range t.methods {
		p = &t.methods[i]
		if p.name != nil && *p.name == name {
			return t.Method(i), true
		}
	}
	return
}
