package strconv_test

import (
	"fmt"
	"strconv"
)

////////////////////////////////////////////////////////////////////////////////
// 进制转换

func ExampleBaseConvert() {
	// 1. 十进制 -> 其他进制
	str2 := strconv.FormatInt(int64(123456789), 2)
	fmt.Println(str2) // 111010110111100110100010101
	str16 := strconv.FormatInt(int64(123456789), 16)
	fmt.Println(str16) // 75bcd15

	fmt.Println()

	// 2. 其他进制 -> 十进制
	num2, _ := strconv.ParseInt(str2, 2, 64)
	fmt.Printf("%d\n", num2) // 123456789
	num16, _ := strconv.ParseInt(str16, 16, 64)
	fmt.Printf("%d\n", num16) // 123456789

	// Output:
	// 111010110111100110100010101
	// 75bcd15
	//
	// 123456789
	// 123456789
}

////////////////////////////////////////////////////////////////////////////////
// 类型转换, ParseInt, ParseFloat, ParseBool, ParseUint

func ExampleTypeConvert() {
	// 1. 整型 <-> 字符串

	// 整型转字符串只需要用`Sprintf()`方法就可以, 而字符串转整型需要借助`strconv`标准库.
	// Atoi() 与 Itoa() 分别是 ParseInt() 和 FormatInt() 函数的封装.
	var strA, strB string
	strA = "123"
	intA, _ := strconv.Atoi(strA)
	fmt.Printf("%d\n", intA)

	strA = strconv.Itoa(intA)
	fmt.Printf("%s\n", strA)

	fmt.Println()

	// 2. 其他类型 <-> 字符串
	strA = "false"
	strB = "true"

	boolA, _ := strconv.ParseBool(strA)
	boolB, _ := strconv.ParseBool(strB)

	fmt.Printf("%t\n", boolA) // false
	fmt.Printf("%t\n", boolB) // true

	strA = "77.9285731709928"
	strB = "250"

	floatA, _ := strconv.ParseFloat(strA, 64)
	floatB, _ := strconv.ParseFloat(strB, 64)

	fmt.Printf("%f\n", floatA) // 77.928573
	fmt.Printf("%f\n", floatB) // 250.000000

	// 3.
	fmt.Println()

	strA = strconv.FormatFloat(floatA, 'f', 6, 32)
	strB = strconv.FormatFloat(floatB, 'e', 6, 32)
	fmt.Printf("%s\n", strA) // 77.928574
	fmt.Printf("%s\n", strB) // 2.500000e+02

	// Output:
	// 123
	// 123
	//
	// false
	// true
	// 77.928573
	// 250.000000
	//
	// 77.928574
	// 2.500000e+02
}
