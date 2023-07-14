// Copyright 2015 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// 	@compatible: 此文件在 v1.5 版本添加

// This file implements nat-to-string conversion functions.

package big

import (
	"errors"
	"fmt"
	"io"
)

// 	@compatible: 此方法在 v1.5 版本添加
//
// maxPow returns (b**n, n) such that b**n is the largest power b**n <= _M.
// For instance maxPow(10) == (1e19, 19) for 19 decimal digits in a 64bit Word.
// In other words, at most n digits in base b fit into a Word.
// TODO(gri) replace this with a table, generated at build time.
func maxPow(b Word) (p Word, n int) {
	p, n = b, 1 // assuming b <= _M
	for max := _M / b; p <= max; {
		// p == b**n && p <= max
		p *= b
		n++
	}
	// p == b**n && p <= _M
	return
}

// 	@compatible: 此方法在 v1.5 版本添加
//
// pow returns x**n for n > 0, and 1 otherwise.
func pow(x Word, n int) (p Word) {
	// n == sum of bi * 2**i, for 0 <= i < imax, and bi is 0 or 1
	// thus x**n == product of x**(2**i) for all i where bi == 1
	// (Russian Peasant Method for exponentiation)
	p = 1
	for n > 0 {
		if n&1 != 0 {
			p *= x
		}
		x *= x
		n >>= 1
	}
	return
}

// 	@compatible: 此方法在 v1.5 版本添加
//
// hexString returns a hexadecimal representation of x.
// It calls x.string with the charset "0123456789abcdef".
func (x nat) hexString() string {
	return x.string(lowercaseDigits[:16])
}

// 	@compatible: nat.scan() 在 v1.5 版本, 原型发生了变动, 为兼容原本的 scan() 函数,
// 	这里将其重命名为 scan_v1_5()
//
// scan scans the number corresponding to the longest possible prefix
// from r representing an unsigned number in a given conversion base.
// It returns the corresponding natural number res, the actual base b,
// a digit count, and a read or syntax error err, if any.
//
//	number   = [ prefix ] mantissa .
//	prefix   = "0" [ "x" | "X" | "b" | "B" ] .
//      mantissa = digits | digits "." [ digits ] | "." digits .
//	digits   = digit { digit } .
//	digit    = "0" ... "9" | "a" ... "z" | "A" ... "Z" .
//
// Unless fracOk is set, the base argument must be 0 or a value between
// 2 and MaxBase. If fracOk is set, the base argument must be one of
// 0, 2, 10, or 16. Providing an invalid base argument leads to a run-
// time panic.
//
// For base 0, the number prefix determines the actual base: A prefix of
// ``0x'' or ``0X'' selects base 16; if fracOk is not set, the ``0'' prefix
// selects base 8, and a ``0b'' or ``0B'' prefix selects base 2. Otherwise
// the selected base is 10 and no prefix is accepted.
//
// If fracOk is set, an octal prefix is ignored (a leading ``0'' simply
// stands for a zero digit), and a period followed by a fractional part
// is permitted. The result value is computed as if there were no period
// present; and the count value is used to determine the fractional part.
//
// A result digit count > 0 corresponds to the number of (non-prefix) digits
// parsed. A digit count <= 0 indicates the presence of a period (if fracOk
// is set, only), and -count is the number of fractional digits found.
// In this case, the actual value of the scanned number is res * b**count.
//
func (z nat) scan_v1_5(r io.ByteScanner, base int, fracOk bool) (res nat, b, count int, err error) {
	// reject illegal bases
	baseOk := base == 0 ||
		!fracOk && 2 <= base && base <= MaxBase ||
		fracOk && (base == 2 || base == 10 || base == 16)
	if !baseOk {
		panic(fmt.Sprintf("illegal number base %d", base))
	}

	// one char look-ahead
	ch, err := r.ReadByte()
	if err != nil {
		return
	}

	// determine actual base
	b = base
	if base == 0 {
		// actual base is 10 unless there's a base prefix
		b = 10
		if ch == '0' {
			count = 1
			switch ch, err = r.ReadByte(); err {
			case nil:
				// possibly one of 0x, 0X, 0b, 0B
				if !fracOk {
					b = 8
				}
				switch ch {
				case 'x', 'X':
					b = 16
				case 'b', 'B':
					b = 2
				}
				switch b {
				case 16, 2:
					count = 0 // prefix is not counted
					if ch, err = r.ReadByte(); err != nil {
						// io.EOF is also an error in this case
						return
					}
				case 8:
					count = 0 // prefix is not counted
				}
			case io.EOF:
				// input is "0"
				res = z[:0]
				err = nil
				return
			default:
				// read error
				return
			}
		}
	}

	// convert string
	// Algorithm: Collect digits in groups of at most n digits in di
	// and then use mulAddWW for every such group to add them to the
	// result.
	z = z[:0]
	b1 := Word(b)
	bn, n := maxPow(b1) // at most n digits in base b1 fit into Word
	di := Word(0)       // 0 <= di < b1**i < bn
	i := 0              // 0 <= i < n
	dp := -1            // position of decimal point
	for {
		if fracOk && ch == '.' {
			fracOk = false
			dp = count
			// advance
			if ch, err = r.ReadByte(); err != nil {
				if err == io.EOF {
					err = nil
					break
				}
				return
			}
		}

		// convert rune into digit value d1
		var d1 Word
		switch {
		case '0' <= ch && ch <= '9':
			d1 = Word(ch - '0')
		case 'a' <= ch && ch <= 'z':
			d1 = Word(ch - 'a' + 10)
		case 'A' <= ch && ch <= 'Z':
			d1 = Word(ch - 'A' + 10)
		default:
			d1 = MaxBase + 1
		}
		if d1 >= b1 {
			r.UnreadByte() // ch does not belong to number anymore
			break
		}
		count++

		// collect d1 in di
		di = di*b1 + d1
		i++

		// if di is "full", add it to the result
		if i == n {
			z = z.mulAddWW(z, bn, di)
			di = 0
			i = 0
		}

		// advance
		if ch, err = r.ReadByte(); err != nil {
			if err == io.EOF {
				err = nil
				break
			}
			return
		}
	}

	if count == 0 {
		// no digits found
		switch {
		case base == 0 && b == 8:
			// there was only the octal prefix 0 (possibly followed by digits > 7);
			// count as one digit and return base 10, not 8
			count = 1
			b = 10
		case base != 0 || b != 8:
			// there was neither a mantissa digit nor the octal prefix 0
			err = errors.New("syntax error scanning number")
		}
		return
	}
	// count > 0

	// add remaining digits to result
	if i > 0 {
		z = z.mulAddWW(z, pow(b1, i), di)
	}
	res = z.norm()

	// adjust for fraction, if any
	if dp >= 0 {
		// 0 <= dp <= count > 0
		count = dp - count
	}

	return
}
