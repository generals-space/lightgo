package unsafe_test

import (
	"fmt"
	"unsafe"
)

type Empty struct{}

type Person struct {
	Name   string
	Age    int8
	Gender bool
	Id     int
}

/*

                | byte | byte | byte | byte | byte | byte | byte | byte |
                |      |      |      |      |      |      |      |      |
struct{} start->| Name                                                  |
                |                                                       |
                | Age  |Gender|      |      |      |      |      |      |
                | Id                                                    |<- struct{} end
                | byte | byte | byte | byte | byte | byte | byte | byte |

Name 占了结构体前面部分的16个字节, Age/Gender 各占用一个字节, 而紧随其后的6个字节是空置的.
Id 占用剩余的8个字节, 整个结构体共占用32个字节.

*/

func ExampleXxxOf() {
	person := Person{
		Name:   "general",
		Age:    21,
		Gender: true,
		Id:     123456,
	}
	// unsafe.Sizeof() 取的是类型的大小, 而非实例的大小.
	fmt.Println("Sizeof:")
	// 不论字符串的`len/cap`有多大, Sizeof始终返回16.
	// 因为实际上字符串类型对应一个结构体, 该结构体有两个字段.
	// 第一个字段是指向该字符串的指针, 第二个字段是字符串的长度, 每个字段占8个字节,
	// 但是并不包含指针指向的字符串的内容, 这也就是为什么 Sizeof() 始终返回的是16.
	//
	// ...同样, 任何类型的切片的长度也是固定的...吧???
	fmt.Println(unsafe.Sizeof(person.Name))   // 16
	fmt.Println(unsafe.Sizeof(person.Age))    // 1
	fmt.Println(unsafe.Sizeof(person.Gender)) // 1
	fmt.Println(unsafe.Sizeof(person.Id))     // 8

	// struct{} 类型情况也差不多, 空结构体占用为0. 增加成员字段, Sizeof() 结果也会相应增大.
	empty := Empty{}
	fmt.Println(unsafe.Sizeof(empty))  // 0
	fmt.Println(unsafe.Sizeof(person)) // 32

	// 偏移是指当前字段相对于结构体初始地址偏移的字节数量.
	fmt.Println("Offsetof:")
	fmt.Println(unsafe.Offsetof(person.Name))   // 0
	fmt.Println(unsafe.Offsetof(person.Age))    // 16
	fmt.Println(unsafe.Offsetof(person.Gender)) // 17
	fmt.Println(unsafe.Offsetof(person.Id))     // 24

	fmt.Println("Alignof:")
	fmt.Println(unsafe.Alignof(person.Name))   // 8
	fmt.Println(unsafe.Alignof(person.Age))    // 1
	fmt.Println(unsafe.Alignof(person.Gender)) // 1
	fmt.Println(unsafe.Alignof(person.Id))     // 8

	// Output:
	// Sizeof:
	// 16
	// 1
	// 1
	// 8
	// 0
	// 32
	// Offsetof:
	// 0
	// 16
	// 17
	// 24
	// Alignof:
	// 8
	// 1
	// 1
	// 8
}
