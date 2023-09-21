package bufio_test

import (
	"bufio"
	"fmt"
	"strings"
)

func ExamplePeek() {
	strReader := strings.NewReader("hello world")
	reader := bufio.NewReader(strReader)

	// 第1次, 从缓存中读取5个字节
	var byteArray []byte
	var err error
	byteArray, err = reader.Peek(5)
	if err != nil {
		fmt.Println(err)
	}
	fmt.Printf("%s\n", byteArray) // hello world

	fmt.Println()

	// 第2次, 从缓存中读取100个字节
	// 注意: 第1次读出来的5个字节并没有消失, 仍然存在于原缓冲区中.
	byteArray, err = reader.Peek(100)
	if err != nil {
		fmt.Println(err) // EOF
	}
	fmt.Printf("%s\n", byteArray) // hello world

	// Output:
	//
	// hello
	//
	// EOF
	// hello world
}
