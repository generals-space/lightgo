参考文章

1. [Go语言调试器开发](https://www.hitzhangjie.pro/debugger101.io/)
2. [Golang笔记–07–Linux环境下的Go程序启动流程](https://wbuntu.com/golang-note-how-does-go-program-startup-in-linux/)
3. [Analyzing Golang Executables](https://www.pnfsoftware.com/blog/analyzing-golang-executables/)
4. [cmd/compile: DWARF information for local variables missing](https://github.com/golang/go/issues/14744)
5. [cmd/ld: older gdb cannot parse dwarf3 output](https://github.com/golang/go/issues/3436)
6. [实现一个 Golang 调试器（第一部分）](https://studygolang.com/articles/12553)
7. [Go: A Documentary](https://golang.design/history/#language-design)

[说说编译链接系统中的符号(symbol)、重定位(relocation)、字串表(string-table)和节(section)](https://blog.csdn.net/liigo/article/details/4858535)
    - 假如一款新诞生的编程语言（就说易语言吧），不想重复开发专用链接器，可以考虑编译生成C语言格式的目标文件，进而得以使用现有的C语言链接器链接生成可执行文件，如此一来可大幅减少开发工作量，降低研发成本的同时还提高了系统的开放性。
    - 目标文件中的基本元素有，符号(symbol)、重定位(relocation)、字串表(string-table)和节(section)。这里说的是逻辑概念。

[go build是如何工作的](https://www.jianshu.com/p/9e71718aa7ef)
    - go build 创建了一个临时目录 /tmp/go-build249279931 并且填充一些框架性的子目录用于保存编译的结果。
    - 编译器和链接器仅接受了单一的文件作为包的内容，如果包含多个目标文件，在使用它们之前，必须将其打包到一个单一的 .a 归档文件中。
