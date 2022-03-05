参考文章

1. [Golang 调用汇编代码，太方便啦](https://my.oschina.net/zengsai/blog/123916)
    - 汇编源码其实不必命名为"xxx_ARCH.s"格式(如"add_amd64.s")

配置好GOPATH环境变量后, 直接执行`go run main.go`即可.

```console
$ go run main.go
5
```

> lib目录下的`.go`和`.asm`文件不必同名.

------

汇编混合编程要求汇编代码必须独立存在一个package中, 汇编代码编写函数体, 同包内使用go代码编写函数声明, 外部可以正常引用.

如果main.go与add.s存在同一目录下, 内容分别为

```go
package main

func Add(x, y int64) int64

func main() {
    println(Add(2, 3))
}
```

```asm
TEXT ·Add(SB),$0-24
    MOVQ x+0(FP), BX
    MOVQ y+8(FP), BP
    ADDQ BP, BX
    MOVQ BX, ret+16(FP)
    RET
```

这样直接执行会出错

```console
$ go run main.go
# command-line-arguments
main.go:3: missing function body
```
