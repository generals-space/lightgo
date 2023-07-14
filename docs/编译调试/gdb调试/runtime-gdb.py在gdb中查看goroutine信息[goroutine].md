# runtime-gdb.py在gdb中查看goroutine信息[goroutine]

参考文章

1. [gdb调试golang中的内部运行时信息(如:goroutine)](https://studygolang.com/topics/816/comment/2773)
    - 提到了 runtime-gdb.py 脚本
    - "gdb -x gdb.config"自动加载配置
2. [golang runtime-gdb.py的问题](https://blog.csdn.net/body100123/article/details/46380457)
    - runtime-gdb.py 的使用方法 - 需要在 gdb 交互式终端中执行 source 命令.

```
(gdb) source ~/go/src/runtime/runtime-gdb.py
Loading Go Runtime support.
```

其实单纯使用gdb, 可以查看到`map`, `slice`变量类型.

> 注意: 使用`go build`编译时需要添加`-gcflags='-l -N'`选项.

```
Breakpoint 1, main.main () at /root/go/gopath/src/006.channel/main.go:8
8	func main() {
(gdb) n
9		m := make(map[string]string, 0)
(gdb) n
10		log.Printf("%+v", m)
2022/05/25 17:13:24 map[]
(gdb) p m
$1 = (map[string]string) 0xc21001e180
(gdb) p $len(m)                                                                 ## $len() 方法也是可以使用的(这里报错是因为map没有被初始化)
Python Exception <type 'exceptions.TypeError'> Could not convert Python object: None.:
Error while executing Python code.
```

runtime-gdb.py提供的是对 goroutine 信息的查看, 在未加载时, 执行"info goroutine"或"info g"时, 会有如下输出

```
(gdb) info goroutine
Undefined info command: "goroutine".  Try "help info".
```

而在加载后, 可以看到这样的结果

```
(gdb) info goroutine
* 1  running syscall.Syscall
* 2  syscall runtime.notetsleepg
```

> 如果输出为空, 可能是因为进程还没启动, 没有生成goroutine.

------

```
warning: File "/root/go/src/pkg/runtime/runtime-gdb.py" auto-loading has been declined by your `auto-load safe-path' set to "$debugdir:$datadir/auto-load:/usr/bin/mono-gdb.py".
To enable execution of this file add
        add-auto-load-safe-path	/root/go/src/pkg/runtime/runtime-gdb.py
line to	your configuration file	"/root/.gdbinit".
To completely disable this security protection add
        set auto-load safe-path	/
```

根据提示, 可以在"/root/.gdbinit"文件中添加"add-auto-load-safe-path	/root/go/src/pkg/runtime/runtime-gdb.py"行, 让gdb每次启动时自动加载此脚本.

