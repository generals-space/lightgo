package sort

// 20230509 新增 byte 排序, 用于变向实现 string 中按字符排序的能力.
//

type ByteSlice []byte
func (p ByteSlice) Len() int           { return len(p) }
func (p ByteSlice) Less(i, j int) bool { return p[i] < p[j] }
func (p ByteSlice) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }
func (p ByteSlice) Sort() { Sort(p) }
func Bytes(a []byte) { Sort(ByteSlice(a)) }
func BytesAreSorted(a []byte) bool { return IsSorted(ByteSlice(a)) }

// String 为目标 a 中的字符进行排序.
//
// 注意: 会为所有合法的 byte 字符排序, 包括各种符号, 空格, 大小写等.
//
// 参考文章
// 	1. [How to replace a letter at a specific index in a string in Go?](https://stackoverflow.com/questions/24893624/how-to-replace-a-letter-at-a-specific-index-in-a-string-in-go)
// 		- 原地替换 string 中字符的方案
// 		- 借鉴了答主 OneOfOne 的方法
func String(a string) string {
	bytes := []byte(a)
	Bytes(bytes)
	return string(bytes)
}

// 选择排序
func String2(a string) string {
	var str string
	var added bool
	for i, _ := range a {
		added = false
		for j, _ := range str {
			// 找到 str 中第1个小于 a[i] 字符的位置, 添加进去.
			if a[i] <= str[j] {
				str = str[:j] + string(a[i]) + str[j:]
				added = true
				break
			}
		}
		if !added {
			str = str + string(a[i])
		}
	}
	return str
}
