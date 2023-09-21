// Copyright 2012 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// 20230921 重命名 search.go -> replace_search.go

package strings

import "io"

// singleStringReplacer is the implementation that's used when there is only
// one string to replace (and that string has more than one byte).
type singleStringReplacer struct {
	finder *stringFinder
	// value is the new string that replaces that pattern when it's found.
	value string
}

// makeSingleStringReplacer 该结构用于对某个字符串中的部分字符进行替换,
// 之所以叫"singleStringReplacer", 是因为该结构针对的只是 old -> new 一对一替换的场景,
// 其他还有 old1 -> new1, old2 -> new2 等多对多的场景.
//
// 	@param pattern: old 字符串, 原字符串中需要被替换的部分
// 	@param value: new 字符串, 将要替换成的部分
//
// caller:
// 	1. NewReplacer() 只有这一处
func makeSingleStringReplacer(pattern string, value string) *singleStringReplacer {
	return &singleStringReplacer{
		finder: makeStringFinder(pattern),
		value:  value,
	}
}

// Replace 将目标字符串中所有 old 字符替换成 new 字符, 并返回替换后的新字符串.
// 注意: 替换后的字符串是新字符串, 而不是在原字符串上替换.
//
// old 与 new 的值是预设的, 可见 makeSingleStringReplacer() 的 pattern/value参数.
//
func (r *singleStringReplacer) Replace(s string) string {
	var buf []byte
	i, matched := 0, false
	// 循环遍历目标字符串 s(i 表示本次遍历的起始位置, 每次循环只找1处匹配)
	for {
		// 找到本次匹配 pattern 的起始位置与 i 处的距离, 赋值给变量 match.
		//
		// 或者换种说法, 如果把 s[i:] 看作一个新字符串 newStr, next() 从 newStr 中寻找
		// 第1次出现 pattern 字符串的位置 pos. 但对于原字符串 s 来说, 这个位置还需要加上 i.
		pos := r.finder.next(s[i:])
		if pos == -1 {
			// 没找到匹配的位置, 结束
			break
		}
		// 这样 s[i+pos] 就表示 pattern 的起始位置, 而 s[i+pos+len(pattern)] 则
		// 表示匹配的结束位置
		matched = true
		buf = append(buf, s[i:i+pos]...)
		buf = append(buf, r.value...)
		// 更新变量i, 作为下次循环的起始位置
		i += pos + len(r.finder.pattern)
	}
	// 只要找到一处匹配, matched 就会是 true, 如果 for{} 结束都还是 false,
	// 则表示没有匹配到, 直接返回原字符串就行.
	if !matched {
		return s
	}
	buf = append(buf, s[i:]...)
	return string(buf)
}

func (r *singleStringReplacer) WriteString(w io.Writer, s string) (n int, err error) {
	sw := getStringWriter(w)
	var i, wn int
	for {
		match := r.finder.next(s[i:])
		if match == -1 {
			break
		}
		wn, err = sw.WriteString(s[i : i+match])
		n += wn
		if err != nil {
			return
		}
		wn, err = sw.WriteString(r.value)
		n += wn
		if err != nil {
			return
		}
		i += match + len(r.finder.pattern)
	}
	wn, err = sw.WriteString(s[i:])
	n += wn
	return
}

////////////////////////////////////////////////////////////////////////////////

// stringFinder efficiently finds strings in a source text.
// It's implemented using the Boyer-Moore string search algorithm:
// http://en.wikipedia.org/wiki/Boyer-Moore_string_search_algorithm
// http://www.cs.utexas.edu/~moore/publications/fstrpos.pdf
// (note: this aged document uses 1-based indexing)
type stringFinder struct {
	// pattern is the string that we are searching for in the text.
	pattern string

	// badCharSkip[b] contains the distance between the last byte of pattern
	// and the rightmost occurrence of b in pattern.
	// If b is not in pattern, badCharSkip[b] is len(pattern).
	//
	// Whenever a mismatch is found with byte b in the text, we can safely
	// shift the matching frame at least badCharSkip[b] until the next time
	// the matching char could be in alignment.
	badCharSkip [256]int

	// goodSuffixSkip[i] defines how far we can shift the matching frame given
	// that the suffix pattern[i+1:] matches, but the byte pattern[i] does not.
	// There are two cases to consider:
	//
	// 1. The matched suffix occurs elsewhere in pattern (with a different
	// byte preceding it that we might possibly match). In this case, we can
	// shift the matching frame to align with the next suffix chunk. For
	// example, the pattern "mississi" has the suffix "issi" next occurring
	// (in right-to-left order) at index 1, so goodSuffixSkip[3] ==
	// shift+len(suffix) == 3+4 == 7.
	//
	// 2. If the matched suffix does not occur elsewhere in pattern, then the
	// matching frame may share part of its prefix with the end of the
	// matching suffix. In this case, goodSuffixSkip[i] will contain how far
	// to shift the frame to align this portion of the prefix to the
	// suffix. For example, in the pattern "abcxxxabc", when the first
	// mismatch from the back is found to be in position 3, the matching
	// suffix "xxabc" is not found elsewhere in the pattern. However, its
	// rightmost "abc" (at position 6) is a prefix of the whole pattern, so
	// goodSuffixSkip[3] == shift+len(suffix) == 6+5 == 11.
	goodSuffixSkip []int
}

// caller:
// 	1. src/pkg/strings/replace.go -> makeSingleStringReplacer() 只有这一处
func makeStringFinder(pattern string) *stringFinder {
	finder := &stringFinder{
		pattern:        pattern,
		goodSuffixSkip: make([]int, len(pattern)),
	}
	// last 表示 pattern 末尾字符的索引
	//
	// last is the index of the last character in the pattern.
	last := len(pattern) - 1

	// 注意: badCharSkip 是定长数组, 在 finder{} 初始化完成时就可以通过索引写入数据了.
	//
	// Build bad character table.
	// Bytes not in the pattern can skip one pattern's length.
	for i := range finder.badCharSkip {
		finder.badCharSkip[i] = len(pattern)
	}
	// The loop condition is < instead of <= so that the last byte does not
	// have a zero distance to itself.
	// Finding this byte out of place implies that it is not in the last position.
	for i := 0; i < last; i++ {
		// 把 badCharSkip 当作 map 来用, 为 pattern 中的每个字符设置反向查找表,
		// key 为成员字符, value 则为与末尾的距离.
		// 假设 pattern = "abc", i = 0 时, pattern[i] = 'a', 则
		// badCharSkip['a'] = last - i = 2, 因为'a'后面有2个字符.
		//
		// 但这会引来一个问题, 假设 pattern = "aba", 那么 i = 2 时, 又会遇到一次字符'a'.
		finder.badCharSkip[pattern[i]] = last - i
	}

	// Build good suffix table.
	// 第1遍: set each value to the next index which starts a prefix of pattern.
	lastPrefix := last
	for i := last; i >= 0; i-- {
		if HasPrefix(pattern, pattern[i+1:]) {
			lastPrefix = i + 1
		}
		// lastPrefix is the shift, and (last-i) is len(suffix).
		finder.goodSuffixSkip[i] = lastPrefix + last - i
	}
	// 第2遍: find repeats of pattern's suffix starting from the front.
	for i := 0; i < last; i++ {
		lenSuffix := longestCommonSuffix(pattern, pattern[1:i+1])
		if pattern[i-lenSuffix] != pattern[last-lenSuffix] {
			// (last-i) is the shift, and lenSuffix is len(suffix).
			finder.goodSuffixSkip[last-lenSuffix] = lenSuffix + last - i
		}
	}

	return finder
}

func longestCommonSuffix(a, b string) (i int) {
	for ; i < len(a) && i < len(b); i++ {
		if a[len(a)-1-i] != b[len(b)-1-i] {
			break
		}
	}
	return
}

// next 在目标字符串 text 中, 寻找第1次出现 pattern 的位置, 返回其索引.
//
// 	@return: 如果没找到, 则返回-1.
//
// next returns the index in text of the first occurrence of the pattern.
// If the pattern is not found, it returns -1.
func (f *stringFinder) next(text string) int {
	// 用类似于滑动窗口的方式遍历查询.
	//
	// text = "hello world", pattern = "ll"
	// 0:      ll
	// 1:       ll
	// 2:        ll  匹配成功
	i := len(f.pattern) - 1
	for i < len(text) {
		// j 表示 pattern 末尾字符的索引,
		// 下面的 for{} 根据 pattern 从后向前与 text 指定范围进行对比的.
		//
		// Compare backwards from the end until the first unmatching character.
		j := len(f.pattern) - 1
		for j >= 0 && text[i] == f.pattern[j] {
			// 匹配成功, 则双方在窗口内部同步前移(比较前一个字符).
			i--
			j--
		}
		if j < 0 {
			// 完成匹配, 计算位置信息.
			return i + 1 // match
		}
		// 运行到这里, 说明此处未成功匹配, 则窗口后移.
		i += max(f.badCharSkip[text[i]], f.goodSuffixSkip[j])
	}
	return -1
}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}
