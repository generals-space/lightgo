// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package fmt

import (
	"reflect"
	"unicode/utf8"
)

// Some constants in the form of bytes, to avoid string overhead.
// Needlessly fastidious, I suppose.
var (
	commaSpaceBytes  = []byte(", ")
	nilAngleBytes    = []byte("<nil>")
	nilParenBytes    = []byte("(nil)")
	nilBytes         = []byte("nil")
	mapBytes         = []byte("map[")
	percentBangBytes = []byte("%!")
	missingBytes     = []byte("(MISSING)")
	badIndexBytes    = []byte("(BADINDEX)")
	panicBytes       = []byte("(PANIC=")
	extraBytes       = []byte("%!(EXTRA ")
	irparenBytes     = []byte("i)")
	bytesBytes       = []byte("[]byte{")
	badWidthBytes    = []byte("%!(BADWIDTH)")
	badPrecBytes     = []byte("%!(BADPREC)")
	noVerbBytes      = []byte("%!(NOVERB)")
)

// State represents the printer state passed to custom formatters.
// It provides access to the io.Writer interface plus information about
// the flags and options for the operand's format specifier.
type State interface {
	// Write is the function to call to emit formatted output to be printed.
	Write(b []byte) (ret int, err error)
	// Width returns the value of the width option and whether it has been set.
	Width() (wid int, ok bool)
	// Precision returns the value of the precision option and whether it has been set.
	Precision() (prec int, ok bool)

	// Flag reports whether the flag c, a character, has been set.
	Flag(c int) bool
}

// Formatter is the interface implemented by values with a custom formatter.
// The implementation of Format may call Sprint(f) or Fprint(f) etc.
// to generate its output.
type Formatter interface {
	Format(f State, c rune)
}

// Stringer is implemented by any value that has a String method,
// which defines the ``native'' format for that value.
// The String method is used to print values passed as an operand
// to any format that accepts a string or to an unformatted printer
// such as Print.
type Stringer interface {
	String() string
}

// GoStringer is implemented by any value that has a GoString method,
// which defines the Go syntax for that value.
// The GoString method is used to print values passed as an operand
// to a %#v format.
type GoStringer interface {
	GoString() string
}

// Use simple []byte instead of bytes.Buffer to avoid large dependency.
type buffer []byte

func (b *buffer) Write(p []byte) (n int, err error) {
	*b = append(*b, p...)
	return len(p), nil
}

func (b *buffer) WriteString(s string) (n int, err error) {
	*b = append(*b, s...)
	return len(s), nil
}

func (b *buffer) WriteByte(c byte) error {
	*b = append(*b, c)
	return nil
}

func (bp *buffer) WriteRune(r rune) error {
	if r < utf8.RuneSelf {
		*bp = append(*bp, byte(r))
		return nil
	}

	b := *bp
	n := len(b)
	for n+utf8.UTFMax > cap(b) {
		b = append(b, 0)
	}
	w := utf8.EncodeRune(b[n:n+utf8.UTFMax], r)
	*bp = b[:n+w]
	return nil
}

type pp struct {
	n         int
	panicking bool
	erroring  bool // printing an error condition
	buf       buffer
	// arg holds the current item, as an interface{}.
	arg interface{}
	// value holds the current item, as a reflect.Value, and will be
	// the zero Value if the item has not been reflected.
	value reflect.Value
	// reordered records whether the format string used argument reordering.
	reordered bool
	// goodArgNum records whether the most recent reordering directive was valid.
	goodArgNum bool
	runeBuf    [utf8.UTFMax]byte
	fmt        fmt
}

var ppFree = newCache(func() interface{} { return new(pp) })

// newPrinter allocates a new pp struct or grab a cached one.
func newPrinter() *pp {
	p := ppFree.get().(*pp)
	p.panicking = false
	p.erroring = false
	p.fmt.init(&p.buf)
	return p
}

// free saves used pp structs in ppFree; avoids an allocation per invocation.
func (p *pp) free() {
	// Don't hold on to pp structs with large buffers.
	if cap(p.buf) > 1024 {
		return
	}
	p.buf = p.buf[:0]
	p.arg = nil
	p.value = reflect.Value{}
	ppFree.put(p)
}

func (p *pp) Width() (wid int, ok bool) { return p.fmt.wid, p.fmt.widPresent }

func (p *pp) Precision() (prec int, ok bool) { return p.fmt.prec, p.fmt.precPresent }

func (p *pp) Flag(b int) bool {
	switch b {
	case '-':
		return p.fmt.minus
	case '+':
		return p.fmt.plus
	case '#':
		return p.fmt.sharp
	case ' ':
		return p.fmt.space
	case '0':
		return p.fmt.zero
	}
	return false
}

func (p *pp) add(c rune) {
	p.buf.WriteRune(c)
}

// Implement Write so we can call Fprintf on a pp (through State),
// for recursive use in custom verbs.
func (p *pp) Write(b []byte) (ret int, err error) {
	return p.buf.Write(b)
}

// getField gets the i'th field of the struct value.
// If the field is itself is an interface,
// return a value for the thing inside the interface, not the interface itself.
func getField(v reflect.Value, i int) reflect.Value {
	val := v.Field(i)
	if val.Kind() == reflect.Interface && !val.IsNil() {
		val = val.Elem()
	}
	return val
}

// parsenum converts ASCII to integer. 
// num is 0 (and isnum is false) if no number present.
func parsenum(s string, start, end int) (num int, isnum bool, newi int) {
	if start >= end {
		return 0, false, end
	}
	for newi = start; newi < end && '0' <= s[newi] && s[newi] <= '9'; newi++ {
		num = num*10 + int(s[newi]-'0')
		isnum = true
	}
	return
}

var (
	intBits     = reflect.TypeOf(0).Bits()
	uintptrBits = reflect.TypeOf(uintptr(0)).Bits()
)

func (p *pp) catchPanic(arg interface{}, verb rune) {
	if err := recover(); err != nil {
		// If it's a nil pointer, just say "<nil>". The likeliest causes are a
		// Stringer that fails to guard against nil or a nil pointer for a
		// value receiver, and in either case, "<nil>" is a nice result.
		if v := reflect.ValueOf(arg); v.Kind() == reflect.Ptr && v.IsNil() {
			p.buf.Write(nilAngleBytes)
			return
		}
		// Otherwise print a concise panic message. Most of the time the panic
		// value will print itself nicely.
		if p.panicking {
			// Nested panics; the recursion in printArg cannot succeed.
			panic(err)
		}
		p.buf.Write(percentBangBytes)
		p.add(verb)
		p.buf.Write(panicBytes)
		p.panicking = true
		p.printArg(err, 'v', false, false, 0)
		p.panicking = false
		p.buf.WriteByte(')')
	}
}

func (p *pp) handleMethods(verb rune, plus, goSyntax bool, depth int) (wasString, handled bool) {
	if p.erroring {
		return
	}
	// Is it a Formatter?
	if formatter, ok := p.arg.(Formatter); ok {
		handled = true
		wasString = false
		defer p.catchPanic(p.arg, verb)
		formatter.Format(p, verb)
		return
	}
	// Must not touch flags before Formatter looks at them.
	if plus {
		p.fmt.plus = false
	}

	// If we're doing Go syntax and the argument knows how to supply it, take care of it now.
	if goSyntax {
		p.fmt.sharp = false
		if stringer, ok := p.arg.(GoStringer); ok {
			wasString = false
			handled = true
			defer p.catchPanic(p.arg, verb)
			// Print the result of GoString unadorned.
			p.fmtString(stringer.GoString(), 's', false)
			return
		}
	} else {
		// If a string is acceptable according to the format, see if
		// the value satisfies one of the string-valued interfaces.
		// Println etc. set verb to %v, which is "stringable".
		switch verb {
		case 'v', 's', 'x', 'X', 'q':
			// Is it an error or Stringer?
			// The duplication in the bodies is necessary:
			// setting wasString and handled, and deferring catchPanic,
			// must happen before calling the method.
			switch v := p.arg.(type) {
			case error:
				wasString = false
				handled = true
				defer p.catchPanic(p.arg, verb)
				p.printArg(v.Error(), verb, plus, false, depth)
				return

			case Stringer:
				wasString = false
				handled = true
				defer p.catchPanic(p.arg, verb)
				p.printArg(v.String(), verb, plus, false, depth)
				return
			}
		}
	}
	handled = false
	return
}

func (p *pp) printArg(arg interface{}, verb rune, plus, goSyntax bool, depth int) (wasString bool) {
	p.arg = arg
	p.value = reflect.Value{}

	if arg == nil {
		if verb == 'T' || verb == 'v' {
			p.fmt.pad(nilAngleBytes)
		} else {
			p.badVerb(verb)
		}
		return false
	}

	// Special processing considerations.
	// %T (the value's type) and %p (its address) are special; we always do them first.
	switch verb {
	case 'T':
		p.printArg(reflect.TypeOf(arg).String(), 's', false, false, 0)
		return false
	case 'p':
		p.fmtPointer(reflect.ValueOf(arg), verb, goSyntax)
		return false
	}

	// Clear flags for base formatters.
	// handleMethods needs them, so we must restore them later.
	// We could call handleMethods here and avoid this work, but
	// handleMethods is expensive enough to be worth delaying.
	oldPlus := p.fmt.plus
	oldSharp := p.fmt.sharp
	if plus {
		p.fmt.plus = false
	}
	if goSyntax {
		p.fmt.sharp = false
	}

	// Some types can be done without reflection.
	switch f := arg.(type) {
	case bool:
		p.fmtBool(f, verb)
	case float32:
		p.fmtFloat32(f, verb)
	case float64:
		p.fmtFloat64(f, verb)
	case complex64:
		p.fmtComplex64(f, verb)
	case complex128:
		p.fmtComplex128(f, verb)
	case int:
		p.fmtInt64(int64(f), verb)
	case int8:
		p.fmtInt64(int64(f), verb)
	case int16:
		p.fmtInt64(int64(f), verb)
	case int32:
		p.fmtInt64(int64(f), verb)
	case int64:
		p.fmtInt64(f, verb)
	case uint:
		p.fmtUint64(uint64(f), verb, goSyntax)
	case uint8:
		p.fmtUint64(uint64(f), verb, goSyntax)
	case uint16:
		p.fmtUint64(uint64(f), verb, goSyntax)
	case uint32:
		p.fmtUint64(uint64(f), verb, goSyntax)
	case uint64:
		p.fmtUint64(f, verb, goSyntax)
	case uintptr:
		p.fmtUint64(uint64(f), verb, goSyntax)
	case string:
		p.fmtString(f, verb, goSyntax)
		wasString = verb == 's' || verb == 'v'
	case []byte:
		p.fmtBytes(f, verb, goSyntax, nil, depth)
		wasString = verb == 's'
	default:
		// Restore flags in case handleMethods finds a Formatter.
		p.fmt.plus = oldPlus
		p.fmt.sharp = oldSharp
		// If the type is not simple, it might have methods.
		if isString, handled := p.handleMethods(verb, plus, goSyntax, depth); handled {
			return isString
		}
		// Need to use reflection
		return p.printReflectValue(reflect.ValueOf(arg), verb, plus, goSyntax, depth)
	}
	p.arg = nil
	return
}

// printValue is like printArg but starts with a reflect value, not an interface{} value.
func (p *pp) printValue(value reflect.Value, verb rune, plus, goSyntax bool, depth int) (wasString bool) {
	if !value.IsValid() {
		if verb == 'T' || verb == 'v' {
			p.buf.Write(nilAngleBytes)
		} else {
			p.badVerb(verb)
		}
		return false
	}

	// Special processing considerations.
	// %T (the value's type) and %p (its address) are special; we always do them first.
	switch verb {
	case 'T':
		p.printArg(value.Type().String(), 's', false, false, 0)
		return false
	case 'p':
		p.fmtPointer(value, verb, goSyntax)
		return false
	}

	// Handle values with special methods.
	// Call always, even when arg == nil, because handleMethods clears p.fmt.plus for us.
	p.arg = nil // Make sure it's cleared, for safety.
	if value.CanInterface() {
		p.arg = value.Interface()
	}
	if isString, handled := p.handleMethods(verb, plus, goSyntax, depth); handled {
		return isString
	}

	return p.printReflectValue(value, verb, plus, goSyntax, depth)
}

// printReflectValue is the fallback for both printArg and printValue.
// It uses reflect to print the value.
func (p *pp) printReflectValue(value reflect.Value, verb rune, plus, goSyntax bool, depth int) (wasString bool) {
	oldValue := p.value
	p.value = value
BigSwitch:
	switch f := value; f.Kind() {
	case reflect.Bool:
		p.fmtBool(f.Bool(), verb)
	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		p.fmtInt64(f.Int(), verb)
	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64, reflect.Uintptr:
		p.fmtUint64(f.Uint(), verb, goSyntax)
	case reflect.Float32, reflect.Float64:
		if f.Type().Size() == 4 {
			p.fmtFloat32(float32(f.Float()), verb)
		} else {
			p.fmtFloat64(f.Float(), verb)
		}
	case reflect.Complex64, reflect.Complex128:
		if f.Type().Size() == 8 {
			p.fmtComplex64(complex64(f.Complex()), verb)
		} else {
			p.fmtComplex128(f.Complex(), verb)
		}
	case reflect.String:
		p.fmtString(f.String(), verb, goSyntax)
	case reflect.Map:
		if goSyntax {
			p.buf.WriteString(f.Type().String())
			if f.IsNil() {
				p.buf.WriteString("(nil)")
				break
			}
			p.buf.WriteByte('{')
		} else {
			p.buf.Write(mapBytes)
		}
		keys := f.MapKeys()
		for i, key := range keys {
			if i > 0 {
				if goSyntax {
					p.buf.Write(commaSpaceBytes)
				} else {
					p.buf.WriteByte(' ')
				}
			}
			p.printValue(key, verb, plus, goSyntax, depth+1)
			p.buf.WriteByte(':')
			p.printValue(f.MapIndex(key), verb, plus, goSyntax, depth+1)
		}
		if goSyntax {
			p.buf.WriteByte('}')
		} else {
			p.buf.WriteByte(']')
		}
	case reflect.Struct:
		if goSyntax {
			p.buf.WriteString(value.Type().String())
		}
		p.add('{')
		v := f
		t := v.Type()
		for i := 0; i < v.NumField(); i++ {
			if i > 0 {
				if goSyntax {
					p.buf.Write(commaSpaceBytes)
				} else {
					p.buf.WriteByte(' ')
				}
			}
			if plus || goSyntax {
				if f := t.Field(i); f.Name != "" {
					p.buf.WriteString(f.Name)
					p.buf.WriteByte(':')
				}
			}
			p.printValue(getField(v, i), verb, plus, goSyntax, depth+1)
		}
		p.buf.WriteByte('}')
	case reflect.Interface:
		value := f.Elem()
		if !value.IsValid() {
			if goSyntax {
				p.buf.WriteString(f.Type().String())
				p.buf.Write(nilParenBytes)
			} else {
				p.buf.Write(nilAngleBytes)
			}
		} else {
			wasString = p.printValue(value, verb, plus, goSyntax, depth+1)
		}
	case reflect.Array, reflect.Slice:
		// Byte slices are special.
		if typ := f.Type(); typ.Elem().Kind() == reflect.Uint8 {
			var bytes []byte
			if f.Kind() == reflect.Slice {
				bytes = f.Bytes()
			} else if f.CanAddr() {
				bytes = f.Slice(0, f.Len()).Bytes()
			} else {
				// We have an array, but we cannot Slice() a non-addressable array,
				// so we build a slice by hand. This is a rare case but it would be nice
				// if reflection could help a little more.
				bytes = make([]byte, f.Len())
				for i := range bytes {
					bytes[i] = byte(f.Index(i).Uint())
				}
			}
			p.fmtBytes(bytes, verb, goSyntax, typ, depth)
			wasString = verb == 's'
			break
		}
		if goSyntax {
			p.buf.WriteString(value.Type().String())
			if f.Kind() == reflect.Slice && f.IsNil() {
				p.buf.WriteString("(nil)")
				break
			}
			p.buf.WriteByte('{')
		} else {
			p.buf.WriteByte('[')
		}
		for i := 0; i < f.Len(); i++ {
			if i > 0 {
				if goSyntax {
					p.buf.Write(commaSpaceBytes)
				} else {
					p.buf.WriteByte(' ')
				}
			}
			p.printValue(f.Index(i), verb, plus, goSyntax, depth+1)
		}
		if goSyntax {
			p.buf.WriteByte('}')
		} else {
			p.buf.WriteByte(']')
		}
	case reflect.Ptr:
		v := f.Pointer()
		// pointer to array or slice or struct?  ok at top level
		// but not embedded (avoid loops)
		if v != 0 && depth == 0 {
			switch a := f.Elem(); a.Kind() {
			case reflect.Array, reflect.Slice:
				p.buf.WriteByte('&')
				p.printValue(a, verb, plus, goSyntax, depth+1)
				break BigSwitch
			case reflect.Struct:
				p.buf.WriteByte('&')
				p.printValue(a, verb, plus, goSyntax, depth+1)
				break BigSwitch
			}
		}
		fallthrough
	case reflect.Chan, reflect.Func, reflect.UnsafePointer:
		p.fmtPointer(value, verb, goSyntax)
	default:
		p.unknownType(f)
	}
	p.value = oldValue
	return wasString
}
