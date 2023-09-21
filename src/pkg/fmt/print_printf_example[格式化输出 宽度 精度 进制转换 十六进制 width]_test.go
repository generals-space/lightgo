package fmt_test

import "fmt"

////////////////////////////////////////////////////////////////////////////////
// Printf("%x") 格式化输出示例

type point struct{ x, y int }

func ExamplePrintfStruct() {
	var p = point{1, 2}

	fmt.Printf("%v\n", p)  // {1 2} (%v 打印结构体的对象的值...只有值)
	fmt.Printf("%+v\n", p) // {x:1 y:2} (%+v 打印结构体的键和值)
	fmt.Printf("%#v\n", p) // main.point{x:1, y:2} (%#v 打印一个值的Go语法表示方式.)
	fmt.Printf("%T\n", p)  // main.point (%T 打印一个值的数据类型(int, string, bool, map[string][string]等))

	// 使用`%p`输出一个指针的地址
	// fmt.Printf("%p\n", &p) // 0xc0000140b0

	// Output:
	// {1 2}
	// {x:1 y:2}
	// fmt_test.point{x:1, y:2}
	// fmt_test.point
}

func ExamplePrintfOthers() {
	fmt.Printf("%t\n", true)         // true (%t 打印布尔变量值)
	fmt.Printf("%s\n", "\"string\"") // "string" (%s 打印基本的字符串)
	fmt.Printf("%d\n", 123)          // 123 (%d 打印整型)
	fmt.Printf("%c\n", 'a')          // a (%c 打印单个字符)
	fmt.Printf("%c\n", 97)           // a (也可以打印ascii码中某个整型对应的字符)
	fmt.Printf("%f\n", 78.9)         // 78.900000 (%f 浮点型数值, 默认精度为6)

	// %e 和 %E 使用科学计数法来输出整型(好像没什么区别)
	fmt.Printf("%e\n", 123400000.0) // 1.234000e+08
	fmt.Printf("%E\n", 123400000.0) // 1.234000E+08

	// %q 打印像Go源码中那样带双引号的字符串(???)
	fmt.Printf("%q\n", "\"string\"") // "\"string\""

	// Output:
	// true
	// "string"
	// 123
	// a
	// a
	// 78.900000
	// 1.234000e+08
	// 1.234000E+08
	// "\"string\""
}

////////////////////////////////////////////////////////////////////////////////
// 进制转换(十进制转二进制)

func ExampleBaseConvert() {
	fmt.Printf("%b\n", 14)                     // 1110 (%b 打印整型数值的二进制)
	fmt.Printf("%o\n", 14)                     // 16 (%o 打印八进制)
	fmt.Printf("%x\n", 456)                    // 1c8 (%x 打印十六进制)
	fmt.Printf("%#x, %#x, %#x\n", 1, 255, 256) // 0x1, 0xff, 0x100 (添加`#`标记输出`0x`前缀)

	fmt.Println()

	// 使用`0n`(`n`为大于0的整数)可以定义每一位16进制的宽度, 用于保持格式的一致
	fmt.Printf("%02x, %02x\n", 1, 255)           // 01, ff
	fmt.Printf("%02x\n", 256)                    // 100 ...这个好像不是02的锅, 因为单个16进制数最大值为255
	fmt.Printf("%02x\n", []byte{1, 2, 254, 255}) // 0102feff
	// 两种标记可以配合使用
	fmt.Printf("%#02x\n", []int{1, 2, 254, 255}) // [0x01 0x02 0xfe 0xff]

	// 注意: "%#x" 只能用于 Printf(), 不能用于 Scanf(), 因此无法格式化读取`0xff`这样的输入.
	// var age int
	// length, err := fmt.Scanf("%#x", &age)
	// 将会报错: "bad verb '%#' for integer", 还是直接用`0x%x`这种格式吧

	fmt.Println()

	// %x 也可以输出字符串的16进制表示, 每个字符串的字节用两个字符输出
	fmt.Printf("%x\n", "hex this") // 6865782074686973

	// Output:
	// 1110
	// 16
	// 1c8
	// 0x1, 0xff, 0x100
	//
	// 01, ff
	// 100
	// 0102feff
	// [0x01 0x02 0xfe 0xff]
	//
	// 6865782074686973
}

////////////////////////////////////////////////////////////////////////////////
// 输出的宽度和精度.

func ExampleWidth() {
	fmt.Println()

	fmt.Printf("|%6d|%6d|\n", 12, 345)   // |    12|   345| (%后面添加数字来控制输出宽度, 默认情况下为右对齐, 左边补空格)
	fmt.Printf("|%-6d|%-6d|\n", 12, 345) // |12    |345   | (`-` 符号指定左对齐(默认情况下, 输出是右对齐的))
	fmt.Printf("|%06d|%6d|\n", 12, 345)  // |000012|   345| (宽度前加0可以补全(其他字符都不好使)仅限左侧补0, 因为右侧补0相当于改变了数值大小)

	fmt.Println()

	fmt.Printf("|%6.2f|%6.2f|\n", 1.2, 3.45)   // |  1.20|  3.45| (指定浮点数的输出宽度与精度)
	fmt.Printf("|%06.2f|%6.2f|\n", 1.2, 3.45)  // |001.20|  3.45|
	fmt.Printf("|%-6.2f|%-6.2f|\n", 1.2, 3.45) // |1.20  |3.45  | (同样可以使用`-`符号)

	fmt.Println()

	fmt.Printf("|%6s|%6s|\n", "foo", "b")    // |   foo|     b| (指定字符串的宽度(同样默认右对齐))
	fmt.Printf("|%-6s|%-6s|\n", "foo", "b")  // |foo   |b     | (同样可以使用`-`符号)
	fmt.Printf("|%-06s|%-6s|\n", "foo", "b") // |foo   |b     | (字符串是没有补0这一说的)

	// Output:
	//
	// |    12|   345|
	// |12    |345   |
	// |000012|   345|
	//
	// |  1.20|  3.45|
	// |001.20|  3.45|
	// |1.20  |3.45  |
	//
	// |   foo|     b|
	// |foo   |b     |
	// |foo   |b     |
}
