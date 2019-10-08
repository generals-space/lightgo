参考文章

1. [Go 系列文章6: syscall](http://xargin.com/syscall/)

按照 linux 的 syscall 调用规范，我们只要在汇编中把参数依次传入寄存器，并调用 SYSCALL 指令即可进入内核处理逻辑，系统调用执行完毕之后，返回值放在 RAX 中:

- RAX: 系统调用编号/返回值
- RDI: 参数1
- RSI: 参数2
- RDX: 参数3
- R10: 参数4
- R8: 参数5
- R9: 参数6

实际说明见 `asm_linux_amd64.s` 文件关于 syscall 的代码.

> 好像参数最多的系统调用是 `futex`, 有6个.

```asm
TEXT runtime·exit1(SB),NOSPLIT,$0-8
	MOVL	8(SP), DI
	MOVL	$60, AX	// exit - exit the current os thread
	SYSCALL
	RET
```

$0 表示此函数栈帧size为0, 内部没有声明局部变量, 也没有调用其他函数.

$8 表示传入参数长度为8 bytes(并且exit函数没有返回值)

`MOVL	8(SP), DI` 将从SP开始的8 bytes的内容, 赋值到 DI 寄存器

如果没有为TEXT指定NOSPLIT标志, 必须提供参数大小.