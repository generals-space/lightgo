// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package strings

import "io"

// 	@implementAt: Replacer{} 结构体
//
// replacer is the interface that a replacement algorithm needs to implement.
type replacer interface {
	Replace(s string) string
	WriteString(w io.Writer, s string) (n int, err error)
}

// byteBitmap represents bytes which are sought(seek的被动, 搜索) for replacement.
// byteBitmap is 256 bits wide, with a bit set for each old byte to be replaced.
type byteBitmap [256 / 32]uint32

func (m *byteBitmap) set(b byte) {
	m[b>>5] |= uint32(1 << (b & 31))
}

////////////////////////////////////////////////////////////////////////////////

// 	@implementOf: replacer 接口
//
// A Replacer replaces a list of strings with replacements.
type Replacer struct {
	r replacer
}

// NewReplacer 创建一个 Replacer{} 对象并返回.
//
// Replacer{} 对象可以看作是预设替换规则的字符串修改器,
// ta的 Replace(s) 函数可以按照预设的规则对目标字符串s进行全局替换.
//
// 	@param oldnew: 为 Replacer{} 对象预设的规则, 成对存在, 如: old1,new1,old2,new2...
//
// 替换规则不同, 处理方案也不同, 核心问题在于: 替换时是在原字符串上操作, 还是新建了一个字符串?
// 目前的方案中, 如果 old 与 new 都是单个字符, 那么就会在原字符串上直接替换, 否则...
//
// 细分场景如下:
//
// 	1. 只有一对替换规则, 且 old > 1个字符, singleStringReplacer{};
// 	2. 没有 1,3,4 的特殊情况, 即最通用的场景, 多字符替换成新的多字符, genericReplacer{};
// 	3. 所有 old/new 参数都为单个字符, 可以在原字符串上完成替换, byteReplacer{};
// 	4. 所有 old 为单字符, 而 new 不是, 即从单字符替换成多字符, byteStringReplacer{};
//
// 	@todo: 其实只要一对 old/new 的长度一致, 就可以在原字符串上完成替换, 不过目前没有这样的实现.
//
// NewReplacer returns a new Replacer from a list of old, new string pairs.
// Replacements are performed in order, without overlapping matches.
func NewReplacer(oldnew ...string) *Replacer {
	// 校验: 替换规则只能成对存在
	if len(oldnew)%2 == 1 {
		panic("strings.NewReplacer: odd argument count")
	}

	// 1. 只有一对替换规则, 所以叫 SingleStringReplacer
	if len(oldnew) == 2 && len(oldnew[0]) > 1 {
		return &Replacer{r: makeSingleStringReplacer(oldnew[0], oldnew[1])}
	}

	// allNewBytes 表示是否所有 old/new 都为单个字符.
	// 如果为 true, 就可以在原字符串上完成替换了.
	allNewBytes := true
	for i := 0; i < len(oldnew); i += 2 {
		if len(oldnew[i]) != 1 {
			// 2. 最通用的场景, 多字符替换成新的多字符, 所以叫 GenericReplacer
			return &Replacer{r: makeGenericReplacer(oldnew)}
		}
		if len(oldnew[i+1]) != 1 {
			allNewBytes = false
		}
	}

	if allNewBytes {
		// 3. 所有 old/new 参数都为单个字符, 所以叫 byteReplacer(byte -> byte)
		// byteReplacer.Replace() 就是在原字符串上直接替换的.
		bb := &byteReplacer{}
		for i := 0; i < len(oldnew); i += 2 {
			o, n := oldnew[i][0], oldnew[i+1][0]
			if bb.old[o>>5]&uint32(1<<(o&31)) != 0 {
				// Later old->new maps do not override previous ones with the same old string.
				continue
			}
			bb.old.set(o)
			bb.new[o] = n
		}
		return &Replacer{r: bb}
	}

	// 4. 所有 old 为单字符, 而 new 不是, 即从单字符替换成多字符,
	// 所以叫 byteStringReplacer(byte -> string)
	bs := &byteStringReplacer{}
	for i := 0; i < len(oldnew); i += 2 {
		o, new := oldnew[i][0], oldnew[i+1]
		if bs.old[o>>5]&uint32(1<<(o&31)) != 0 {
			// Later old->new maps do not override previous ones with the same old string.
			continue
		}
		bs.old.set(o)
		bs.new[o] = []byte(new)
	}
	return &Replacer{r: bs}
}

// Replace returns a copy of s with all replacements performed.
func (r *Replacer) Replace(s string) string {
	return r.r.Replace(s)
}

// WriteString writes s to w with all replacements performed.
func (r *Replacer) WriteString(w io.Writer, s string) (n int, err error) {
	return r.r.WriteString(w, s)
}

////////////////////////////////////////////////////////////////////////////////

// 拆分 genericReplacer{} 结构体到独立的文件.

////////////////////////////////////////////////////////////////////////////////

// 拆分 singleStringReplacer{} 结构体到 replace_search.go

////////////////////////////////////////////////////////////////////////////////

// byteReplacer is the implementation that's used when all the "old"
// and "new" values are single ASCII bytes.
type byteReplacer struct {
	// 原字符串中待替换的字符表, 可视作一个 map
	// old has a bit set for each old byte that should be replaced.
	old byteBitmap

	// 待替换的新字符表, 由于单个可见字符可以确定为 ASCII 码, 所以直接用索引来表示 old 字符,
	// 值为 new 字符. 比如 new['a'] = 'b', 那就是将 new 的第97位设置为 'b'.
	//
	// replacement byte, indexed by old byte.
	// only valid if corresponding old bit is set.
	new [256]byte
}

// Replace 逻辑很简单, 逐个 byte 遍历目标 s, 与 old 表中记录的字符做对比, 如果存在则替换.
// 在原字符串上完成 byte 单字符替换, 只用一次 for{} 循环.
func (r *byteReplacer) Replace(s string) string {
	var buf []byte // lazily allocated
	for i := 0; i < len(s); i++ {
		b := s[i]
		if r.old[b>>5]&uint32(1<<(b&31)) != 0 {
			if buf == nil {
				buf = []byte(s)
			}
			buf[i] = r.new[b]
		}
	}
	if buf == nil {
		return s
	}
	return string(buf)
}

func (r *byteReplacer) WriteString(w io.Writer, s string) (n int, err error) {
	// TODO(bradfitz): use io.WriteString with slices of s, avoiding allocation.
	bufsize := 32 << 10
	if len(s) < bufsize {
		bufsize = len(s)
	}
	buf := make([]byte, bufsize)

	for len(s) > 0 {
		ncopy := copy(buf, s[:])
		s = s[ncopy:]
		for i, b := range buf[:ncopy] {
			if r.old[b>>5]&uint32(1<<(b&31)) != 0 {
				buf[i] = r.new[b]
			}
		}
		wn, err := w.Write(buf[:ncopy])
		n += wn
		if err != nil {
			return n, err
		}
	}
	return n, nil
}

////////////////////////////////////////////////////////////////////////////////

// byteStringReplacer is the implementation that's used when all the
// "old" values are single ASCII bytes but the "new" values vary in
// size.
type byteStringReplacer struct {
	// old has a bit set for each old byte that should be replaced.
	old byteBitmap

	// replacement string, indexed by old byte.
	// only valid if corresponding old bit is set.
	new [256][]byte
}

func (r *byteStringReplacer) Replace(s string) string {
	newSize := 0
	anyChanges := false
	for i := 0; i < len(s); i++ {
		b := s[i]
		if r.old[b>>5]&uint32(1<<(b&31)) != 0 {
			anyChanges = true
			newSize += len(r.new[b])
		} else {
			newSize++
		}
	}
	if !anyChanges {
		return s
	}
	buf := make([]byte, newSize)
	bi := buf
	for i := 0; i < len(s); i++ {
		b := s[i]
		if r.old[b>>5]&uint32(1<<(b&31)) != 0 {
			n := copy(bi[:], r.new[b])
			bi = bi[n:]
		} else {
			bi[0] = b
			bi = bi[1:]
		}
	}
	return string(buf)
}

// WriteString maintains one buffer that's at most 32KB.  The bytes in
// s are enumerated and the buffer is filled.  If it reaches its
// capacity or a byte has a replacement, the buffer is flushed to w.
func (r *byteStringReplacer) WriteString(w io.Writer, s string) (n int, err error) {
	// TODO(bradfitz): use io.WriteString with slices of s instead.
	bufsize := 32 << 10
	if len(s) < bufsize {
		bufsize = len(s)
	}
	buf := make([]byte, bufsize)
	bi := buf[:0]

	for i := 0; i < len(s); i++ {
		b := s[i]
		var new []byte
		if r.old[b>>5]&uint32(1<<(b&31)) != 0 {
			new = r.new[b]
		} else {
			bi = append(bi, b)
		}
		if len(bi) == cap(bi) || (len(bi) > 0 && len(new) > 0) {
			nw, err := w.Write(bi)
			n += nw
			if err != nil {
				return n, err
			}
			bi = buf[:0]
		}
		if len(new) > 0 {
			nw, err := w.Write(new)
			n += nw
			if err != nil {
				return n, err
			}
		}
	}
	if len(bi) > 0 {
		nw, err := w.Write(bi)
		n += nw
		if err != nil {
			return n, err
		}
	}
	return n, nil
}
