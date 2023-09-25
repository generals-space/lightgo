// 20231007 拆分 format.go 到独立的文件, 主要包含 complex 类型的格式化函数

package fmt

// fmt_c64 formats a complex64 according to the verb.
func (f *fmt) fmt_c64(v complex64, verb rune) {
	f.buf.WriteByte('(')
	r := real(v)
	oldPlus := f.plus
	for i := 0; ; i++ {
		switch verb {
		case 'b':
			f.fmt_fb32(r)
		case 'e':
			f.fmt_e32(r)
		case 'E':
			f.fmt_E32(r)
		case 'f':
			f.fmt_f32(r)
		case 'g':
			f.fmt_g32(r)
		case 'G':
			f.fmt_G32(r)
		}
		if i != 0 {
			break
		}
		f.plus = true
		r = imag(v)
	}
	f.plus = oldPlus
	f.buf.Write(irparenBytes)
}

// fmt_c128 formats a complex128 according to the verb.
func (f *fmt) fmt_c128(v complex128, verb rune) {
	f.buf.WriteByte('(')
	r := real(v)
	oldPlus := f.plus
	for i := 0; ; i++ {
		switch verb {
		case 'b':
			f.fmt_fb64(r)
		case 'e':
			f.fmt_e64(r)
		case 'E':
			f.fmt_E64(r)
		case 'f':
			f.fmt_f64(r)
		case 'g':
			f.fmt_g64(r)
		case 'G':
			f.fmt_G64(r)
		}
		if i != 0 {
			break
		}
		f.plus = true
		r = imag(v)
	}
	f.plus = oldPlus
	f.buf.Write(irparenBytes)
}
