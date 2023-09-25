// 20231007 拆分 format.go 到独立的文件, 主要包含 float 类型的格式化函数

package fmt

import "strconv"

// floating-point

func doPrec(f *fmt, def int) int {
	if f.precPresent {
		return f.prec
	}
	return def
}

// formatFloat formats a float64; it is an efficient equivalent to  f.pad(strconv.FormatFloat()...).
func (f *fmt) formatFloat(v float64, verb byte, prec, n int) {
	// We leave one byte at the beginning of f.intbuf for a sign if needed,
	// and make it a space, which we might be able to use.
	f.intbuf[0] = ' '
	slice := strconv.AppendFloat(f.intbuf[0:1], v, verb, prec, n)
	// Add a plus sign or space to the floating-point string representation if missing and required.
	// The formatted number starts at slice[1].
	switch slice[1] {
	case '-', '+':
		// If we're zero padding, want the sign before the leading zeros.
		// Achieve this by writing the sign out and padding the postive number.
		if f.zero && f.widPresent && f.wid > len(slice) {
			f.buf.WriteByte(slice[1])
			f.wid--
			f.pad(slice[2:])
			return
		}
		// We're set; drop the leading space.
		slice = slice[1:]
	default:
		// There's no sign, but we might need one.
		if f.plus {
			slice[0] = '+'
		} else if f.space {
			// space is already there
		} else {
			slice = slice[1:]
		}
	}
	f.pad(slice)
}

// fmt_e64 formats a float64 in the form -1.23e+12.
func (f *fmt) fmt_e64(v float64) { f.formatFloat(v, 'e', doPrec(f, 6), 64) }

// fmt_E64 formats a float64 in the form -1.23E+12.
func (f *fmt) fmt_E64(v float64) { f.formatFloat(v, 'E', doPrec(f, 6), 64) }

// fmt_f64 formats a float64 in the form -1.23.
func (f *fmt) fmt_f64(v float64) { f.formatFloat(v, 'f', doPrec(f, 6), 64) }

// fmt_g64 formats a float64 in the 'f' or 'e' form according to size.
func (f *fmt) fmt_g64(v float64) { f.formatFloat(v, 'g', doPrec(f, -1), 64) }

// fmt_G64 formats a float64 in the 'f' or 'E' form according to size.
func (f *fmt) fmt_G64(v float64) { f.formatFloat(v, 'G', doPrec(f, -1), 64) }

// fmt_fb64 formats a float64 in the form -123p3 (exponent is power of 2).
func (f *fmt) fmt_fb64(v float64) { f.formatFloat(v, 'b', 0, 64) }

// float32
// cannot defer to float64 versions
// because it will get rounding wrong in corner cases.

// fmt_e32 formats a float32 in the form -1.23e+12.
func (f *fmt) fmt_e32(v float32) { f.formatFloat(float64(v), 'e', doPrec(f, 6), 32) }

// fmt_E32 formats a float32 in the form -1.23E+12.
func (f *fmt) fmt_E32(v float32) { f.formatFloat(float64(v), 'E', doPrec(f, 6), 32) }

// fmt_f32 formats a float32 in the form -1.23.
func (f *fmt) fmt_f32(v float32) { f.formatFloat(float64(v), 'f', doPrec(f, 6), 32) }

// fmt_g32 formats a float32 in the 'f' or 'e' form according to size.
func (f *fmt) fmt_g32(v float32) { f.formatFloat(float64(v), 'g', doPrec(f, -1), 32) }

// fmt_G32 formats a float32 in the 'f' or 'E' form according to size.
func (f *fmt) fmt_G32(v float32) { f.formatFloat(float64(v), 'G', doPrec(f, -1), 32) }

// fmt_fb32 formats a float32 in the form -123p3 (exponent is power of 2).
func (f *fmt) fmt_fb32(v float32) { f.formatFloat(float64(v), 'b', 0, 32) }
