```
$ gdb /root/go/pkg/tool/linux_amd64/6c
(gdb) set args -F -V -w -N -p -I /var/tmp/go-cbuild-juDAqI -I /root/go/pkg/linux_amd64 -D GOOS_linux -D GOARCH_amd64 -D GOOS_GOARCH_linux_amd64 -o /var/tmp/go-cbuild-juDAqI/runtime.o /root/go/src/pkg/runtime/runtime.c
```

`/var/tmp/go-cbuild-XXXXXX`是编译的临时目录, 编译后会立刻删除, 需要事先进行一次不完整的编译, 使临时目录可以保留下来, 以便 6c 调试时使用.

------
