参考文章

1. [go程序调试总结](http://blog.rootk.com/post/use-gdb-with-golang.html)
2. [Go 1.5 源码剖析 （书签版）第2章 "引导"]()
    - gdb info files
3. [A TRIP DOWN THE (SPLIT) RABBITHOLE](http://blog.nella.org/?p=849)
    - gdb info reg 查看当前上下文寄存器中的信息
4. [Debugging Go Code with GDB](https://go.dev/doc/gdb)
    - golang 官方文档
5. [go程序调试总结 - 天地孤影任我行](http://blog.rootk.com/post/use-gdb-with-golang.html)
    - golang1.3之后的版本，对于支持gdb调试存在很大的问题
    - Python Exception <class 'gdb.MemoryError'> Cannot access memory at address 0x8:
6. [gdb: nothing works (windows amd64) ](https://github.com/golang/go/issues/5552)
    - p 'main.S'
    - as main.S is the symbol name.
    - 可以解决打印 runtime 包中变量时, 出现"No symbol "runtime" in current context."的问题
