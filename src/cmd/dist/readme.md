# dist

`dist`工具是源码编译的第1步, 之后由`dist`构建并生成`pkg/tool/linux_amd64{6g, 6l, 6c, go_bootstrap}`等工具, 然后再由`go_bootstrap`生成`go`二进制可执行程序.

`dist`程序入口: `main.c -> xmain()` -> `unix.c -> main()`.

`src/cmd/dist/build.c`中对`pkg/runtime`进行构建.
