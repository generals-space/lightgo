package bufio_test

import (
	"bufio"
	"fmt"
	"io"
	"os"
)

// 文件读取

func ExampleRead() {
	file, err := os.Open("/etc/os-release")
	if err != nil {
		panic(err)
	}
	reader := bufio.NewReader(file)
	for {
		// ReadLine() 按行读取字符, 但返回值 line 中不包含行尾的换行符 '\n'
		line, _, err := reader.ReadLine()
		if err == io.EOF {
			break
		}
		fmt.Printf("%s\n", line)
	}
}
