在 make.bash 执行期间, dist 会先在 src/cmd/dist/ 目录下生成`dist`可执行文件.

在打印如下语句后, 执行`./src/cmd/dist/dist bootstrap -a -v`命令编译各目录
```
Building compilers and Go bootstrap tool for host, linux/amd64.
```

```
$ ./src/cmd/dist/dist bootstrap -a -v
lib9
libbio
libmach
misc/pprof
cmd/addr2line
cmd/nm
cmd/objdump
...
```


```console
$ ./src/make.bash
$ gdb /root/go/pkg/tool/linux_amd64/dist
(gdb) set args bootstrap -a -v
(gdb) b install ## 每一次断点, 都会编译一个目录的代码, 见 build.c -> buildorder()
(gdb) r
Starting program: /root/go/pkg/tool/linux_amd64/dist bootstrap -a -v

Breakpoint 1, install (dir=0x614840 "lib9") at cmd/dist/build.c:629
629		if(vflag) {
(gdb) c
```

在编译 runtime 时, 会在 workdir 生成 runtimedefs 文件, 看内容像是变量表与函数表, 这个文件可以研究一下.

```c++
func gentraceback(a uint64, b uint64, c uint64, d *g, e int32, f *uint64, g int32, h func(*stkframe, unsafe.Pointer), i unsafe.Pointer, j uint8) int32
func traceback(a uint64, b uint64, c uint64, d *g)
func tracebackothers(a *g)
func haszeroargs(a uint64) uint8
func topofstack(a *_func) uint8
var emptystring	string
var zerobase	uint64
var allg	*g
var lastg	*g
var allm	*m
var allp	**p
```
