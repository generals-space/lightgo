package reflect

import (
	"unsafe"
)

// Method represents a single method.
type Method struct {
	// Name 方法名, 即常规的函数名, 但是只有名称, 并不是函数声明.
	//
	// Name is the method name.
	// PkgPath is the package path that qualifies a lower case (unexported)
	// method name.  It is empty for upper case (exported) method names.
	// The combination of PkgPath and Name uniquely identifies a method
	// in a method set.
	// See http://golang.org/ref/spec#Uniqueness_of_identifiers
	Name    string
	PkgPath string

	// Type 表示实际的函数声明, 如: func(string) string
	//
	// method type
	Type  Type
	// func with receiver as first argument
	Func  Value
	// 当前 Method 方法在其所属的 接口/对象 中的索引值.
	//
	// index for Type.Method
	Index int
}

// TODO(rsc): 6g supplies these, but they are not as efficient as they could be:
// they have commonType as the receiver instead of *rtype.
func (t *rtype) NumMethod() int {
	if t.Kind() == Interface {
		tt := (*interfaceType)(unsafe.Pointer(t))
		return tt.NumMethod()
	}
	return t.uncommonType.NumMethod()
}

func (t *rtype) Method(i int) (m Method) {
	if t.Kind() == Interface {
		tt := (*interfaceType)(unsafe.Pointer(t))
		return tt.Method(i)
	}
	return t.uncommonType.Method(i)
}

func (t *rtype) MethodByName(name string) (m Method, ok bool) {
	if t.Kind() == Interface {
		tt := (*interfaceType)(unsafe.Pointer(t))
		return tt.MethodByName(name)
	}
	return t.uncommonType.MethodByName(name)
}

////////////////////////////////////////////////////////////////////////////////

// Implements 判断当前类型 t 是否实现了目标接口 u
//
// 不管是接口类型还是普通实例类型, ta们拥有的方法(rtype.methods)都是经过字母排序的,
// 只要按顺序比较即可.
//
// 	@param u: 必须是一个接口类型, 而非一个具体的变量/对象
func (t *rtype) Implements(u Type) bool {
	if u == nil {
		panic("reflect: nil type passed to Type.Implements")
	}
	// u 必须是接口类型.
	if u.Kind() != Interface {
		panic("reflect: non-interface type passed to Type.Implements")
	}
	return implements(u.(*rtype), t)
}

// Implements 判断普通变量/对象的类型 V 是否实现了目标接口类型 T.

// 不管是接口类型还是普通实例类型, ta们拥有的方法(rtype.methods)都是经过字母排序的,
// 只要按顺序比较即可.
//
// 	@param T: 必须是接口类型
// 	@param V: 一个对象类型, 也可以是父接口
//
// caller:
// 	1. rtype.Implements()
//
// implements returns true if the type V implements the interface type T.
func implements(T, V *rtype) bool {
	// T 必须是接口类型.
	if T.Kind() != Interface {
		return false
	}
	t := (*interfaceType)(unsafe.Pointer(T))
	// 如果接口中不包含任何方法, 表示这是一个空接口, 任意类型都自动实现该接口, 直接返回 true.
	if len(t.methods) == 0 {
		return true
	}

	// 如果 V 也是接口类型
	//
	// The same algorithm applies in both cases, but the
	// method tables for an interface type and a concrete type
	// are different, so the code is duplicated.
	// In both cases the algorithm is a linear scan over the two
	// lists - T's methods and V's methods - simultaneously.
	// Since method tables are stored in a unique sorted order
	// (alphabetical, with no duplicate method names), the scan
	// through V's methods must hit a match for each of T's
	// methods along the way, or else V does not implement T.
	// This lets us run the scan in overall linear time instead of
	// the quadratic time  a naive search would require.
	// See also ../runtime/iface.c.
	if V.Kind() == Interface {
		v := (*interfaceType)(unsafe.Pointer(V))
		i := 0
		// 不管是接口类型还是普通实例类型, ta们拥有的方法(rtype.methods)都是经过字母排序的,
		// 只要按顺序比较即可.
		for j := 0; j < len(v.methods); j++ {
			tm := &t.methods[i]
			vm := &v.methods[j]
			if vm.name == tm.name && vm.pkgPath == tm.pkgPath && vm.typ == tm.typ {
				if i++; i >= len(t.methods) {
					return true
				}
			}
		}
		return false
	}

	v := V.uncommon()
	if v == nil {
		return false
	}
	i := 0
	for j := 0; j < len(v.methods); j++ {
		// 同上述的 for{} 循环.
		tm := &t.methods[i]
		vm := &v.methods[j]
		if vm.name == tm.name && vm.pkgPath == tm.pkgPath && vm.mtyp == tm.typ {
			if i++; i >= len(t.methods) {
				return true
			}
		}
	}
	return false
}

////////////////////////////////////////////////////////////////////////////////

// imethod 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> IMethod{}
//
// imethod represents a method on an interface type
type imethod struct {
	name    *string // name of method
	pkgPath *string // nil for exported Names; otherwise import path
	typ     *rtype  // .(*FuncType) underneath
}

// interfaceType 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> InterfaceType{}
//
// interfaceType represents an interface type.
type interfaceType struct {
	rtype   `reflect:"interface"`
	methods []imethod // sorted by hash
}

// caller:
// 	1. rtype.Method()
//
// Method returns the i'th method in the type's method set.
func (t *interfaceType) Method(i int) (m Method) {
	if i < 0 || i >= len(t.methods) {
		return
	}
	p := &t.methods[i]
	m.Name = *p.name
	if p.pkgPath != nil {
		m.PkgPath = *p.pkgPath
	}
	m.Type = toType(p.typ)
	m.Index = i
	return
}

// NumMethod 返回当前**结构体类型**中的**成员方法**的数量. 
//
// 类似的还有 NumField(), 返回**成员属性**的数量.
//
// 注意: 目标类型对象必须是**结构体**, 否则会 panic.
//
// caller:
// 	1. rtype.NumMethod()
//
// NumMethod returns the number of interface methods in the type's method set.
func (t *interfaceType) NumMethod() int { 
	return len(t.methods) 
}

// caller:
// 	1. rtype.MethodByName()
//
// MethodByName method with the given name in the type's method set.
func (t *interfaceType) MethodByName(name string) (m Method, ok bool) {
	if t == nil {
		return
	}
	var p *imethod
	for i := range t.methods {
		p = &t.methods[i]
		if *p.name == name {
			return t.Method(i), true
		}
	}
	return
}
