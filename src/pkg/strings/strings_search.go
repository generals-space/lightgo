// 本文件于 20230924 从 strings.go 文件中拆分出来, 主要为 strings 库的"查找"类函数.

package strings

import "unicode/utf8"

// primeRK is the prime base used in Rabin-Karp algorithm.
const primeRK = 16777619

// hashstr returns the hash and the appropriate multiplicative
// factor for use in Rabin-Karp algorithm.
func hashstr(sep string) (uint32, uint32) {
	hash := uint32(0)
	for i := 0; i < len(sep); i++ {
		hash = hash*primeRK + uint32(sep[i])

	}
	var pow, sq uint32 = 1, primeRK
	for i := len(sep); i > 0; i >>= 1 {
		if i&1 != 0 {
			pow *= sq
		}
		sq *= sq
	}
	return hash, pow
}

// Count 计算目标 sep 子串在目标字符串 s 中出现的次数.
//
// non-overlapping: 非重叠, 比如 s = "aaa", sep = "aa", 那么 Count() 将返回1.
//
// Count counts the number of non-overlapping instances of sep in s.
func Count(s, sep string) int {
	n := 0
	// special cases
	switch {
	case len(sep) == 0:
		return utf8.RuneCountInString(s) + 1
	case len(sep) == 1:
		// special case worth making fast
		c := sep[0]
		for i := 0; i < len(s); i++ {
			if s[i] == c {
				n++
			}
		}
		return n
	case len(sep) > len(s):
		return 0
	case len(sep) == len(s):
		if sep == s {
			return 1
		}
		return 0
	}

	// ...啥情况才会运行到这里???
	hashsep, pow := hashstr(sep)
	h := uint32(0)
	for i := 0; i < len(sep); i++ {
		h = h*primeRK + uint32(s[i])
	}
	lastmatch := 0
	if h == hashsep && s[:len(sep)] == sep {
		n++
		lastmatch = len(sep)
	}
	for i := len(sep); i < len(s); {
		h *= primeRK
		h += uint32(s[i])
		h -= pow * uint32(s[i-len(sep)])
		i++
		if h == hashsep && lastmatch <= i-len(sep) && s[i-len(sep):i] == sep {
			n++
			lastmatch = i
		}
	}
	return n
}

// Contains 判断字符串 s 是否包含 substr
//
// Contains returns true if substr is within s.
func Contains(s, substr string) bool {
	return Index(s, substr) >= 0
}

// ContainsAny returns true if any Unicode code points in chars are within s.
func ContainsAny(s, chars string) bool {
	return IndexAny(s, chars) >= 0
}

// ContainsRune returns true if the Unicode code point r is within s.
func ContainsRune(s string, r rune) bool {
	return IndexRune(s, r) >= 0
}

// Index 查找 str 在 s 中的索引（str 的第一个字符的索引）, -1表示未找到
//
// Index returns the index of the first instance of sep in s, or -1 if sep is not present in s.
func Index(s, sep string) int {
	n := len(sep)
	switch {
	case n == 0:
		return 0
	case n == 1:
		return IndexByte(s, sep[0])
	case n == len(s):
		if sep == s {
			return 0
		}
		return -1
	case n > len(s):
		return -1
	}
	// Hash sep.
	hashsep, pow := hashstr(sep)
	var h uint32
	for i := 0; i < n; i++ {
		h = h*primeRK + uint32(s[i])
	}
	if h == hashsep && s[:n] == sep {
		return 0
	}
	for i := n; i < len(s); {
		h *= primeRK
		h += uint32(s[i])
		h -= pow * uint32(s[i-n])
		i++
		if h == hashsep && s[i-n:i] == sep {
			return i - n
		}
	}
	return -1
}

// LastIndex 反向查找str在字符串s中出现位置的索引（str 的第一个字符的索引）并返回
//
// 	@param: -1 表示未找到.
//
// LastIndex returns the index of the last instance of sep in s, or -1 if sep is not present in s.
func LastIndex(s, sep string) int {
	n := len(sep)
	if n == 0 {
		return len(s)
	}
	c := sep[0]
	if n == 1 {
		// special case worth making fast
		for i := len(s) - 1; i >= 0; i-- {
			if s[i] == c {
				return i
			}
		}
		return -1
	}
	// n > 1
	for i := len(s) - n; i >= 0; i-- {
		if s[i] == c && s[i:i+n] == sep {
			return i
		}
	}
	return -1
}

// IndexRune returns the index of the first instance of the Unicode code point
// r, or -1 if rune is not present in s.
func IndexRune(s string, r rune) int {
	switch {
	case r < 0x80:
		b := byte(r)
		for i := 0; i < len(s); i++ {
			if s[i] == b {
				return i
			}
		}
	default:
		for i, c := range s {
			if c == r {
				return i
			}
		}
	}
	return -1
}

// IndexAny returns the index of the first instance of any Unicode code point
// from chars in s, or -1 if no Unicode code point from chars is present in s.
func IndexAny(s, chars string) int {
	if len(chars) > 0 {
		for i, c := range s {
			for _, m := range chars {
				if c == m {
					return i
				}
			}
		}
	}
	return -1
}

// LastIndexAny returns the index of the last instance of any Unicode code
// point from chars in s, or -1 if no Unicode code point from chars is
// present in s.
func LastIndexAny(s, chars string) int {
	if len(chars) > 0 {
		for i := len(s); i > 0; {
			rune, size := utf8.DecodeLastRuneInString(s[0:i])
			i -= size
			for _, m := range chars {
				if rune == m {
					return i
				}
			}
		}
	}
	return -1
}

// HasPrefix 判断字符串 s 是否以 prefix 开头
//
// HasPrefix tests whether the string s begins with prefix.
func HasPrefix(s, prefix string) bool {
	return len(s) >= len(prefix) && s[0:len(prefix)] == prefix
}

// HasSuffix 判断字符串 s 是否以 suffix 结尾
//
// HasSuffix tests whether the string s ends with suffix.
func HasSuffix(s, suffix string) bool {
	return len(s) >= len(suffix) && s[len(s)-len(suffix):] == suffix
}
