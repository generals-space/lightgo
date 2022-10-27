# go1.2 6l 6g等工具的编译生成及使用[8l 8g objdump]

参考文章

1. [开始学Go](https://tonybai.com/2012/08/14/getting-going-with-go/)
    - 标准Go语言编译器被称为gc，与其相关的工具链包括用于编译的`5g`、`6g`和`8g`；用于链接的`5l`、`6l`和`8l`以及用于查看Go语言文档的`godoc`
    - 这些奇怪的名字遵循了Plan 9操作系统编译器的命名方式，即用数字表示处理器体系.
        - "5"代表ARM
        - "6"代表AMD64(包括Intel 64位处理器)
        - "8"代表Intel 386
2. [A TRIP DOWN THE (SPLIT) RABBITHOLE](http://blog.nella.org/?p=849)
    - split stacks is implemented in the linker, not in the compiler

执行`make.bash`脚本编译好go的二进制文件后, 可以使用刚生成的go命令继续安装调试命令, 如下

```console
$ cd ./src
$ go tool dist install -vv
cmd/6l
rm -rf /var/tmp/go-cbuild-uc0i4Z
```

这一句执行的其实是"./src/Make.dist"文件中的编译命令, 这个文件被"./src/cmd/{6l,6g,cgo,objdump}"等每个目录下的Makefile文件引用.

如"./src/cmd/6l/Makefile"文件的内容是

```makefile
include ../../Make.dist
```

生成的二进制文件可在如下路径查看

```console
$ cd ./pkg/tool/linux_amd64
$ ll
total 18672
-rwxr-xr-x 1 root root  341216 Mar  5 18:57 6a
-rwxr-xr-x 1 root root 1030088 Mar  5 18:57 6c
-rwxr-xr-x 1 root root 1972432 Mar  5 18:57 6g
-rwxr-xr-x 1 root root 1089880 Mar  6 19:45 6l
-rwxr-xr-x 1 root root  523880 Mar  5 18:57 addr2line
-rwxr-xr-x 1 root root 5483904 Mar  5 18:57 cgo
-rwxr-xr-x 1 root root   74592 Mar  5 18:57 dist
-rwxr-xr-x 1 root root 4191264 Mar  5 18:57 fix
-rwxr-xr-x 1 root root  406256 Mar  5 18:57 nm
-rwxr-xr-x 1 root root  529552 Mar  5 18:57 objdump
-rwxr-xr-x 1 root root  355096 Mar  5 18:57 pack
-rwxr-xr-x 1 root root  157784 Mar  5 18:57 pprof
-rwxr-xr-x 1 root root 2935448 Mar  5 18:57 yacc
```

其中6a用于编译, 6g用于链接.

在`go build`出现之前, golang开发者一直使用6a, 6c, 6g等工具进行编译和链接的.

`go build`子命令应该是直接调用的`6g`和`6l`(不过不知道是不是将其内嵌到go命令了).

## 使用方法 - 编译, 链接

以示例001为例

6g编译生成 main.6

```console
$ 6g main.go
$ ls
main.6  main.go
```

6l链接生成可执行文件 6.out

```console
$ 6l main.6
$ ls
6.out  main.6  main.go
```

执行

```console
$ ./6.out
hello world
```

6.out 与 go build 生成的文件大小相同.

```console
$ ll -h
总用量 1.2M
-rwxr-xr-x 1 root root 564K 4月  27 14:26 6.out
-rw-r--r-- 1 root root 1.8K 4月  27 14:25 main.6
-rw-r--r-- 1 root root  106 2月   7 19:03 main.go
-rwxr-xr-x 1 root root 564K 2月  15 14:33 main.gobin
```

## 查看汇编过程代码

查看参考文章2, 可知`6g -S *.go`与`6l -a *.6`可分别在"编译", "链接"过程中打印出汇编代码.

## FAQ

### go tool: no such tool "dist"

```console
$ go tool dist install -vv
go tool: no such tool "dist"
```

