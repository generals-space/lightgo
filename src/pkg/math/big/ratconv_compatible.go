// 	@compatible: 此文件在 v1.5 版本添加

package big

import (
	"fmt"
	"io"
	"strconv"
)

// scanExponent scans the longest possible prefix of r representing a decimal
// ('e', 'E') or binary ('p') exponent, if any. It returns the exponent, the
// exponent base (10 or 2), or a read or syntax error, if any.
//
//	exponent = ( "E" | "e" | "p" ) [ sign ] digits .
//	sign     = "+" | "-" .
//	digits   = digit { digit } .
//	digit    = "0" ... "9" .
//
// A binary exponent is only permitted if binExpOk is set.
func scanExponent(r io.ByteScanner, binExpOk bool) (exp int64, base int, err error) {
	base = 10

	var ch byte
	if ch, err = r.ReadByte(); err != nil {
		if err == io.EOF {
			err = nil // no exponent; same as e0
		}
		return
	}

	switch ch {
	case 'e', 'E':
		// ok
	case 'p':
		if binExpOk {
			base = 2
			break // ok
		}
		fallthrough // binary exponent not permitted
	default:
		r.UnreadByte()
		return // no exponent; same as e0
	}

	var neg bool
	if neg, err = scanSign(r); err != nil {
		return
	}

	var digits []byte
	if neg {
		digits = append(digits, '-')
	}

	// no need to use nat.scan for exponent digits
	// since we only care about int64 values - the
	// from-scratch scan is easy enough and faster
	for i := 0; ; i++ {
		if ch, err = r.ReadByte(); err != nil {
			if err != io.EOF || i == 0 {
				return
			}
			err = nil
			break // i > 0
		}
		if ch < '0' || '9' < ch {
			if i == 0 {
				r.UnreadByte()
				err = fmt.Errorf("invalid exponent (missing digits)")
				return
			}
			break // i > 0
		}
		digits = append(digits, byte(ch))
	}
	// i > 0 => we have at least one digit

	exp, err = strconv.ParseInt(string(digits), 10, 64)
	return
}
