# 拆分chan.c与select.c, 在make生成go二进制文件时出错-selectgo：undefined：dequeue

参考文章

1. [Static functions declared in "C" header files](https://stackoverflow.com/questions/42056160/static-functions-declared-in-c-header-files)
2. [c/c++开发分享在“C”头文件中声明的静态函数](https://www.ssfiction.com/c-cyuyankaifa/560791.html)
    - 参考文章1的中文翻译

## 问题描述

在读 chan.c 源码时, 发现 select 关键字的处理逻辑也在里面. 虽然 select 基本都是与 chan 对象同时使用, 但是还是想把两者区分开, 于是尝试拆分出 select.c 文件.

为了一些 chan.c 与 select.c 共用的变量及函数, 只能再增加一个头文件 chan.h.

但是在执行 make 生成 go 可执行文件时, 却出错了.

```console
$ cd ./src
$ bash ./make.bash
# Building C bootstrap tool.
cmd/dist

# Building compilers and Go bootstrap tool for host, linux/amd64.
lib9
libbio
## ...省略
pkg/go/build
cmd/go
selectgo: undefined: dequeue
selectgo: undefined: enqueue
selectgo: undefined: dequeueg
go tool dist: FAILED: /root/go/pkg/tool/linux_amd64/6l -o /root/go/pkg/tool/linux_amd64/go_bootstrap $WORK/_go_.6
```

看最后一行的信息, 发现没想到在编译期间没出现的问题, 却在链接期间发生了...明明在头文件中写了函数声明, 怎么会找不到呢?

## 排查思路(可以略过)

先从 make.bash 的编译过程查起, 定位到

```
./cmd/dist/dist bootstrap $buildall $GO_DISTFLAGS -v # builds go_bootstrap
```

再用 gdb 调试`dist`命令.

### gdb dist

```console
$ gdb /root/go/src/cmd/dist/dist    ## 此时 dist 命令还没拷贝到 pkg/tool/ 目录下.
(gdb) set args bootstrap -a -v
(gdb) b install                     ## 每一次断点, 都会编译一个目录的代码, 见 build.c -> buildorder()
(gdb) r
Starting program: /root/go/src/cmd/dist/dist bootstrap -a -v
```

然后定位到如下行

```c++
// src/cmd/dist/build.c
static void install(char *dir) {
    // dir 为 cmd/go (由于每个目录都会调用一次 install(), 所以最好在源码加个判断条件, 方便打断点)
    if(streq(dir, "cmd/go")) {
		xprintf("---\n");
	}
    // ...省略
	runv(nil, nil, CheckExit, &link);
}
```

此时 link 命令大致为

```
/root/go/pkg/tool/linux_amd64/6l -o /root/go/pkg/tool/linux_amd64/go_bootstrap /var/tmp/go-cbuild-NnkpPv/_go_.6
```

然后开始调试`6l`

### gdb 6l

```
$ gdb /root/go/pkg/tool/linux_amd64/6l
(gdb) set args -o /root/go/pkg/tool/linux_amd64/go_bootstrap /var/tmp/go-cbuild-wbBvNx/_go_.6
(gdb) b p9main
(gdb) r

```

然后定位到如下行

```c++
// src/cmd/6l/pass.c
void patch(void) {
    // ...省略
    diag("undefined: %s", s->name);
}
```

不过链接过程太复杂, 我还没搞明白, 根据看不懂这是在干啥.

## 解决方法

后来无意间找到了参考文章1, 发现题主描述的问题为, 在头文件中声明了静态函数声明, 还需要在包含头文件的每个`.c`文件中定义它, 否则编译器会报错.

好吧, 因此我把被 chan.c 和 select.c 同时引用的`dequeue()`, `enqueue()`和`dequeueg()` 3个函数, 在2个`.c`源文件都定义一遍.

这次编译真的成功了.
