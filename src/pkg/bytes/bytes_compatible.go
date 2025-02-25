package bytes

// 	@compatible: 该函数在 v1.5 版本初次添加
//
// LastIndexByte returns the index of the last instance of c in s, or -1 if c is not present in s.
func LastIndexByte(s []byte, c byte) int {
	for i := len(s) - 1; i >= 0; i-- {
		if s[i] == c {
			return i
		}
	}
	return -1
}

// 	@compatible: 该函数在 v1.7 版本初次添加
//
// ContainsAny reports whether any of the UTF-8-encoded Unicode code points in chars are within b.
func ContainsAny(b []byte, chars string) bool {
	return IndexAny(b, chars) >= 0
}
