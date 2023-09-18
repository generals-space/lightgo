// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package filepath

import (
	"errors"
	"os"
	"runtime"
	"sort"
	"strings"
	"unicode/utf8"
)

// ErrBadPattern indicates a globbing pattern was malformed.
var ErrBadPattern = errors.New("syntax error in pattern")

// Match returns true if name matches the shell file name pattern.
// The pattern syntax is:
//
//	pattern:
//		{ term }
//	term:
//		'*'         matches any sequence of non-Separator characters
//		'?'         matches any single non-Separator character
//		'[' [ '^' ] { character-range } ']'
//		            character class (must be non-empty)
//		c           matches character c (c != '*', '?', '\\', '[')
//		'\\' c      matches character c
//
//	character-range:
//		c           matches character c (c != '\\', '-', ']')
//		'\\' c      matches character c
//		lo '-' hi   matches character c for lo <= c <= hi
//
// Match requires pattern to match all of name, not just a substring.
// The only possible returned error is ErrBadPattern, when pattern is malformed.
//
// On Windows, escaping is disabled. Instead, '\\' is treated as path separator.
//
func Match(pattern, name string) (matched bool, err error) {
Pattern:
	for len(pattern) > 0 {
		var star bool
		var chunk string
		star, chunk, pattern = scanChunk(pattern)
		// 如果找到了 star, 但 chunk 为空, 表示 * 号在 pattern[0] 的位置
		if star && chunk == "" {
			// Trailing * matches rest of string unless it has a /.
			return strings.Index(name, string(Separator)) < 0, nil
		}
		// 用不包含 * 号的 chunk 部分, 与 name 进行匹配.
		// 匹配过程中双方会同步消除, 直到 chunk 被清空, rest 为 name 中剩余的部分.
		//
		// Look for match at current position.
		rest, ok, err := matchChunk(chunk, name)

		// 如果上面的 matchChunk() 匹配流程成功完成, 且满足下面任一条件:
		// 1. 剩余的 name (即 rest)已经完全消除
		// 2. pattern 还有剩余
		//
		// if we're the last chunk, make sure we've exhausted the name
		// otherwise we'll give a false result even if we could still match
		// using the star
		if ok && (len(rest) == 0 || len(pattern) > 0) {
			name = rest
			continue
		}
		if err != nil {
			return false, err
		}

		// 运行到这里, 说明 ok == false, 上面的 matchChunk(chunk, name) 匹配失败.
		// 但如果 star == true, 则可以跳过不匹配的部分, 下面 if{} 块中的 for{} 循环,
		// 就是从 name[i+1:], 继续匹配, 这里 name[i] 就是用 * 号跳过的部分.
		if star {
			// 注意: * 号不能匹配斜杠字符, 即 for{} 循环中的"name[i] != Separator"语句
			//
			// Look for match skipping i+1 bytes.
			// Cannot skip /.
			for i := 0; i < len(name) && name[i] != Separator; i++ {
				rest, ok, err := matchChunk(chunk, name[i+1:])
				if ok {
					// if we're the last chunk, make sure we exhausted the name
					if len(pattern) == 0 && len(rest) > 0 {
						continue
					}
					name = rest
					continue Pattern
				}
				if err != nil {
					return false, err
				}
			}
		}
		// 运行到这里, 就没希望了.
		return false, nil
	}
	return len(name) == 0, nil
}

// scanChunk 找到 pattern 中第1个"*"号, 并按该其索引将原 pattern 切割成 chunk, rest 两个部分.
//
// 	@return star: 是否找到了 * 号
// 	@return chunk: 原 pattern 中第1个*号前的部分, 比如 pattern = "a*/b", chunk = "a"
// 	@return rest: 原 pattern 中第1个*号后的部分, 比如 pattern = "a*/b", rest = "*/b"
//
// scanChunk gets the next segment of pattern, which is a non-star string
// possibly preceded by a star.
func scanChunk(pattern string) (star bool, chunk, rest string) {
	for len(pattern) > 0 && pattern[0] == '*' {
		pattern = pattern[1:]
		star = true
	}
	inrange := false
	var i int
Scan:
	for i = 0; i < len(pattern); i++ {
		switch pattern[i] {
		case '\\':
			if runtime.GOOS != "windows" {
				// error check handled in matchChunk: bad pattern.
				if i+1 < len(pattern) {
					i++
				}
			}
		case '[':
			inrange = true
		case ']':
			inrange = false
		case '*':
			if !inrange {
				break Scan
			}
		}
	}
	return star, pattern[0:i], pattern[i:]
}

// matchChunk 对 chunk 部分与 name 字符串进行匹配, 直到把 chunk 消耗完毕.
//
// 	@param chunk: 原 pattern 中第1个*号前的部分, 比如 pattern = "a*/b", chunk = "a";
// 	即, 传入的 chunk 参数中是不会出现*号的.
// 	@paarm name: 待匹配的路径字符串
//
// 	@return rest: 匹配完成后, name 剩余的部分
// 	@return ok: 是否正常完成匹配流程
//
// matchChunk checks whether chunk matches the beginning of name.
// If so, it returns the remainder of name (after the match).
// Chunk is all single-character operators: literals, char classes, and ?.
func matchChunk(chunk, name string) (rest string, ok bool, err error) {
	for len(chunk) > 0 {
		// 	@todo: chunk 不为空, 但是 name 被消耗光了, 会发生什么?
		if len(name) == 0 {
			return
		}
		switch chunk[0] {
		case '[':
			// character class
			r, n := utf8.DecodeRuneInString(name)
			name = name[n:]
			chunk = chunk[1:]
			// We can't end right after '[', we're expecting at least
			// a closing bracket and possibly a caret.
			if len(chunk) == 0 {
				err = ErrBadPattern
				return
			}
			// possibly negated
			negated := chunk[0] == '^'
			if negated {
				chunk = chunk[1:]
			}
			// parse all ranges
			match := false
			nrange := 0
			for {
				if len(chunk) > 0 && chunk[0] == ']' && nrange > 0 {
					chunk = chunk[1:]
					break
				}
				var lo, hi rune
				if lo, chunk, err = getEsc(chunk); err != nil {
					return
				}
				hi = lo
				if chunk[0] == '-' {
					if hi, chunk, err = getEsc(chunk[1:]); err != nil {
						return
					}
				}
				if lo <= r && r <= hi {
					match = true
				}
				nrange++
			}
			if match == negated {
				return
			}

		case '?':
			if name[0] == Separator {
				return
			}
			_, n := utf8.DecodeRuneInString(name)
			name = name[n:]
			chunk = chunk[1:]

		case '\\':
			// 转义字符
			if runtime.GOOS != "windows" {
				chunk = chunk[1:]
				if len(chunk) == 0 {
					err = ErrBadPattern
					return
				}
			}
			fallthrough

		default:
			// 非 *, ? 等特殊字符, 需要一对一进行匹配.
			if chunk[0] != name[0] {
				return
			}
			// 普通字符匹配成功后, chunk 和 name 分别右移一位
			name = name[1:]
			chunk = chunk[1:]
		}
	}
	return name, true, nil
}

// getEsc gets a possibly-escaped character from chunk, for a character class.
func getEsc(chunk string) (r rune, nchunk string, err error) {
	if len(chunk) == 0 || chunk[0] == '-' || chunk[0] == ']' {
		err = ErrBadPattern
		return
	}
	if chunk[0] == '\\' && runtime.GOOS != "windows" {
		chunk = chunk[1:]
		if len(chunk) == 0 {
			err = ErrBadPattern
			return
		}
	}
	r, n := utf8.DecodeRuneInString(chunk)
	if r == utf8.RuneError && n == 1 {
		err = ErrBadPattern
	}
	nchunk = chunk[n:]
	if len(nchunk) == 0 {
		err = ErrBadPattern
	}
	return
}

// Glob returns the names of all files matching pattern or nil
// if there is no matching file. The syntax of patterns is the same
// as in Match. The pattern may describe hierarchical names such as
// /usr/*/bin/ed (assuming the Separator is '/').
//
func Glob(pattern string) (matches []string, err error) {
	if !hasMeta(pattern) {
		if _, err = os.Stat(pattern); err != nil {
			return nil, nil
		}
		return []string{pattern}, nil
	}

	dir, file := Split(pattern)
	switch dir {
	case "":
		dir = "."
	case string(Separator):
		// nothing
	default:
		dir = dir[0 : len(dir)-1] // chop off trailing separator
	}

	if !hasMeta(dir) {
		return glob(dir, file, nil)
	}

	var m []string
	m, err = Glob(dir)
	if err != nil {
		return
	}
	for _, d := range m {
		matches, err = glob(d, file, matches)
		if err != nil {
			return
		}
	}
	return
}

// glob searches for files matching pattern in the directory dir
// and appends them to matches. If the directory cannot be
// opened, it returns the existing matches. New matches are
// added in lexicographical order.
func glob(dir, pattern string, matches []string) (m []string, e error) {
	m = matches
	fi, err := os.Stat(dir)
	if err != nil {
		return
	}
	if !fi.IsDir() {
		return
	}
	d, err := os.Open(dir)
	if err != nil {
		return
	}
	defer d.Close()

	names, err := d.Readdirnames(-1)
	if err != nil {
		return
	}
	sort.Strings(names)

	for _, n := range names {
		matched, err := Match(pattern, n)
		if err != nil {
			return m, err
		}
		if matched {
			m = append(m, Join(dir, n))
		}
	}
	return
}

// hasMeta returns true if path contains any of the magic characters
// recognized by Match.
func hasMeta(path string) bool {
	// TODO(niemeyer): Should other magic characters be added here?
	return strings.IndexAny(path, "*?[") >= 0
}
