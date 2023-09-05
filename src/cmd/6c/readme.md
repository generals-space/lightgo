`6c`的入口函数是在`src/lib9/main.c -> main()` -> `src/cmd/cc/lex.c -> main()`

用于构建 *.c 源码, 与之对应的是 6g, 用来构建 *.go 源码.

caller:

1. src/cmd/go/build_toolchain_gc.go -> gcToolchain.cc()
