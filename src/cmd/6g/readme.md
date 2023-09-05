`6g`的入口函数在`src/cmd/gc/lex.c -> main()`.

用于构建 *.go 源码, 与之对应的是 6c, 用来构建 *.c 源码.

caller:

1. src/cmd/go/build_toolchain_gc.go -> gcToolchain.gc()
