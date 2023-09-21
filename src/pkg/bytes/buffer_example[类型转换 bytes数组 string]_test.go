package bytes_test

import (
	"bytes"
	"fmt"
)

// string -> []byte
func ExampleStringToBytes() {
	var str string

	// 1. 强制转换 []byte(str)
	str = "hello world"
	fmt.Printf("bytes: %+v\n", []byte(str))

	fmt.Println()

	// 2. bytes.Buffer
	str = "hello"
	buf := bytes.NewBufferString(str)
	// 单个字符即可用做字节类型
	buf.WriteByte(' ')
	buf.Write([]byte("world"))

	fmt.Printf("bytes: %+v\n", buf.Bytes())
	fmt.Printf("string: %s\n", buf.String())

	// Output:
	// bytes: [104 101 108 108 111 32 119 111 114 108 100]
	//
	// bytes: [104 101 108 108 111 32 119 111 114 108 100]
	// string: hello world
}

// []byte -> string
func ExampleBytesToString() {
	var byteArray []byte

	// 1. 强制转换 string(bytes)
	byteArray = []byte("hello world")
	fmt.Printf("string: %s\n", string(byteArray))

	fmt.Println()

	// 2. bytes.Buffer
	byteArray = []byte{'h', 'e', 'l', 'l', 'o'}
	buf := bytes.NewBuffer(byteArray)
	// 单个字符即可用做字节类型
	buf.WriteByte(' ')
	buf.Write([]byte("world"))

	fmt.Printf("string: %s\n", buf.String())
	fmt.Printf("bytes: %+v\n", buf.Bytes())

	// Output:
	// string: hello world
	//
	// string: hello world
	// bytes: [104 101 108 108 111 32 119 111 114 108 100]
}
