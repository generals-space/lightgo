package reflect

import (
	"unsafe"
	"runtime"
)

// Call calls the function v with the input arguments in.
// For example, if len(in) == 3, v.Call(in) represents the Go call v(in[0], in[1], in[2]).
// Call panics if v's Kind is not Func.
// It returns the output results as Values.
// As in Go, each input argument must be assignable to the
// type of the function's corresponding input parameter.
// If v is a variadic function, Call creates the variadic slice parameter
// itself, copying in the corresponding values.
func (v Value) Call(in []Value) []Value {
	v.mustBe(Func)
	v.mustBeExported()
	return v.call("Call", in)
}

// CallSlice calls the variadic function v with the input arguments in,
// assigning the slice in[len(in)-1] to v's final variadic argument.
// For example, if len(in) == 3, v.Call(in) represents the Go call v(in[0], in[1], in[2]...).
// Call panics if v's Kind is not Func or if v is not variadic.
// It returns the output results as Values.
// As in Go, each input argument must be assignable to the
// type of the function's corresponding input parameter.
func (v Value) CallSlice(in []Value) []Value {
	v.mustBe(Func)
	v.mustBeExported()
	return v.call("CallSlice", in)
}

func (v Value) call(op string, in []Value) []Value {
	// Get function pointer, type.
	t := v.typ
	var (
		fn   unsafe.Pointer
		rcvr iword
	)
	if v.flag&flagMethod != 0 {
		t, fn, rcvr = methodReceiver(op, v, int(v.flag)>>flagMethodShift)
	} else if v.flag&flagIndir != 0 {
		fn = *(*unsafe.Pointer)(v.val)
	} else {
		fn = v.val
	}

	if fn == nil {
		panic("reflect.Value.Call: call of nil function")
	}

	isSlice := op == "CallSlice"
	n := t.NumIn()
	if isSlice {
		if !t.IsVariadic() {
			panic("reflect: CallSlice of non-variadic function")
		}
		if len(in) < n {
			panic("reflect: CallSlice with too few input arguments")
		}
		if len(in) > n {
			panic("reflect: CallSlice with too many input arguments")
		}
	} else {
		if t.IsVariadic() {
			n--
		}
		if len(in) < n {
			panic("reflect: Call with too few input arguments")
		}
		if !t.IsVariadic() && len(in) > n {
			panic("reflect: Call with too many input arguments")
		}
	}
	for _, x := range in {
		if x.Kind() == Invalid {
			panic("reflect: " + op + " using zero Value argument")
		}
	}
	for i := 0; i < n; i++ {
		if xt, targ := in[i].Type(), t.In(i); !xt.AssignableTo(targ) {
			panic("reflect: " + op + " using " + xt.String() + " as type " + targ.String())
		}
	}
	if !isSlice && t.IsVariadic() {
		// prepare slice for remaining values
		m := len(in) - n
		slice := MakeSlice(t.In(n), m, m)
		elem := t.In(n).Elem()
		for i := 0; i < m; i++ {
			x := in[n+i]
			if xt := x.Type(); !xt.AssignableTo(elem) {
				panic("reflect: cannot use " + xt.String() + " as type " + elem.String() + " in " + op)
			}
			slice.Index(i).Set(x)
		}
		origIn := in
		in = make([]Value, n+1)
		copy(in[:n], origIn)
		in[n] = slice
	}

	nin := len(in)
	if nin != t.NumIn() {
		panic("reflect.Value.Call: wrong argument count")
	}
	nout := t.NumOut()

	// Compute arg size & allocate.
	// This computation is 5g/6g/8g-dependent
	// and probably wrong for gccgo, but so
	// is most of this function.
	size, _, _, _ := frameSize(t, v.flag&flagMethod != 0)

	// Copy into args.
	//
	// TODO(rsc): This will need to be updated for any new garbage collector.
	// For now make everything look like a pointer by allocating
	// a []unsafe.Pointer.
	args := make([]unsafe.Pointer, size/ptrSize)
	ptr := unsafe.Pointer(&args[0])
	off := uintptr(0)
	if v.flag&flagMethod != 0 {
		// Hard-wired first argument.
		*(*iword)(ptr) = rcvr
		off = ptrSize
	}
	for i, v := range in {
		v.mustBeExported()
		targ := t.In(i).(*rtype)
		a := uintptr(targ.align)
		off = (off + a - 1) &^ (a - 1)
		n := targ.size
		addr := unsafe.Pointer(uintptr(ptr) + off)
		v = v.assignTo("reflect.Value.Call", targ, (*interface{})(addr))
		if v.flag&flagIndir == 0 {
			storeIword(addr, iword(v.val), n)
		} else {
			memmove(addr, v.val, n)
		}
		off += n
	}
	off = (off + ptrSize - 1) &^ (ptrSize - 1)

	// Call.
	call(fn, ptr, uint32(size))

	// Copy return values out of args.
	//
	// TODO(rsc): revisit like above.
	ret := make([]Value, nout)
	for i := 0; i < nout; i++ {
		tv := t.Out(i)
		a := uintptr(tv.Align())
		off = (off + a - 1) &^ (a - 1)
		fl := flagIndir | flag(tv.Kind())<<flagKindShift
		ret[i] = Value{tv.common(), unsafe.Pointer(uintptr(ptr) + off), fl}
		off += tv.Size()
	}

	return ret
}

// caller:
// 	1. src/pkg/reflect/asm_amd64.s -> ·makeFuncStub 只有这一处
//
// callReflect is the call implementation used by a function
// returned by MakeFunc. In many ways it is the opposite of the
// method Value.call above. The method above converts a call using Values
// into a call of a function with a concrete argument frame, while
// callReflect converts a call of a function with a concrete argument
// frame into a call using Values.
// It is in this file so that it can be next to the call method above.
// The remainder of the MakeFunc implementation is in makefunc.go.
//
// NOTE: This function must be marked as a "wrapper" in the generated code,
// so that the linker can make it work correctly for panic and recover.
// The gc compilers know to do that for the name "reflect.callReflect".
func callReflect(ctxt *makeFuncImpl, frame unsafe.Pointer) {
	ftyp := ctxt.typ
	f := ctxt.fn

	// Copy argument frame into Values.
	ptr := frame
	off := uintptr(0)
	in := make([]Value, 0, len(ftyp.in))
	for _, arg := range ftyp.in {
		typ := arg
		off += -off & uintptr(typ.align-1)
		v := Value{typ, nil, flag(typ.Kind()) << flagKindShift}
		if typ.size <= ptrSize {
			// value fits in word.
			v.val = unsafe.Pointer(loadIword(unsafe.Pointer(uintptr(ptr)+off), typ.size))
		} else {
			// value does not fit in word.
			// Must make a copy, because f might keep a reference to it,
			// and we cannot let f keep a reference to the stack frame
			// after this function returns, not even a read-only reference.
			v.val = unsafe_New(typ)
			memmove(v.val, unsafe.Pointer(uintptr(ptr)+off), typ.size)
			v.flag |= flagIndir
		}
		in = append(in, v)
		off += typ.size
	}

	// Call underlying function.
	out := f(in)
	if len(out) != len(ftyp.out) {
		panic("reflect: wrong return count from function created by MakeFunc")
	}

	// Copy results back into argument frame.
	if len(ftyp.out) > 0 {
		off += -off & (ptrSize - 1)
		for i, arg := range ftyp.out {
			typ := arg
			v := out[i]
			if v.typ != typ {
				panic("reflect: function created by MakeFunc using " + funcName(f) +
					" returned wrong type: have " +
					out[i].typ.String() + " for " + typ.String())
			}
			if v.flag&flagRO != 0 {
				panic("reflect: function created by MakeFunc using " + funcName(f) +
					" returned value obtained from unexported field")
			}
			off += -off & uintptr(typ.align-1)
			addr := unsafe.Pointer(uintptr(ptr) + off)
			if v.flag&flagIndir == 0 {
				storeIword(addr, iword(v.val), typ.size)
			} else {
				memmove(addr, v.val, typ.size)
			}
			off += typ.size
		}
	}
}

// caller:
// 	1. src/pkg/reflect/asm_amd64.s -> ·methodValueCall() 只有这一处
//
// callMethod is the call implementation used by a function returned
// by makeMethodValue (used by v.Method(i).Interface()).
// It is a streamlined version of the usual reflect call: the caller has
// already laid out the argument frame for us, so we don't have
// to deal with individual Values for each argument.
// It is in this file so that it can be next to the two similar functions above.
// The remainder of the makeMethodValue implementation is in makefunc.go.
//
// NOTE: This function must be marked as a "wrapper" in the generated code,
// so that the linker can make it work correctly for panic and recover.
// The gc compilers know to do that for the name "reflect.callMethod".
func callMethod(ctxt *methodValue, frame unsafe.Pointer) {
	t, fn, rcvr := methodReceiver("call", ctxt.rcvr, ctxt.method)
	total, in, outOffset, out := frameSize(t, true)

	// Copy into args.
	//
	// TODO(rsc): This will need to be updated for any new garbage collector.
	// For now make everything look like a pointer by allocating
	// a []unsafe.Pointer.
	args := make([]unsafe.Pointer, total/ptrSize)
	args[0] = unsafe.Pointer(rcvr)
	base := unsafe.Pointer(&args[0])
	memmove(unsafe.Pointer(uintptr(base)+ptrSize), frame, in)

	// Call.
	call(fn, unsafe.Pointer(&args[0]), uint32(total))

	// Copy return values.
	memmove(
		unsafe.Pointer(uintptr(frame)+outOffset-ptrSize), 
		unsafe.Pointer(uintptr(base)+outOffset), 
		out,
	)
}

func (t *rtype) AssignableTo(u Type) bool {
	if u == nil {
		panic("reflect: nil type passed to Type.AssignableTo")
	}
	uu := u.(*rtype)
	return directlyAssignable(uu, t) || implements(uu, t)
}

////////////////////////////////////////////////////////////////////////////////

// funcName returns the name of f, for use in error messages.
func funcName(f func([]Value) []Value) string {
	pc := *(*uintptr)(unsafe.Pointer(&f))
	rf := runtime.FuncForPC(pc)
	if rf != nil {
		return rf.Name()
	}
	return "closure"
}

// methodReceiver returns information about the receiver described by v.
// The Value v may or may not have the flagMethod bit set,
// so the kind cached in v.flag should not be used.
func methodReceiver(
	op string, v Value, methodIndex int,
) (t *rtype, fn unsafe.Pointer, rcvr iword) {
	i := methodIndex
	if v.typ.Kind() == Interface {
		tt := (*interfaceType)(unsafe.Pointer(v.typ))
		if i < 0 || i >= len(tt.methods) {
			panic("reflect: internal error: invalid method index")
		}
		m := &tt.methods[i]
		if m.pkgPath != nil {
			panic("reflect: " + op + " of unexported method")
		}
		t = m.typ
		iface := (*nonEmptyInterface)(v.val)
		if iface.itab == nil {
			panic("reflect: " + op + " of method on nil interface value")
		}
		fn = unsafe.Pointer(&iface.itab.fun[i])
		rcvr = iface.word
	} else {
		ut := v.typ.uncommon()
		if ut == nil || i < 0 || i >= len(ut.methods) {
			panic("reflect: internal error: invalid method index")
		}
		m := &ut.methods[i]
		if m.pkgPath != nil {
			panic("reflect: " + op + " of unexported method")
		}
		fn = unsafe.Pointer(&m.ifn)
		t = m.mtyp
		rcvr = v.iword()
	}
	return
}

// align returns the result of rounding x up to a multiple of n.
// n must be a power of two.
func align(x, n uintptr) uintptr {
	return (x + n - 1) &^ (n - 1)
}

// frameSize returns the sizes of the argument and result frame
// for a function of the given type. The rcvr bool specifies whether
// a one-word receiver should be included in the total.
func frameSize(t *rtype, rcvr bool) (total, in, outOffset, out uintptr) {
	if rcvr {
		// extra word for receiver interface word
		total += ptrSize
	}

	nin := t.NumIn()
	in = -total
	for i := 0; i < nin; i++ {
		tv := t.In(i)
		total = align(total, uintptr(tv.Align()))
		total += tv.Size()
	}
	in += total
	total = align(total, ptrSize)
	nout := t.NumOut()
	outOffset = total
	out = -total
	for i := 0; i < nout; i++ {
		tv := t.Out(i)
		total = align(total, uintptr(tv.Align()))
		total += tv.Size()
	}
	out += total

	// total must be > 0 in order for &args[0] to be valid.
	// the argument copying is going to round it up to
	// a multiple of ptrSize anyway, so make it ptrSize to begin with.
	if total < ptrSize {
		total = ptrSize
	}

	// round to pointer
	total = align(total, ptrSize)

	return
}
