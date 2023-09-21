package strings

import (
	"unicode"
	"unicode/utf8"
)

// Map returns a copy of the string s with all its characters modified according
// to the mapping function.
// If mapping returns a negative value, the character is dropped from the string
// with no replacement.
func Map(mapping func(rune) rune, s string) string {
	// In the worst case, the string can grow when mapped, making
	// things unpleasant.  But it's so rare we barge in assuming it's
	// fine.  It could also shrink but that falls out naturally.
	maxbytes := len(s) // length of b
	nbytes := 0        // number of bytes encoded in b
	// The output buffer b is initialized on demand, the first
	// time a character differs.
	var b []byte

	for i, c := range s {
		r := mapping(c)
		if b == nil {
			if r == c {
				continue
			}
			b = make([]byte, maxbytes)
			nbytes = copy(b, s[:i])
		}
		if r >= 0 {
			wid := 1
			if r >= utf8.RuneSelf {
				wid = utf8.RuneLen(r)
			}
			if nbytes+wid > maxbytes {
				// Grow the buffer.
				maxbytes = maxbytes*2 + utf8.UTFMax
				nb := make([]byte, maxbytes)
				copy(nb, b[0:nbytes])
				b = nb
			}
			nbytes += utf8.EncodeRune(b[nbytes:maxbytes], r)
		}
	}
	if b == nil {
		return s
	}
	return string(b[0:nbytes])
}

// ToUpper returns a copy of the string s with all Unicode letters mapped to their upper case.
func ToUpper(s string) string { return Map(unicode.ToUpper, s) }

// ToLower returns a copy of the string s with all Unicode letters mapped to their lower case.
func ToLower(s string) string { return Map(unicode.ToLower, s) }

// ToTitle returns a copy of the string s with all Unicode letters mapped to their title case.
func ToTitle(s string) string { return Map(unicode.ToTitle, s) }

// ToUpperSpecial returns a copy of the string s with all Unicode letters mapped to their
// upper case, giving priority to the special casing rules.
func ToUpperSpecial(_case unicode.SpecialCase, s string) string {
	return Map(func(r rune) rune { return _case.ToUpper(r) }, s)
}

// ToLowerSpecial returns a copy of the string s with all Unicode letters mapped to their
// lower case, giving priority to the special casing rules.
func ToLowerSpecial(_case unicode.SpecialCase, s string) string {
	return Map(func(r rune) rune { return _case.ToLower(r) }, s)
}

// ToTitleSpecial returns a copy of the string s with all Unicode letters mapped to their
// title case, giving priority to the special casing rules.
func ToTitleSpecial(_case unicode.SpecialCase, s string) string {
	return Map(func(r rune) rune { return _case.ToTitle(r) }, s)
}
