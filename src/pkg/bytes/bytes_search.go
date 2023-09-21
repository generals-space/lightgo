package bytes

import "unicode/utf8"

func equalPortable(a, b []byte) bool {
	if len(a) != len(b) {
		return false
	}
	for i, c := range a {
		if c != b[i] {
			return false
		}
	}
	return true
}

// Count counts the number of non-overlapping instances of sep in s.
func Count(s, sep []byte) int {
	n := len(sep)
	if n == 0 {
		return utf8.RuneCount(s) + 1
	}
	if n > len(s) {
		return 0
	}
	count := 0
	c := sep[0]
	i := 0
	t := s[:len(s)-n+1]
	for i < len(t) {
		if t[i] != c {
			o := IndexByte(t[i:], c)
			if o < 0 {
				break
			}
			i += o
		}
		if n == 1 || Equal(s[i:i+n], sep) {
			count++
			i += n
			continue
		}
		i++
	}
	return count
}

// Contains reports whether subslice is within b.
func Contains(b, subslice []byte) bool {
	return Index(b, subslice) != -1
}

// Index returns the index of the first instance of sep in s, or -1 if sep is not present in s.
func Index(s, sep []byte) int {
	n := len(sep)
	if n == 0 {
		return 0
	}
	if n > len(s) {
		return -1
	}
	c := sep[0]
	if n == 1 {
		return IndexByte(s, c)
	}
	i := 0
	t := s[:len(s)-n+1]
	for i < len(t) {
		if t[i] != c {
			o := IndexByte(t[i:], c)
			if o < 0 {
				break
			}
			i += o
		}
		if Equal(s[i:i+n], sep) {
			return i
		}
		i++
	}
	return -1
}

func indexBytePortable(s []byte, c byte) int {
	for i, b := range s {
		if b == c {
			return i
		}
	}
	return -1
}

// LastIndex returns the index of the last instance of sep in s, or -1 if sep is not present in s.
func LastIndex(s, sep []byte) int {
	n := len(sep)
	if n == 0 {
		return len(s)
	}
	c := sep[0]
	for i := len(s) - n; i >= 0; i-- {
		if s[i] == c && (n == 1 || Equal(s[i:i+n], sep)) {
			return i
		}
	}
	return -1
}

// IndexRune interprets s as a sequence of UTF-8-encoded Unicode code points.
// It returns the byte index of the first occurrence in s of the given rune.
// It returns -1 if rune is not present in s.
func IndexRune(s []byte, r rune) int {
	for i := 0; i < len(s); {
		r1, size := utf8.DecodeRune(s[i:])
		if r == r1 {
			return i
		}
		i += size
	}
	return -1
}

// IndexAny interprets s as a sequence of UTF-8-encoded Unicode code points.
// It returns the byte index of the first occurrence in s of any of the Unicode
// code points in chars.  It returns -1 if chars is empty or if there is no code
// point in common.
func IndexAny(s []byte, chars string) int {
	if len(chars) > 0 {
		var r rune
		var width int
		for i := 0; i < len(s); i += width {
			r = rune(s[i])
			if r < utf8.RuneSelf {
				width = 1
			} else {
				r, width = utf8.DecodeRune(s[i:])
			}
			for _, ch := range chars {
				if r == ch {
					return i
				}
			}
		}
	}
	return -1
}

// LastIndexAny interprets s as a sequence of UTF-8-encoded Unicode code
// points.  It returns the byte index of the last occurrence in s of any of
// the Unicode code points in chars.  It returns -1 if chars is empty or if
// there is no code point in common.
func LastIndexAny(s []byte, chars string) int {
	if len(chars) > 0 {
		for i := len(s); i > 0; {
			r, size := utf8.DecodeLastRune(s[0:i])
			i -= size
			for _, ch := range chars {
				if r == ch {
					return i
				}
			}
		}
	}
	return -1
}

// HasPrefix tests whether the byte slice s begins with prefix.
func HasPrefix(s, prefix []byte) bool {
	return len(s) >= len(prefix) && Equal(s[0:len(prefix)], prefix)
}

// HasSuffix tests whether the byte slice s ends with suffix.
func HasSuffix(s, suffix []byte) bool {
	return len(s) >= len(suffix) && Equal(s[len(s)-len(suffix):], suffix)
}
