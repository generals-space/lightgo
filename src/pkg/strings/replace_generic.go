package strings

import "io"

// 	@alg: Trie(读'try')树，前缀树, 又称字典树, 单词查找树，是一种树形结构，哈希树的变种。
// 典型应用是用于统计，排序和保存大量的字符串（但不仅限于字符串），所以经常被搜索引擎系统用于文本词频统计。
//
// trieNode is a node in a lookup trie for prioritized key/value pairs.
// Keys and values may be empty.
// For example, the trie containing keys "ax", "ay", "bcbc", "x" and "xy"
// could have eight nodes:
//
//  n0  -
//  n1  a-
//  n2  .x+
//  n3  .y+
//  n4  b-
//  n5  .cbc+
//  n6  x+
//  n7  .y+
//
// n0 is the root node, and its children are n1, n4 and n6; n1's children are
// n2 and n3; n4's child is n5; n6's child is n7. Nodes n0, n1 and n4 (marked
// with a trailing "-") are partial keys, and nodes n2, n3, n5, n6 and n7
// (marked with a trailing "+") are complete keys.
type trieNode struct {
	// value is the value of the trie node's key/value pair.
	// It is empty if this node is not a complete key.
	value string
	// priority is the priority (higher is more important) of the trie node's
	// key/value pair; keys are not necessarily matched shortest- or longest-
	// first. Priority is positive if this node is a complete key, and zero
	// otherwise. In the example above, positive/zero priorities are marked
	// with a trailing "+" or "-".
	priority int

	// A trie node may have zero, one or more child nodes:
	//  * if the remaining fields are zero, there are no children.
	//  * if prefix and next are non-zero, there is one child in next.
	//  * if table is non-zero, it defines all the children.
	//
	// Prefixes are preferred over tables when there is one child, but the
	// root node always uses a table for lookup efficiency.

	// prefix is the difference in keys between this trie node and the next.
	// In the example above, node n4 has prefix "cbc" and n4's next node is n5.
	// Node n5 has no children and so has zero prefix, next and table fields.
	prefix string
	next   *trieNode

	// table is a lookup table indexed by the next byte in the key, after
	// remapping that byte through genericReplacer.mapping to create a dense
	// index. In the example above, the keys only use 'a', 'b', 'c', 'x' and
	// 'y', which remap to 0, 1, 2, 3 and 4. All other bytes remap to 5, and
	// genericReplacer.tableSize will be 5. Node n0's table will be
	// []*trieNode{ 0:n1, 1:n4, 3:n6 }, where the 0, 1 and 3 are the remapped
	// 'a', 'b' and 'x'.
	table []*trieNode
}

// 	@param key: replace 时的 old 字符串
// 	@param val: 与 key 相对应的 new 字符串
// 	@param priority:
//
// caller:
// 	1. makeGenericReplacer()
func (t *trieNode) add(key, val string, priority int, r *genericReplacer) {
	if key == "" {
		if t.priority == 0 {
			t.value = val
			t.priority = priority
		}
		return
	}

	if t.prefix != "" {
		// Need to split the prefix among multiple nodes.
		var n int // length of the longest common prefix
		for ; n < len(t.prefix) && n < len(key); n++ {
			if t.prefix[n] != key[n] {
				break
			}
		}
		if n == len(t.prefix) {
			t.next.add(key[n:], val, priority, r)
		} else if n == 0 {
			// First byte differs, start a new lookup table here. Looking up
			// what is currently t.prefix[0] will lead to prefixNode, and
			// looking up key[0] will lead to keyNode.
			var prefixNode *trieNode
			if len(t.prefix) == 1 {
				prefixNode = t.next
			} else {
				prefixNode = &trieNode{
					prefix: t.prefix[1:],
					next:   t.next,
				}
			}
			keyNode := new(trieNode)
			t.table = make([]*trieNode, r.tableSize)
			t.table[r.mapping[t.prefix[0]]] = prefixNode
			t.table[r.mapping[key[0]]] = keyNode
			t.prefix = ""
			t.next = nil
			keyNode.add(key[1:], val, priority, r)
		} else {
			// Insert new node after the common section of the prefix.
			next := &trieNode{
				prefix: t.prefix[n:],
				next:   t.next,
			}
			t.prefix = t.prefix[:n]
			t.next = next
			next.add(key[n:], val, priority, r)
		}
	} else if t.table != nil {
		// Insert into existing table.
		m := r.mapping[key[0]]
		if t.table[m] == nil {
			t.table[m] = new(trieNode)
		}
		t.table[m].add(key[1:], val, priority, r)
	} else {
		t.prefix = key
		t.next = new(trieNode)
		t.next.add("", val, priority, r)
	}
}

// caller:
// 	1. genericReplacer.WriteString() 在 Replace() 执行替换时被调用
func (r *genericReplacer) lookup(
	str string, ignoreRoot bool,
) (val string, keylen int, found bool) {
	// Iterate down the trie to the end, and grab the value and keylen with
	// the highest priority.
	bestPriority := 0
	node := &r.root
	n := 0
	for node != nil {
		if node.priority > bestPriority && !(ignoreRoot && node == &r.root) {
			bestPriority = node.priority
			val = node.value
			keylen = n
			found = true
		}

		if str == "" {
			break
		}
		if node.table != nil {
			index := r.mapping[str[0]]
			if int(index) == r.tableSize {
				break
			}
			node = node.table[index]
			str = str[1:]
			n++
		} else if node.prefix != "" && HasPrefix(str, node.prefix) {
			n += len(node.prefix)
			str = str[len(node.prefix):]
			node = node.next
		} else {
			break
		}
	}
	return
}

// genericReplacer is the fully generic algorithm.
// It's used as a fallback when nothing faster can be used.
type genericReplacer struct {
	// 由于单个可见字符可以确定为 ASCII 码(ASCII 表的数量为256),
	// 所以直接用索引来表示 old 字符串中的字符,
	// 如 mapping['a'] = 1, 则表示 mapping[97] = 1.
	// 其实是当作了 map[byte]byte 来用了.
	//
	// mapping maps from key bytes to a dense index for trieNode.table.
	mapping [256]byte

	// tableSize 表示 mapping 表key的数量
	//
	// tableSize is the size of a trie node's lookup table.
	// It is the number of unique key bytes.
	tableSize int

	root trieNode
}

// 	@param oldnew: 为 Replacer{} 对象预设的规则, 成对存在, 如: old1,new1,old2,new2...
//
// caller:
// 	1. NewReplacer() 只有这一处
func makeGenericReplacer(oldnew []string) *genericReplacer {
	r := new(genericReplacer)

	// 遍历 oldnew 列表, 将 old 成员中所有字符都添加到 mapping 表中.
	//
	// Find each byte used, then assign them each an index.
	for i := 0; i < len(oldnew); i += 2 {
		old := oldnew[i]
		for j := 0; j < len(old); j++ {
			r.mapping[old[j]] = 1
		}
	}

	// 遍历 mapping 表, 统计所有出现过的字符数量, 赋值给 tableSize
	for _, b := range r.mapping {
		// 虽然 b 是 byte 类型, 不过当 var b byte = 1 时, int(b) 也还是1.
		r.tableSize += int(b)
	}

	// ...下面这一段有什么意义???
	var index byte
	for i, b := range r.mapping {
		if b == 0 {
			r.mapping[i] = byte(r.tableSize)
		} else {
			r.mapping[i] = index
			index++
		}
	}

	// Ensure root node uses a lookup table (for performance).
	r.root.table = make([]*trieNode, r.tableSize)
	for i := 0; i < len(oldnew); i += 2 {
		r.root.add(oldnew[i], oldnew[i+1], len(oldnew)-i, r)
	}
	return r
}

type appendSliceWriter []byte

// Write writes to the buffer to satisfy io.Writer.
func (w *appendSliceWriter) Write(p []byte) (int, error) {
	*w = append(*w, p...)
	return len(p), nil
}

// WriteString writes to the buffer without string->[]byte->string allocations.
func (w *appendSliceWriter) WriteString(str string) (int, error) {
	*w = append(*w, str...)
	return len(str), nil
}

type stringWriterIface interface {
	WriteString(string) (int, error)
}

type stringWriter struct {
	w io.Writer
}

func (w stringWriter) WriteString(str string) (int, error) {
	return w.w.Write([]byte(str))
}

func getStringWriter(w io.Writer) stringWriterIface {
	sw, ok := w.(stringWriterIface)
	if !ok {
		sw = stringWriter{w}
	}
	return sw
}

func (r *genericReplacer) Replace(str string) string {
	buf := make(appendSliceWriter, 0, len(str))
	r.WriteString(&buf, str)
	return string(buf)
}

func (r *genericReplacer) WriteString(w io.Writer, str string) (n int, err error) {
	sw := getStringWriter(w)
	var last, wn int
	var prevMatchEmpty bool
	for i := 0; i <= len(str); {
		// Ignore the empty match iff the previous loop found the empty match.
		val, keylen, match := r.lookup(str[i:], prevMatchEmpty)
		prevMatchEmpty = match && keylen == 0
		if match {
			wn, err = sw.WriteString(str[last:i])
			n += wn
			if err != nil {
				return
			}
			wn, err = sw.WriteString(val)
			n += wn
			if err != nil {
				return
			}
			i += keylen
			last = i
			continue
		}
		i++
	}
	if last != len(str) {
		wn, err = sw.WriteString(str[last:])
		n += wn
	}
	return
}
