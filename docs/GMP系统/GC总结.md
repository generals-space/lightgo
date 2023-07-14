# GC总结

`runtime/proc.c` -> `runtime·gomaxprocsfunc()` 中对 MaxGomaxprocs 的调整用到了 STW 操作, 先 stoptheworld, 然后修改 procs 值, 再 starttheworld.

由此猜想, 在 stop 的时候将当前所有 p 休眠, 而在 start 的时候 p 启动将受到 procs 的限制. 不足的 p 在start 时应该会补足, 那么多余的 p 是在 start 的时候不启动吧? 会被删除吗?
