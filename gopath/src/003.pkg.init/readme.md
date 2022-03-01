这个示例是为了验证 Go 1.5源码剖析 第三章 初始化中, 对于 runtime.init -> main.init -> main.main 初始化流程.

不过 go 1.2 貌似没有 go tool objdump 命令, 没办法看到汇编代码, 而且目前还不会 gdb 的 disass 命令.

粗略的试了下, 下面的一系列`callq`指令, 还是能看出些东西的, 尤其是"<lib.init>"和"<net/http.init>"部分

> go 1.2 和 go 1.5 还是有点差别的, 在"runtime·main"函数中, 前者是 main.init(), main.main(), 而后者是 main_init(), main_main().

```
(gdb) info file
(gdb) b *xxxxxx
(gdb) r
(gdb) b main.init
(gdb) c
(gdb)
(gdb) disass main.init
Dump of assembler code for function main.init:
   0x0000000000400dc0 <+0>:	mov    %fs:0xfffffffffffffff0,%rcx
   0x0000000000400dc9 <+9>:	cmp    (%rcx),%rsp
   0x0000000000400dcc <+12>:	ja     0x400dd5 <main.init+21>
   0x0000000000400dce <+14>:	callq  0x4246b0 <runtime.morestack00>
   0x0000000000400dd3 <+19>:	jmp    0x400dc0 <main.init>
   0x0000000000400dd5 <+21>:	mov    0x87e0a4,%bl
   0x0000000000400ddc <+28>:	cmp    $0x0,%bl
   0x0000000000400ddf <+31>:	je     0x400df5 <main.init+53>
   0x0000000000400de1 <+33>:	mov    0x87e0a4,%bl
   0x0000000000400de8 <+40>:	cmp    $0x2,%bl
   0x0000000000400deb <+43>:	jne    0x400dee <main.init+46>
   0x0000000000400ded <+45>:	retq
   0x0000000000400dee <+46>:	callq  0x413450 <runtime.throwinit>
   0x0000000000400df3 <+51>:	ud2
   0x0000000000400df5 <+53>:	movb   $0x1,0x87e0a4
   0x0000000000400dfd <+61>:	callq  0x44a0c0 <lib.init>
   0x0000000000400e02 <+66>:	callq  0x440f80 <net/http.init>
   0x0000000000400e07 <+71>:	callq  0x400c00 <main.init·1>
   0x0000000000400e0c <+76>:	callq  0x400c60 <main.init·2>
   0x0000000000400e11 <+81>:	callq  0x400ca0 <main.init·3>
   0x0000000000400e16 <+86>:	movb   $0x2,0x87e0a4
   0x0000000000400e1e <+94>:	retq
End of assembler dump.
```
