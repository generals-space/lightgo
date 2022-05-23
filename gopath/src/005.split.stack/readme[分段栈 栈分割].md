[A TRIP DOWN THE (SPLIT) RABBITHOLE](http://blog.nella.org/?p=849)
    - 使用6g, 6l等工具, 分别展现在编译/链接阶段的汇编行为, 确定栈分割是在哪个阶段注入的.
    - split stacks is implemented in the linker, not in the compiler
[Split Stacks in GCC](https://gcc.gnu.org/wiki/SplitStacks)



```
# 6l -a main.6 | head
codeblk [0x400c00,0x41c01a) at offset 0xc00
400c00	main.main            | (3)	TEXT	main.main+0(SB),$0
400c00	64488b0c25f0ffffff   | (3)	MOVQ	-16(FS),CX
400c09	483b21               | (3)	CMPQ	SP,(CX)
400c0c	7707                 | (3)	JHI	,400c15
400c0e	e87da60100           | (3)	CALL	,41b290+runtime.morestack00
400c13	ebeb                 | (3)	JMP	,400c00
400c15	                     | (3)	NOP	,
400c15	                     | (3)	NOP	,
400c15	                     | (3)	FUNCDATA	$0,main.gcargs·0+0(SB)
```

1. 链接期间的汇编指令是不分16位, 32位还是64位的.
2. fs指向当前G(Goroutine)结构体的起始位置.