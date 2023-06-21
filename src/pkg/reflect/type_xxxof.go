package reflect

import (
	"unsafe"
	"strconv"
)

// garbage collection bytecode program for chan.
// See ../../cmd/gc/reflect.c:/^dgcsym1 and :/^dgcsym.
type chanGC struct {
	width uintptr // sizeof(map)
	op    uintptr // _GC_CHAN_PTR
	off   uintptr // 0
	typ   *rtype  // map type
	end   uintptr // _GC_END
}

type badGC struct {
	width uintptr
	end   uintptr
}

// ChanOf returns the channel type with the given direction and element type.
// For example, if t represents int, ChanOf(RecvDir, t) represents <-chan int.
//
// The gc runtime imposes a limit of 64 kB on channel element types.
// If t's size is equal to or exceeds this limit, ChanOf panics.
func ChanOf(dir ChanDir, t Type) Type {
	typ := t.(*rtype)

	// Look in cache.
	ckey := cacheKey{Chan, typ, nil, uintptr(dir)}
	if ch := cacheGet(ckey); ch != nil {
		return ch
	}

	// This restriction is imposed by the gc compiler and the runtime.
	if typ.size >= 1<<16 {
		lookupCache.Unlock()
		panic("reflect.ChanOf: element size too large")
	}

	// Look in known types.
	// TODO: Precedence when constructing string.
	var s string
	switch dir {
	default:
		lookupCache.Unlock()
		panic("reflect.ChanOf: invalid dir")
	case SendDir:
		s = "chan<- " + *typ.string
	case RecvDir:
		s = "<-chan " + *typ.string
	case BothDir:
		s = "chan " + *typ.string
	}
	for _, tt := range typesByString(s) {
		ch := (*chanType)(unsafe.Pointer(tt))
		if ch.elem == typ && ch.dir == uintptr(dir) {
			return cachePut(ckey, tt)
		}
	}

	// Make a channel type.
	var ichan interface{} = (chan unsafe.Pointer)(nil)
	prototype := *(**chanType)(unsafe.Pointer(&ichan))
	ch := new(chanType)
	*ch = *prototype
	ch.string = &s
	ch.hash = fnv1(typ.hash, 'c', byte(dir))
	ch.elem = typ
	ch.uncommonType = nil
	ch.ptrToThis = nil

	ch.gc = unsafe.Pointer(&chanGC{
		width: ch.size,
		op:    _GC_CHAN_PTR,
		off:   0,
		typ:   &ch.rtype,
		end:   _GC_END,
	})

	// INCORRECT. Uncomment to check that TestChanOfGC fails when ch.gc is wrong.
	//ch.gc = unsafe.Pointer(&badGC{width: ch.size, end: _GC_END})

	return cachePut(ckey, &ch.rtype)
}

func ismapkey(*rtype) bool // implemented in runtime

// MapOf returns the map type with the given key and element types.
// For example, if k represents int and e represents string,
// MapOf(k, e) represents map[int]string.
//
// If the key type is not a valid map key type (that is, if it does
// not implement Go's == operator), MapOf panics.
func MapOf(key, elem Type) Type {
	ktyp := key.(*rtype)
	etyp := elem.(*rtype)

	if !ismapkey(ktyp) {
		panic("reflect.MapOf: invalid key type " + ktyp.String())
	}

	// Look in cache.
	ckey := cacheKey{Map, ktyp, etyp, 0}
	if mt := cacheGet(ckey); mt != nil {
		return mt
	}

	// Look in known types.
	s := "map[" + *ktyp.string + "]" + *etyp.string
	for _, tt := range typesByString(s) {
		mt := (*mapType)(unsafe.Pointer(tt))
		if mt.key == ktyp && mt.elem == etyp {
			return cachePut(ckey, tt)
		}
	}

	// Make a map type.
	var imap interface{} = (map[unsafe.Pointer]unsafe.Pointer)(nil)
	prototype := *(**mapType)(unsafe.Pointer(&imap))
	mt := new(mapType)
	*mt = *prototype
	mt.string = &s
	mt.hash = fnv1(etyp.hash, 'm', byte(ktyp.hash>>24), byte(ktyp.hash>>16), byte(ktyp.hash>>8), byte(ktyp.hash))
	mt.key = ktyp
	mt.elem = etyp
	mt.bucket = bucketOf(ktyp, etyp)
	mt.hmap = hMapOf(mt.bucket)
	mt.uncommonType = nil
	mt.ptrToThis = nil

	// INCORRECT. Uncomment to check that TestMapOfGC and TestMapOfGCValues
	// fail when mt.gc is wrong.
	//mt.gc = unsafe.Pointer(&badGC{width: mt.size, end: _GC_END})

	return cachePut(ckey, &mt.rtype)
}

// Make sure these routines stay in sync with ../../pkg/runtime/hashmap.c!
// These types exist only for GC, so we only fill out GC relevant info.
// Currently, that's just size and the GC program. 
// We also fill in string for possible debugging use.
const (
	_BUCKETSIZE = 8
	_MAXKEYSIZE = 128
	_MAXVALSIZE = 128
)

// caller:
// 	1. MapOf() 只有这一处
//
func bucketOf(ktyp, etyp *rtype) *rtype {
	if ktyp.size > _MAXKEYSIZE {
		ktyp = PtrTo(ktyp).(*rtype)
	}
	if etyp.size > _MAXVALSIZE {
		etyp = PtrTo(etyp).(*rtype)
	}
	ptrsize := unsafe.Sizeof(uintptr(0))

	gc := make([]uintptr, 1)                                       // first entry is size, filled in at the end
	offset := _BUCKETSIZE * unsafe.Sizeof(uint8(0))                // topbits
	gc = append(gc, _GC_PTR, offset, 0 /*self pointer set below*/) // overflow
	offset += ptrsize

	// keys
	if ktyp.kind&kindNoPointers == 0 {
		gc = append(gc, _GC_ARRAY_START, offset, _BUCKETSIZE, ktyp.size)
		gc = appendGCProgram(gc, ktyp)
		gc = append(gc, _GC_ARRAY_NEXT)
	}
	offset += _BUCKETSIZE * ktyp.size

	// values
	if etyp.kind&kindNoPointers == 0 {
		gc = append(gc, _GC_ARRAY_START, offset, _BUCKETSIZE, etyp.size)
		gc = appendGCProgram(gc, etyp)
		gc = append(gc, _GC_ARRAY_NEXT)
	}
	offset += _BUCKETSIZE * etyp.size

	gc = append(gc, _GC_END)
	gc[0] = offset
	gc[3] = uintptr(unsafe.Pointer(&gc[0])) // set self pointer

	b := new(rtype)
	b.size = offset
	b.gc = unsafe.Pointer(&gc[0])
	s := "bucket(" + *ktyp.string + "," + *etyp.string + ")"
	b.string = &s
	return b
}

// 具体解析见 ../runtime/mgc0.h
//
// NOTE: These are copied from ../runtime/mgc0.h.
// They must be kept in sync.
const (
	_GC_END = iota
	_GC_PTR
	_GC_APTR
	_GC_ARRAY_START
	_GC_ARRAY_NEXT
	_GC_CALL
	_GC_CHAN_PTR
	_GC_STRING
	_GC_EFACE
	_GC_IFACE
	_GC_SLICE
	_GC_REGION
	_GC_NUM_INSTR
)

// caller:
// 	1. bucketOf() 只有这一处
//
// Take the GC program for "t" and append it to the GC program "gc".
func appendGCProgram(gc []uintptr, t *rtype) []uintptr {
	p := t.gc
	p = unsafe.Pointer(uintptr(p) + unsafe.Sizeof(uintptr(0))) // skip size
loop:
	for {
		var argcnt int
		switch *(*uintptr)(p) {
		case _GC_END:
			// Note: _GC_END not included in append
			break loop
		case _GC_ARRAY_NEXT:
			argcnt = 0
		case _GC_APTR, _GC_STRING, _GC_EFACE, _GC_IFACE:
			argcnt = 1
		case _GC_PTR, _GC_CALL, _GC_CHAN_PTR, _GC_SLICE:
			argcnt = 2
		case _GC_ARRAY_START, _GC_REGION:
			argcnt = 3
		default:
			panic(
				"unknown GC program op for " + *t.string + ": " + 
				strconv.FormatUint(*(*uint64)(p), 10),
			)
		}
		for i := 0; i < argcnt+1; i++ {
			gc = append(gc, *(*uintptr)(p))
			p = unsafe.Pointer(uintptr(p) + unsafe.Sizeof(uintptr(0)))
		}
	}
	return gc
}
func hMapOf(bucket *rtype) *rtype {
	ptrsize := unsafe.Sizeof(uintptr(0))

	// make gc program & compute hmap size
	gc := make([]uintptr, 1)           // first entry is size, filled in at the end
	offset := unsafe.Sizeof(uint(0))   // count
	offset += unsafe.Sizeof(uint32(0)) // flags
	offset += unsafe.Sizeof(uint32(0)) // hash0
	offset += unsafe.Sizeof(uint8(0))  // B
	offset += unsafe.Sizeof(uint8(0))  // keysize
	offset += unsafe.Sizeof(uint8(0))  // valuesize
	offset = (offset + 1) / 2 * 2
	offset += unsafe.Sizeof(uint16(0)) // bucketsize
	offset = (offset + ptrsize - 1) / ptrsize * ptrsize
	gc = append(gc, _GC_PTR, offset, uintptr(bucket.gc)) // buckets
	offset += ptrsize
	gc = append(gc, _GC_PTR, offset, uintptr(bucket.gc)) // oldbuckets
	offset += ptrsize
	offset += ptrsize // nevacuate
	gc = append(gc, _GC_END)
	gc[0] = offset

	h := new(rtype)
	h.size = offset
	h.gc = unsafe.Pointer(&gc[0])
	s := "hmap(" + *bucket.string + ")"
	h.string = &s
	return h
}

// garbage collection bytecode program for slice of non-zero-length values.
// See ../../cmd/gc/reflect.c:/^dgcsym1 and :/^dgcsym.
type sliceGC struct {
	width  uintptr        // sizeof(slice)
	op     uintptr        // _GC_SLICE
	off    uintptr        // 0
	elemgc unsafe.Pointer // element gc program
	end    uintptr        // _GC_END
}

// garbage collection bytecode program for slice of zero-length values.
// See ../../cmd/gc/reflect.c:/^dgcsym1 and :/^dgcsym.
type sliceEmptyGC struct {
	width uintptr // sizeof(slice)
	op    uintptr // _GC_APTR
	off   uintptr // 0
	end   uintptr // _GC_END
}

var sliceEmptyGCProg = sliceEmptyGC{
	width: unsafe.Sizeof([]byte(nil)),
	op:    _GC_APTR,
	off:   0,
	end:   _GC_END,
}

// SliceOf 返回目标类型对应的数组类型.
//
// SliceOf returns the slice type with element type t.
// For example, if t represents int, SliceOf(t) represents []int.
func SliceOf(t Type) Type {
	typ := t.(*rtype)

	// Look in cache.
	ckey := cacheKey{Slice, typ, nil, 0}
	if slice := cacheGet(ckey); slice != nil {
		return slice
	}

	// Look in known types.
	s := "[]" + *typ.string
	for _, tt := range typesByString(s) {
		slice := (*sliceType)(unsafe.Pointer(tt))
		if slice.elem == typ {
			return cachePut(ckey, tt)
		}
	}

	// Make a slice type.
	var islice interface{} = ([]unsafe.Pointer)(nil)
	prototype := *(**sliceType)(unsafe.Pointer(&islice))
	slice := new(sliceType)
	*slice = *prototype
	slice.string = &s
	slice.hash = fnv1(typ.hash, '[')
	slice.elem = typ
	slice.uncommonType = nil
	slice.ptrToThis = nil

	if typ.size == 0 {
		slice.gc = unsafe.Pointer(&sliceEmptyGCProg)
	} else {
		slice.gc = unsafe.Pointer(&sliceGC{
			width:  slice.size,
			op:     _GC_SLICE,
			off:    0,
			elemgc: typ.gc,
			end:    _GC_END,
		})
	}

	// INCORRECT. Uncomment to check that TestSliceOfOfGC fails when slice.gc is wrong.
	//slice.gc = unsafe.Pointer(&badGC{width: slice.size, end: _GC_END})

	return cachePut(ckey, &slice.rtype)
}

// ArrayOf returns the array type with the given count and element type.
// For example, if t represents int, ArrayOf(5, t) represents [5]int.
//
// If the resulting type would be larger than the available address space,
// ArrayOf panics.
//
// TODO(rsc): Unexported for now. Export once the alg field is set correctly
// for the type. This may require significant work.
func arrayOf(count int, elem Type) Type {
	typ := elem.(*rtype)
	slice := SliceOf(elem)

	// Look in cache.
	ckey := cacheKey{Array, typ, nil, uintptr(count)}
	if slice := cacheGet(ckey); slice != nil {
		return slice
	}

	// Look in known types.
	s := "[" + strconv.Itoa(count) + "]" + *typ.string
	for _, tt := range typesByString(s) {
		slice := (*sliceType)(unsafe.Pointer(tt))
		if slice.elem == typ {
			return cachePut(ckey, tt)
		}
	}

	// Make an array type.
	var iarray interface{} = [1]unsafe.Pointer{}
	prototype := *(**arrayType)(unsafe.Pointer(&iarray))
	array := new(arrayType)
	*array = *prototype
	array.string = &s
	array.hash = fnv1(typ.hash, '[')
	for n := uint32(count); n > 0; n >>= 8 {
		array.hash = fnv1(array.hash, byte(n))
	}
	array.hash = fnv1(array.hash, ']')
	array.elem = typ
	max := ^uintptr(0) / typ.size
	if uintptr(count) > max {
		panic("reflect.ArrayOf: array size would exceed virtual address space")
	}
	array.size = typ.size * uintptr(count)
	array.align = typ.align
	array.fieldAlign = typ.fieldAlign
	// TODO: array.alg
	// TODO: array.gc
	array.uncommonType = nil
	array.ptrToThis = nil
	array.len = uintptr(count)
	array.slice = slice.(*rtype)

	return cachePut(ckey, &array.rtype)
}

////////////////////////////////////////////////////////////////////////////////

// typelinks is implemented in package runtime.
// It returns a slice of all the 'typelink' information in the binary,
// which is to say a slice of known types, sorted by string.
// Note that strings are not unique identifiers for types:
// there can be more than one with a given string.
// Only types we might want to look up are included:
// channels, maps, slices, and arrays.
func typelinks() []*rtype

// typesByString returns the subslice of typelinks() whose elements have
// the given string representation.
// It may be empty (no known types with that string) or may have
// multiple elements (multiple types with that string).
func typesByString(s string) []*rtype {
	typ := typelinks()

	// We are looking for the first index i where the string becomes >= s.
	// This is a copy of sort.Search, with f(h) replaced by (*typ[h].string >= s).
	i, j := 0, len(typ)
	for i < j {
		h := i + (j-i)/2 // avoid overflow when computing h
		// i ≤ h < j
		if !(*typ[h].string >= s) {
			i = h + 1 // preserves f(i-1) == false
		} else {
			j = h // preserves f(j) == true
		}
	}
	// i == j, f(i-1) == false, and f(j) (= f(i)) == true  =>  answer is i.

	// Having found the first, linear scan forward to find the last.
	// We could do a second binary search, but the caller is going
	// to do a linear scan anyway.
	j = i
	for j < len(typ) && *typ[j].string == s {
		j++
	}

	// This slice will be empty if the string is not found.
	return typ[i:j]
}
