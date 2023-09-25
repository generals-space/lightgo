// 	@example_test: 本类型文件的目的在于提供示例, 且尽量实现将文件直接迁移到某个 main.go 文件,
// 然后可以直接执行的程度(要将 ExampleXXX 改成 main()).

package main

import "fmt"

func main() {
	var s string
	var result bool

	s = "()"
	result = checkValidString(s)
	fmt.Printf("result: %t\n", result)
}

func checkValidString(s string) bool {
	var left, right int

	for _, c := range s {
		if c == '(' {
			left += 1
			right += 1
		} else if c == ')' {
			left -= 1
			right -= 1
		} else if c == '*' {
			left -= 1
			right += 1
		}

		if right < 0 {
			return false
		}
	}
	return left <= 0 && right >= 0
}
