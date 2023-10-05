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

// 	@alg: bitmap 位图, 基本原理就是用一个 bit 位来存放某种状态，适用于大规模数据，
// 但数据的状态又不是很多的情况。通常是用来判断某个数据存是否存在的。
//
// 这里的 byteBitmap 只在 byteReplacer{} 和 byteStringReplacer{} 中被用到,
// 目的在于, 遍历目标字符串 s 中所有字符时, 用来判断当前字符是否需要"被替换".
// (这两种结构体明确被应用于, 被替换字符为单个字符的场景)
//
// 一般来说, 判断某个字符是否存在, 以及ta应该被替换成哪个字符(或字符串), 用 map[byte]string
// 就可以实现. 但是为了空间利用率, 这里选用了 bitmap 位图.
//
// 标准 ASCII 码表为256个字符, 因此需要256个 bit 位来表示, 1表示已存在, 0表示不存在.
// 而这可以 256 / 32 = 16 个 uint32 数字的数组来表示, 如:
// byteBitmap[0] = 31-0  bit表示[00000000000000000000000000000000]
// byteBitmap[1] = 63-32 bit表示[00000000000000000000000000000000]
// byteBitmap[2] = 95-64 bit表示[00000000000000000000000000000000]
// byteBitmap[...]
//
// 在设置/查找某个 char 对应的 bit 位时, 需要先找到该 char 在哪个 uint32 成员中,
// 即同样需要先除以 32, 这就是 "b >> 5" 的作用, 用来找到 byteBitmap 中的成员索引.
// 比如 'a' = 97, ta 的 bit 在 byteBitmap[3] 中, 而 97 / 32 = 97 >> 5 = 3
//
// 接下来就是在 uint32 中找到对应的 bit,
// 对于整除找到索引, 很容易想到另一种常用的配套方案, 即, 取模.
// char % 32 的结果, 就对应在 uint32 的 bit 位中, 右移的位数.
// 还是以'a'为例, 97 % 32 = 97 & 31 = 1, 就是 byteBitmap[3] 的第1个bit(从0开始计数)
//
// byteBitmap[3] = 127-96 [00000000000000000000000000000010]
// 
// byteBitmap represents bytes which are sought(seek的被动, 搜索) for replacement.
// byteBitmap is 256 bits wide, with a bit set for each old byte to be replaced.
type byteBitmap [256 / 32]uint32

// 	@param b: 原字符串中将要被替换的 old 字符.
//
// caller:
// 	1. NewReplacer() 只有这一处
func (m *byteBitmap) set(b byte) {
	// 31 的二进制表示为 11111
	// |= 对目标 b 对应的 bit 设置标志位.
	m[b >> 5] |= uint32(1 << (b & 31))
}

////////////////////////////////////////////////////////////////////////////////

// byteReplacer is the implementation that's used when all the "old" and "new"
// values are single ASCII bytes.
type byteReplacer struct {
	// 原字符串中待替换的字符表, 可视作一个 map
	// old has a bit set for each old byte that should be replaced.
	old byteBitmap

	// 待替换的新字符表, 由于单个可见字符可以确定为 ASCII 码(ASCII 表的数量为256),
	// 所以直接用索引来表示 old 字符, 值为 new 字符. 
	// 比如 new['a'] = 'b', 那就是将 new 的第97位设置为 'b'.
	// 其实是当作了 map[byte]byte 来用了.
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
		if r.old[b >> 5]&uint32(1<<(b&31)) != 0 {
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
			if r.old[b >> 5]&uint32(1<<(b&31)) != 0 {
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
// "old" values are single ASCII bytes but the "new" values vary in size.
type byteStringReplacer struct {
	// old has a bit set for each old byte that should be replaced.
	old byteBitmap

	// replacement string, indexed by old byte.
	// only valid if corresponding old bit is set.
	new [256][]byte
}

// Replace 与 byteReplacer{} 相比, old 字符串同样是单个字符, 但是 new 是无法确定的.
// 因为将要替换进来的新字符长度不为1, 所以不能在原字符串上完成, 需要新建一个字符串.
func (r *byteStringReplacer) Replace(s string) string {
	// 由于替换进来的新字符长度不为1, 但又不清楚 old 字符在原字符串中出现了几次,
	// 无法确认新字符串的长度, 下面第1个 for{} 循环就是计算新字符串的长度, 赋值给 newSize.
	newSize := 0
	anyChanges := false
	// 第1次遍历, 确认目标字符串中是否存在需要被替换的字符, 并计算新字符串的长度.
	// 遍历目标字符串 s 中的所有字符, 如果在 r.old 这个 bitmap 中不存在, 表示不需要替换.
	for i := 0; i < len(s); i++ {
		b := s[i]
		if r.old[b >> 5]&uint32(1<<(b&31)) != 0 {
			anyChanges = true
			newSize += len(r.new[b])
		} else {
			newSize++
		}
	}
	if !anyChanges {
		// 不需要替换, 返回原字符串
		return s
	}

	// 第2次遍历, 对原字符串逐个字符拷贝到新串中.
	// 为新字符串创建定长切片
	buf := make([]byte, newSize)
	bi := buf
	for i := 0; i < len(s); i++ {
		b := s[i]
		if r.old[b >> 5]&uint32(1<<(b&31)) != 0 {
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
		if r.old[b >> 5]&uint32(1<<(b&31)) != 0 {
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
