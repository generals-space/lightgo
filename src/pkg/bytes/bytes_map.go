package bytes

import (
	"unicode"
	"unicode/utf8"
)

// Map returns a copy of the byte slice s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the string with no replacement.  The characters in s and the
// output are interpreted as UTF-8-encoded Unicode code points.
func Map(mapping func(r rune) rune, s []byte) []byte {
	// In the worst case, the slice can grow when mapped, making
	// things unpleasant.  But it's so rare we barge in assuming it's
	// fine.  It could also shrink but that falls out naturally.
	maxbytes := len(s) // length of b
	nbytes := 0        // number of bytes encoded in b
	b := make([]byte, maxbytes)
	for i := 0; i < len(s); {
		wid := 1
		r := rune(s[i])
		if r >= utf8.RuneSelf {
			r, wid = utf8.DecodeRune(s[i:])
		}
		r = mapping(r)
		if r >= 0 {
			if nbytes+utf8.RuneLen(r) > maxbytes {
				// Grow the buffer.
				maxbytes = maxbytes*2 + utf8.UTFMax
				nb := make([]byte, maxbytes)
				copy(nb, b[0:nbytes])
				b = nb
			}
			nbytes += utf8.EncodeRune(b[nbytes:maxbytes], r)
		}
		i += wid
	}
	return b[0:nbytes]
}

// ToUpper returns a copy of the byte slice s with all Unicode letters mapped to their upper case.
func ToUpper(s []byte) []byte { return Map(unicode.ToUpper, s) }

// ToLower returns a copy of the byte slice s with all Unicode letters mapped to their lower case.
func ToLower(s []byte) []byte { return Map(unicode.ToLower, s) }

// ToTitle returns a copy of the byte slice s with all Unicode letters mapped to their title case.
func ToTitle(s []byte) []byte { return Map(unicode.ToTitle, s) }

// ToUpperSpecial returns a copy of the byte slice s with all Unicode letters mapped to their
// upper case, giving priority to the special casing rules.
func ToUpperSpecial(_case unicode.SpecialCase, s []byte) []byte {
	return Map(func(r rune) rune { return _case.ToUpper(r) }, s)
}

// ToLowerSpecial returns a copy of the byte slice s with all Unicode letters mapped to their
// lower case, giving priority to the special casing rules.
func ToLowerSpecial(_case unicode.SpecialCase, s []byte) []byte {
	return Map(func(r rune) rune { return _case.ToLower(r) }, s)
}

// ToTitleSpecial returns a copy of the byte slice s with all Unicode letters mapped to their
// title case, giving priority to the special casing rules.
func ToTitleSpecial(_case unicode.SpecialCase, s []byte) []byte {
	return Map(func(r rune) rune { return _case.ToTitle(r) }, s)
}
