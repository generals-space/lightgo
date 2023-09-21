package bytes

import (
	"unicode"
	"unicode/utf8"
)

// TrimLeftFunc returns a subslice of s by slicing off all leading UTF-8-encoded
// Unicode code points c that satisfy f(c).
func TrimLeftFunc(s []byte, f func(r rune) bool) []byte {
	i := indexFunc(s, f, false)
	if i == -1 {
		return nil
	}
	return s[i:]
}

// TrimRightFunc returns a subslice of s by slicing off all trailing UTF-8
// encoded Unicode code points c that satisfy f(c).
func TrimRightFunc(s []byte, f func(r rune) bool) []byte {
	i := lastIndexFunc(s, f, false)
	if i >= 0 && s[i] >= utf8.RuneSelf {
		_, wid := utf8.DecodeRune(s[i:])
		i += wid
	} else {
		i++
	}
	return s[0:i]
}

// TrimFunc returns a subslice of s by slicing off all leading and trailing
// UTF-8-encoded Unicode code points c that satisfy f(c).
func TrimFunc(s []byte, f func(r rune) bool) []byte {
	return TrimRightFunc(TrimLeftFunc(s, f), f)
}

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
func TrimPrefix(s, prefix []byte) []byte {
	if HasPrefix(s, prefix) {
		return s[len(prefix):]
	}
	return s
}

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
func TrimSuffix(s, suffix []byte) []byte {
	if HasSuffix(s, suffix) {
		return s[:len(s)-len(suffix)]
	}
	return s
}

////////////////////////////////////////////////////////////////////////////////

func makeCutsetFunc(cutset string) func(r rune) bool {
	return func(r rune) bool {
		for _, c := range cutset {
			if c == r {
				return true
			}
		}
		return false
	}
}

// Trim returns a subslice of s by slicing off all leading and
// trailing UTF-8-encoded Unicode code points contained in cutset.
func Trim(s []byte, cutset string) []byte {
	return TrimFunc(s, makeCutsetFunc(cutset))
}

// TrimLeft returns a subslice of s by slicing off all leading
// UTF-8-encoded Unicode code points contained in cutset.
func TrimLeft(s []byte, cutset string) []byte {
	return TrimLeftFunc(s, makeCutsetFunc(cutset))
}

// TrimRight returns a subslice of s by slicing off all trailing
// UTF-8-encoded Unicode code points that are contained in cutset.
func TrimRight(s []byte, cutset string) []byte {
	return TrimRightFunc(s, makeCutsetFunc(cutset))
}

// TrimSpace returns a subslice of s by slicing off all leading and
// trailing white space, as defined by Unicode.
func TrimSpace(s []byte) []byte {
	return TrimFunc(s, unicode.IsSpace)
}
