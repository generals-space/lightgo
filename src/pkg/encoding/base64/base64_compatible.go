package base64

// 	@compatible: 如下变量在 v1.5 版本添加
//
const (
	StdPadding rune = '=' // Standard padding character
	NoPadding  rune = -1  // No padding
)

// 	@compatible: 此变量在 v1.5 版本添加
//
// RawStdEncoding is the standard raw, unpadded base64 encoding,
// as defined in RFC 4648 section 3.2.
// This is the same as StdEncoding but omits padding characters.
var RawStdEncoding = StdEncoding.WithPadding(NoPadding)

// 	@compatible: 此变量在 v1.5 版本添加
//
// URLEncoding is the unpadded alternate base64 encoding defined in RFC 4648.
// It is typically used in URLs and file names.
// This is the same as URLEncoding but omits padding characters.
var RawURLEncoding = URLEncoding.WithPadding(NoPadding)

// 	@compatible: 此方法在 v1.5 版本添加
//
// WithPadding creates a new encoding identical to enc except
// with a specified padding character, or NoPadding to disable padding.
func (enc Encoding) WithPadding(padding rune) *Encoding {
	enc.padChar = padding
	return &enc
}
