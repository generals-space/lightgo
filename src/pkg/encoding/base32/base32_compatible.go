package base32

// 	@compatible: 如下变量在 v1.9 版本添加
//
const (
	StdPadding rune = '=' // Standard padding character
	NoPadding  rune = -1  // No padding
)

// 	@compatible: 此方法在 v1.9 版本添加
//
// WithPadding creates a new encoding identical to enc except
// with a specified padding character, or NoPadding to disable padding.
// The padding character must not be '\r' or '\n', must not
// be contained in the encoding's alphabet and must be a rune equal or
// below '\xff'.
func (enc Encoding) WithPadding(padding rune) *Encoding {
	if padding == '\r' || padding == '\n' || padding > 0xff {
		panic("invalid padding")
	}

	for i := 0; i < len(enc.encode); i++ {
		if rune(enc.encode[i]) == padding {
			panic("padding contained in alphabet")
		}
	}

	enc.padChar = padding
	return &enc
}
