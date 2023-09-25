// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package fmt

import (
	"strconv"
	"unicode/utf8"
)

const (
	nByte = 65 // %b of an int64, plus a sign.

	ldigits = "0123456789abcdef"
	udigits = "0123456789ABCDEF"
)

const (
	signed   = true
	unsigned = false
)

var padZeroBytes = make([]byte, nByte)
var padSpaceBytes = make([]byte, nByte)

func init() {
	for i := 0; i < nByte; i++ {
		padZeroBytes[i] = '0'
		padSpaceBytes[i] = ' '
	}
}

// A fmt is the raw formatter used by Printf etc.
// It prints into a buffer that must be set up separately.
type fmt struct {
	intbuf [nByte]byte
	buf    *buffer
	// width, precision
	wid  int
	prec int
	// flags
	widPresent  bool
	precPresent bool
	minus       bool
	plus        bool
	sharp       bool
	space       bool
	unicode     bool
	uniQuote    bool // Use 'x'= prefix for %U if printable.
	zero        bool
}

func (f *fmt) clearflags() {
	f.wid = 0
	f.widPresent = false
	f.prec = 0
	f.precPresent = false
	f.minus = false
	f.plus = false
	f.sharp = false
	f.space = false
	f.unicode = false
	f.uniQuote = false
	f.zero = false
}

func (f *fmt) init(buf *buffer) {
	f.buf = buf
	f.clearflags()
}

// computePadding computes left and right padding widths (only one will be non-zero).
func (f *fmt) computePadding(width int) (padding []byte, leftWidth, rightWidth int) {
	left := !f.minus
	w := f.wid
	if w < 0 {
		left = false
		w = -w
	}
	w -= width
	if w > 0 {
		if left && f.zero {
			return padZeroBytes, w, 0
		}
		if left {
			return padSpaceBytes, w, 0
		} else {
			// can't be zero padding on the right
			return padSpaceBytes, 0, w
		}
	}
	return
}

// writePadding generates n bytes of padding.
func (f *fmt) writePadding(n int, padding []byte) {
	for n > 0 {
		m := n
		if m > nByte {
			m = nByte
		}
		f.buf.Write(padding[0:m])
		n -= m
	}
}

// pad appends b to f.buf, padded on left (w > 0) or right (w < 0 or f.minus).
func (f *fmt) pad(b []byte) {
	if !f.widPresent || f.wid == 0 {
		f.buf.Write(b)
		return
	}
	padding, left, right := f.computePadding(len(b))
	if left > 0 {
		f.writePadding(left, padding)
	}
	f.buf.Write(b)
	if right > 0 {
		f.writePadding(right, padding)
	}
}

// padString appends s to buf, padded on left (w > 0) or right (w < 0 or f.minus).
func (f *fmt) padString(s string) {
	if !f.widPresent || f.wid == 0 {
		f.buf.WriteString(s)
		return
	}
	padding, left, right := f.computePadding(utf8.RuneCountInString(s))
	if left > 0 {
		f.writePadding(left, padding)
	}
	f.buf.WriteString(s)
	if right > 0 {
		f.writePadding(right, padding)
	}
}

var (
	trueBytes  = []byte("true")
	falseBytes = []byte("false")
)

// fmt_boolean formats a boolean.
func (f *fmt) fmt_boolean(v bool) {
	if v {
		f.pad(trueBytes)
	} else {
		f.pad(falseBytes)
	}
}

// integer; interprets prec but not wid.  Once formatted, result is sent to pad()
// and then flags are cleared.
func (f *fmt) integer(a int64, base uint64, signedness bool, digits string) {
	// precision of 0 and value of 0 means "print nothing"
	if f.precPresent && f.prec == 0 && a == 0 {
		return
	}

	var buf []byte = f.intbuf[0:]
	if f.widPresent && f.wid > nByte {
		// We're going to need a bigger boat.
		buf = make([]byte, f.wid)
	}

	negative := signedness == signed && a < 0
	if negative {
		a = -a
	}

	// two ways to ask for extra leading zero digits: %.3d or %03d.
	// apparently the first cancels the second.
	prec := 0
	if f.precPresent {
		prec = f.prec
		f.zero = false
	} else if f.zero && f.widPresent && !f.minus && f.wid > 0 {
		prec = f.wid
		if negative || f.plus || f.space {
			prec-- // leave room for sign
		}
	}

	// format a into buf, ending at buf[i].  (printing is easier right-to-left.)
	// a is made into unsigned ua.  we could make things
	// marginally faster by splitting the 32-bit case out into a separate
	// block but it's not worth the duplication, so ua has 64 bits.
	i := len(buf)
	ua := uint64(a)
	for ua >= base {
		i--
		buf[i] = digits[ua%base]
		ua /= base
	}
	i--
	buf[i] = digits[ua]
	for i > 0 && prec > len(buf)-i {
		i--
		buf[i] = '0'
	}

	// Various prefixes: 0x, -, etc.
	if f.sharp {
		switch base {
		case 8:
			if buf[i] != '0' {
				i--
				buf[i] = '0'
			}
		case 16:
			i--
			buf[i] = 'x' + digits[10] - 'a'
			i--
			buf[i] = '0'
		}
	}
	if f.unicode {
		i--
		buf[i] = '+'
		i--
		buf[i] = 'U'
	}

	if negative {
		i--
		buf[i] = '-'
	} else if f.plus {
		i--
		buf[i] = '+'
	} else if f.space {
		i--
		buf[i] = ' '
	}

	// If we want a quoted char for %#U, move the data up to make room.
	if f.unicode && f.uniQuote && a >= 0 && a <= utf8.MaxRune && strconv.IsPrint(rune(a)) {
		runeWidth := utf8.RuneLen(rune(a))
		width := 1 + 1 + runeWidth + 1 // space, quote, rune, quote
		copy(buf[i-width:], buf[i:])   // guaranteed to have enough room.
		i -= width
		// Now put " 'x'" at the end.
		j := len(buf) - width
		buf[j] = ' '
		j++
		buf[j] = '\''
		j++
		utf8.EncodeRune(buf[j:], rune(a))
		j += runeWidth
		buf[j] = '\''
	}

	f.pad(buf[i:])
}

// truncate truncates the string to the specified precision, if present.
func (f *fmt) truncate(s string) string {
	if f.precPresent && f.prec < utf8.RuneCountInString(s) {
		n := f.prec
		for i := range s {
			if n == 0 {
				s = s[:i]
				break
			}
			n--
		}
	}
	return s
}

// fmt_s formats a string.
func (f *fmt) fmt_s(s string) {
	s = f.truncate(s)
	f.padString(s)
}

// fmt_sbx formats a string or byte slice as a hexadecimal encoding of its bytes.
func (f *fmt) fmt_sbx(s string, b []byte, digits string) {
	n := len(b)
	if b == nil {
		n = len(s)
	}
	x := digits[10] - 'a' + 'x'
	// TODO: Avoid buffer by pre-padding.
	var buf []byte
	for i := 0; i < n; i++ {
		if i > 0 && f.space {
			buf = append(buf, ' ')
		}
		if f.sharp {
			buf = append(buf, '0', x)
		}
		var c byte
		if b == nil {
			c = s[i]
		} else {
			c = b[i]
		}
		buf = append(buf, digits[c>>4], digits[c&0xF])
	}
	f.pad(buf)
}

// fmt_sx formats a string as a hexadecimal encoding of its bytes.
func (f *fmt) fmt_sx(s, digits string) {
	f.fmt_sbx(s, nil, digits)
}

// fmt_bx formats a byte slice as a hexadecimal encoding of its bytes.
func (f *fmt) fmt_bx(b []byte, digits string) {
	f.fmt_sbx("", b, digits)
}

// fmt_q formats a string as a double-quoted, escaped Go string constant.
func (f *fmt) fmt_q(s string) {
	s = f.truncate(s)
	var quoted string
	if f.sharp && strconv.CanBackquote(s) {
		quoted = "`" + s + "`"
	} else {
		if f.plus {
			quoted = strconv.QuoteToASCII(s)
		} else {
			quoted = strconv.Quote(s)
		}
	}
	f.padString(quoted)
}

// fmt_qc formats the integer as a single-quoted, escaped Go character constant.
// If the character is not valid Unicode, it will print '\ufffd'.
func (f *fmt) fmt_qc(c int64) {
	var quoted []byte
	if f.plus {
		quoted = strconv.AppendQuoteRuneToASCII(f.intbuf[0:0], rune(c))
	} else {
		quoted = strconv.AppendQuoteRune(f.intbuf[0:0], rune(c))
	}
	f.pad(quoted)
}
