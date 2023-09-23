/*

参考文章

1. [9.4 精密计算和 big 包](http://wiki.jikexueyuan.com/project/the-way-to-go/09.4.html)

现代操作系统中, 整型的最大位数为64位, 相当于8个`byte`.

在进行`lorawan`开发时, 设备key为`[16]byte`的16进制数值, 已经超出了系统所能表示的极限(int64只有8个字节), 所以找到了这个库.

按照参考文章1中所说, big库其实是用于高精度计算的, 目前没有找到相关示例, 这篇文章简单介绍大数计算.

*/
package big_test

import (
	"fmt"
	"math/big"
)

// 命令行执行可用:
// 	1. go test -timeout 30s -run "^(ExampleInit|ExampleCompute|ExampleCompare|ExamplePrint)$" -v
// 	2. go test -v -run ./int_example\[大数\ 计算\ 比较\]_test.go

////////////////////////////////////////////////////////////////////////////////
// 大数定义与打印输出

func ExampleInit() {
	// 1. 定义一个简单的大数.
	simpleBig := big.NewInt(1000)
	fmt.Printf("type: %T, value: %s\n", simpleBig, simpleBig) // *big.Int, 1000

	// 简单整型没什么意思,
	// 如果想表示一个值为`FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF`的32位16进制整型, 怎么办?
	//
	// 常规整型会溢出, 编译都无法通过.
	// var a int64
	// constant 340282366920938463463374607431768211455 overflows int64
	// a = 340282366920938463463374607431768211455

	// 但单纯使用字符串表示数值虽然可以, 但之后涉及计算的部分就无能为力了.
	// `big`库提供了解决办法, 大的数据可以通过字符串, 或是数组表示, `big`允许从这些结构中读入, 然后构造成大数类型. 如下

	// 2. 典型大数
	// 首先需要得到一个big int的结构, 以下两种初始化方式都可以
	baseBig := new(big.Int)
	// baseBig := big.NewInt(0)

	// SetString()和SetBytes()可以对 BigInt 类型赋值
	big1, _ := baseBig.SetString("340282366920938463463374607431768211455", 10)
	fmt.Printf("big1: %s\n", big1)
	big2, _ := baseBig.SetString("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16)
	fmt.Printf("big2: %s\n", big2)
	big3 := baseBig.SetBytes([]byte{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	})
	fmt.Printf("big3: %s\n", big3)

	// Output:
	// type: *big.Int, value: 1000
	// big1: 340282366920938463463374607431768211455
	// big2: 340282366920938463463374607431768211455
	// big3: 340282366920938463463374607431768211455
}

////////////////////////////////////////////////////////////////////////////////
// 大数计算(加减乘除)
//
// BigInt 类型的数据无法直接通过`+`, `-`, `*`, `/`进行计算, 而是提供类似于`time.Duration`的计算方式.
//
// 计算的方法包括
//
// - `Add func(x, y *big.Int) *big.Int`: 加
// - `Sub func(x, y *big.Int) *big.Int`: 减
// - `Mul func(x, y *big.Int) *big.Int`: 乘
// - `Div func(x, y *big.Int) *big.Int`: 除
// - ...
//

func ExampleCompute() {
	baseBig := big.NewInt(0)
	bigInt, _ := baseBig.SetString("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16)
	fmt.Printf("%s\n", bigInt) // 340282366920938463463374607431768211455
	// 减1操作
	result1 := big.NewInt(0).Sub(bigInt, big.NewInt(1))
	fmt.Printf("%s\n", result1) // 340282366920938463463374607431768211454
	result2 := big.NewInt(0).Sub(bigInt, big.NewInt(10))
	fmt.Printf("%s\n", result2) // 340282366920938463463374607431768211445

	// Output:
	// 340282366920938463463374607431768211455
	// 340282366920938463463374607431768211454
	// 340282366920938463463374607431768211445
}

////////////////////////////////////////////////////////////////////////////////
// 大数比较

func ExampleCompare() {
	baseBig1 := big.NewInt(0)
	big1, _ := baseBig1.SetString("340282366920938463463374607431768211455", 10)

	baseBig2 := big.NewInt(0)
	big2, _ := baseBig2.SetString("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16)

	// 本来两者是相等的
	fmt.Printf("%d\n", big1.Cmp(big2)) // big1 == big2 0

	big1 = big.NewInt(0).Sub(big1, big.NewInt(1))

	fmt.Printf("%d\n", big1.Cmp(big2)) // -1 表示当前 BigInt 小于目标对象, 即 big1 < big2 -1
	fmt.Printf("%d\n", big2.Cmp(big1)) // 1 表示当前 BigInt 大于目标对象, 即 big2 > big1

	// Output:
	// 0
	// -1
	// 1
}

////////////////////////////////////////////////////////////////////////////////
// 大数展示, 打印

func ExamplePrint() {
	// BigInt 类型的值可以转换为其他类型, 比如`bytes(数组)`, `int64(不能超过上限)`, 或是`string`.

	baseBig := big.NewInt(0)
	bigInt, _ := baseBig.SetString("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16)
	fmt.Println(bigInt) // 340282366920938463463374607431768211455
	result := big.NewInt(0).Sub(bigInt, big.NewInt(1))
	fmt.Println(result)          // 340282366920938463463374607431768211454
	fmt.Println(result.String()) // 340282366920938463463374607431768211454
	fmt.Println(result.Bytes())  // [255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 254]

	fmt.Println(result.Int64()) // -2

	// Output:
	// 340282366920938463463374607431768211455
	// 340282366920938463463374607431768211454
	// 340282366920938463463374607431768211454
	// [255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 254]
	// -2
}
