package reflect

import (
	"sync"
	"unsafe"
)

////////////////////////////////////////////////////////////////////////////////

// ptrType represents a pointer type.
type ptrType struct {
	rtype `reflect:"ptr"`
	elem  *rtype // pointer element (pointed at) type
}

// ptrMap is the cache for PtrTo.
var ptrMap struct {
	sync.RWMutex
	m map[*rtype]*ptrType
}

// garbage collection bytecode program for pointer to memory without pointers.
// See ../../cmd/gc/reflect.c:/^dgcsym1 and :/^dgcsym.
type ptrDataGC struct {
	width uintptr // sizeof(ptr)
	op    uintptr // _GC_APTR
	off   uintptr // 0
	end   uintptr // _GC_END
}

var ptrDataGCProg = ptrDataGC{
	width: unsafe.Sizeof((*byte)(nil)),
	op:    _GC_APTR,
	off:   0,
	end:   _GC_END,
}

// garbage collection bytecode program for pointer to memory with pointers.
// See ../../cmd/gc/reflect.c:/^dgcsym1 and :/^dgcsym.
type ptrGC struct {
	width  uintptr        // sizeof(ptr)
	op     uintptr        // _GC_PTR
	off    uintptr        // 0
	elemgc unsafe.Pointer // element gc type
	end    uintptr        // _GC_END
}

// PtrTo returns the pointer type with element t.
// For example, if t represents type Foo, PtrTo(t) represents *Foo.
func PtrTo(t Type) Type {
	return t.(*rtype).ptrTo()
}

func (t *rtype) ptrTo() *rtype {
	if p := t.ptrToThis; p != nil {
		return p
	}

	// Otherwise, synthesize one.
	// This only happens for pointers with no methods.
	// We keep the mapping in a map on the side, because
	// this operation is rare and a separate map lets us keep
	// the type structures in read-only memory.
	ptrMap.RLock()
	if m := ptrMap.m; m != nil {
		if p := m[t]; p != nil {
			ptrMap.RUnlock()
			return &p.rtype
		}
	}
	ptrMap.RUnlock()
	ptrMap.Lock()
	if ptrMap.m == nil {
		ptrMap.m = make(map[*rtype]*ptrType)
	}
	p := ptrMap.m[t]
	if p != nil {
		// some other goroutine won the race and created it
		ptrMap.Unlock()
		return &p.rtype
	}

	// Create a new ptrType starting with the description
	// of an *unsafe.Pointer.
	p = new(ptrType)
	var iptr interface{} = (*unsafe.Pointer)(nil)
	prototype := *(**ptrType)(unsafe.Pointer(&iptr))
	*p = *prototype

	s := "*" + *t.string
	p.string = &s

	// For the type structures linked into the binary, the
	// compiler provides a good hash of the string.
	// Create a good hash for the new string by using
	// the FNV-1 hash's mixing function to combine the
	// old hash and the new "*".
	p.hash = fnv1(t.hash, '*')

	p.uncommonType = nil
	p.ptrToThis = nil
	p.elem = t

	if t.kind&kindNoPointers != 0 {
		p.gc = unsafe.Pointer(&ptrDataGCProg)
	} else {
		p.gc = unsafe.Pointer(&ptrGC{
			width:  p.size,
			op:     _GC_PTR,
			off:    0,
			elemgc: t.gc,
			end:    _GC_END,
		})
	}
	// INCORRECT. Uncomment to check that TestPtrToGC fails when p.gc is wrong.
	//p.gc = unsafe.Pointer(&badGC{width: p.size, end: _GC_END})

	ptrMap.m[t] = p
	ptrMap.Unlock()
	return &p.rtype
}

// fnv1 incorporates the list of bytes into the hash x using the FNV-1 hash function.
func fnv1(x uint32, list ...byte) uint32 {
	for _, b := range list {
		x = x*16777619 ^ uint32(b)
	}
	return x
}
