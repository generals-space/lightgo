package reflect

import (
	"unsafe"
	"strconv"
)

// sliceType 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> SliceType{}
//
// sliceType represents a slice type.
type sliceType struct {
	rtype `reflect:"slice"`
	elem  *rtype // slice element type
}

// arrayType 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> {}
//
// 	@todo: 这个跟 sliceType 啥关系, 没有映射
//
// arrayType represents a fixed array type.
type arrayType struct {
	rtype `reflect:"array"`
	elem  *rtype // array element type
	slice *rtype // slice type
	len   uintptr
}

func (t *rtype) Len() int {
	if t.Kind() != Array {
		panic("reflect: Len of non-array type")
	}
	tt := (*arrayType)(unsafe.Pointer(t))
	return int(tt.len)
}

////////////////////////////////////////////////////////////////////////////////
// mapType 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> MapType{}
//
// mapType represents a map type.
type mapType struct {
	rtype  `reflect:"map"`
	key    *rtype // map key type
	elem   *rtype // map element (value) type
	bucket *rtype // internal bucket structure
	hmap   *rtype // internal map header
}

func (t *rtype) Key() Type {
	if t.Kind() != Map {
		panic("reflect: Key of non-map type")
	}
	tt := (*mapType)(unsafe.Pointer(t))
	return toType(tt.key)
}

////////////////////////////////////////////////////////////////////////////////

// ChanDir represents a channel type's direction.
type ChanDir int

const (
	RecvDir ChanDir             = 1 << iota // <-chan
	SendDir                                 // chan<-
	BothDir = RecvDir | SendDir             // chan
)

func (d ChanDir) String() string {
	switch d {
	case SendDir:
		return "chan<-"
	case RecvDir:
		return "<-chan"
	case BothDir:
		return "chan"
	}
	return "ChanDir" + strconv.Itoa(int(d))
}

// chanType 该结构是编译器已知的, 见
// src/pkg/runtime/type.h -> ChanType{}
//
// chanType represents a channel type.
type chanType struct {
	rtype `reflect:"chan"`
	elem  *rtype  // channel element type
	dir   uintptr // channel direction (ChanDir)
}

func (t *rtype) ChanDir() ChanDir {
	if t.Kind() != Chan {
		panic("reflect: ChanDir of non-chan type")
	}
	tt := (*chanType)(unsafe.Pointer(t))
	return ChanDir(tt.dir)
}
