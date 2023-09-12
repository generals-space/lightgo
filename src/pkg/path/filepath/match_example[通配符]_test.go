package filepath_test

import (
	"fmt"
	"path/filepath"
)

func ExampleMatch() {
	var pattern, path string
	var result bool
	var err error

	pattern = "a*/b"
	path = "abc/b"
	result, err = filepath.Match(pattern, path)
	if err != nil {
		fmt.Printf("匹配异常: %s", err)
		return
	}
	fmt.Printf("result: %t\n", result)

	pattern = "a*/b"
	path = "a/c/b"
	result, err = filepath.Match(pattern, path)
	if err != nil {
		fmt.Printf("匹配异常: %s", err)
		return
	}
	fmt.Printf("result: %t\n", result)

	// Output:
	// result: true
	// result: false
}
