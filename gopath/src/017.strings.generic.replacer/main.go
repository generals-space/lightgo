// 	@example_test: 本类型文件的目的在于提供示例, 且尽量实现将文件直接迁移到某个 main.go 文件,
// 然后可以直接执行的程度(要将 ExampleXXX 改成 main()).

package main

import (
	"encoding/json"
	"fmt"
	"unsafe"
)

func main() {
	var oldnew []string
	var str, result string

	oldnew = []string{"flag", "pkg/flag", "fmt", "pkg/fmt", "go", "pkg/go"}
	replacer := makeGenericReplacer(oldnew)

	// 1. tableSize
	fmt.Printf("table size: %d\n", replacer.tableSize)

	// 2. mapping
	for i, num := range replacer.mapping {
		if int(num) == replacer.tableSize {
			continue
		}
		fmt.Printf("%c: %d\n", i, num)
	}

	// 3. TrieNode 前缀树
	
	// replacer.root 的原本类型 trieNode 中, 各字段都是首字母小写, 无法打印.
	// 这里需要先进行强制类型转换, 借助了 unsafe 包.
	root := *(*TrieNode)(unsafe.Pointer(&(replacer.root)))
	// trieTree, _ := json.MarshalIndent(replacer.root, "", "    ")
	trieTree, _ := json.MarshalIndent(root, "", "    ")
	fmt.Printf("trieTree:\n")
	fmt.Printf("%s\n", trieTree)

	// 开始替换
	str = "source code package list: pkg/errors, flag, fmt, go, pkg/hash ..."
	result = replacer.Replace(str)
	fmt.Printf("%s\n", result)
}

// go run *.go
// go build -gcflags='-l -N' -o main.gobin *.go

////////////////////////////////////////////////////////////////////////////////

type TrieNode struct {
	Value    string
	Priority int
	Prefix   string
	Next     *TrieNode
	Table    []*TrieNode
}

// 拷贝自 strings.HasPrefix() 函数.
func HasPrefix(s, prefix string) bool {
	return len(s) >= len(prefix) && s[0:len(prefix)] == prefix
}
