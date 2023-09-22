package runtime_test

import (
	"fmt"
	"runtime"
)

// 本文件中的 Example() 函数会执行失败, 因为所在文件不同, 文件名与行号信息都有差异.
// 需要将 Example() 放到 main() 函数中执行, 才能获取真实的结果.

func ExampleCaller() {
	foo()
	// Output:
}
func foo() {
	// This is: main.foo, pc: 17385455, file: /usr/local/go/gopath/src/test/main.go, line: 13, ok: bool
	printFuncInfo()
	bar()
}
func bar() {
	// This is: main.bar, pc: 17385461, file: /usr/local/go/gopath/src/test/main.go, line: 18, ok: bool
	printFuncInfo()
}

func printFuncInfo() {
	// Caller() 的参数, 0表示当前函数, 1表示第1层主调函数, 第n层就是第n层主调函数
	pc, file, line, ok := runtime.Caller(3)
	funcName := runtime.FuncForPC(pc).Name()
	fmt.Printf(
		"This is: %s, pc: %d, file: %s, line: %d, ok: %T\n",
		funcName, pc, file, line, ok,
	)
}

/*
上述的foo()函数调用链为:

...
runtime.main
main.main
main.foo
main.printFuncInfo     rpc[1]  Caller(0)
runtime.Caller         rpc[0]  skip = 0

这是 Caller(0) 的情况, rpc, skip 为 src/pkg/runtime/runtime.c -> runtime·Caller() 中的变量

如果 skip 参数为3, 那么 rpc 所表示的应该是如下情况

...
runtime.main           rpc[1]  Caller(3)
main.main              rpc[0]
main.foo               ┐
main.printFuncInfo     |       skip = 3
runtime.Caller         ┘

*/
