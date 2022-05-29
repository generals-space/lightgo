
```bash
add-auto-load-safe-path	/root/go/src/pkg/runtime/runtime-gdb.py
set disassembly-flavor intel
set print pretty on
dir /root/go/src/pkg/runtime
```



```
b main.main
b newm
b startm
b schedule
b runtime.newproc
b runtime.gc
```

GODEBUG=gctrace=1


```
b gc
```
