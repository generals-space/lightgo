# gdb调试时报错退出fatal error - runtime - misuse of rewindmorestack[bug]

参考文章

1. [Why does gdb output fatal error?](https://stackoverflow.com/questions/21323654/why-does-gdb-output-fatal-error)
2. [runtime: gdb next at function entry point makes "misuse of rewindmorestack" crash in runtime.newstack on freebsd/amd64](https://github.com/golang/go/issues/6278)
3. [runtime: jump to badmcall instead of calling it.](https://github.com/golang/go/commit/32b770b2c05d69c41f0ab6719dc028cf4c79e334)
    - 修复代码

## 问题描述

在用gdb调试示例006时, 执行next指令, 突然报了"fatal error: runtime: misuse of rewindmorestack"的错误...

```

```
