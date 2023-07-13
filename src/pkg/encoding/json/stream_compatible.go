package json

// 	@compatible: 此函数在 v1.7 版本初次添加
// 	@todo: 只有函数体, 但 indentPrefix, indentValue 字段没有被使用
//
// SetIndent instructs the encoder to format each subsequent encoded
// value as if indented by the package-level function Indent(dst, src, prefix, indent).
// Calling SetIndent("", "") disables indentation.
func (enc *Encoder) SetIndent(prefix, indent string) {
	enc.indentPrefix = prefix
	enc.indentValue = indent
}

// 	@compatible: 此函数在 v1.7 版本初次添加
// 	@todo: 只有函数体, 但 escapeHTML 字段没有被使用
//
// SetEscapeHTML specifies whether problematic HTML characters
// should be escaped inside JSON quoted strings.
// The default behavior is to escape &, <, and > to \u0026, \u003c, and \u003e
// to avoid certain safety problems that can arise when embedding JSON in HTML.
//
// In non-HTML settings where the escaping interferes with the readability
// of the output, SetEscapeHTML(false) disables this behavior.
func (enc *Encoder) SetEscapeHTML(on bool) {
	enc.escapeHTML = on
}
