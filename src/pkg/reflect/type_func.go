package reflect

import (
	"unsafe"
)

////////////////////////////////////////////////////////////////////////////////
// Func 函数类型相关

// NumIn 返回当前**函数类型**的入参数量.
func (t *rtype) NumIn() int {
	if t.Kind() != Func {
		panic("reflect: NumIn of non-func type")
	}
	tt := (*funcType)(unsafe.Pointer(t))
	return len(tt.in)
}

// NumOut 返回当前**函数类型**的返回值数量.
func (t *rtype) NumOut() int {
	if t.Kind() != Func {
		panic("reflect: NumOut of non-func type")
	}
	tt := (*funcType)(unsafe.Pointer(t))
	return len(tt.out)
}

// In 返回当前**函数类型**的第i个入参的类型.
func (t *rtype) In(i int) Type {
	if t.Kind() != Func {
		panic("reflect: In of non-func type")
	}
	tt := (*funcType)(unsafe.Pointer(t))
	return toType(tt.in[i])
}

// Out 返回当前**函数类型**的第i个返回值的类型.
func (t *rtype) Out(i int) Type {
	if t.Kind() != Func {
		panic("reflect: Out of non-func type")
	}
	tt := (*funcType)(unsafe.Pointer(t))
	return toType(tt.out[i])
}
////////////////////////////////////////////////////////////////////////////////

// funcType represents a function type.
type funcType struct {
	rtype     `reflect:"func"`
	dotdotdot bool     // last input parameter is ...
	in        []*rtype // input parameter types
	out       []*rtype // output parameter types
}

func (t *rtype) IsVariadic() bool {
	if t.Kind() != Func {
		panic("reflect: IsVariadic of non-func type")
	}
	tt := (*funcType)(unsafe.Pointer(t))
	return tt.dotdotdot
}
