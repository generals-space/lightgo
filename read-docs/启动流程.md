# 启动流程

```
func main()
|
└─  src/pkg/runtime/rt0_linux_amd64.s
    _rt0_amd64_linux(): 汇编入口
    main()
    |
    └─  src/pkg/runtime/asm_amd64.s
        _rt0_go(): 初始操作汇总函数
        |
        ├─  src/pkg/runtime/runtime.c
        |   runtime·args()
        ├─  src/pkg/runtime/os_linux.c
        |   runtime·osinit()
        ├─  src/pkg/runtime/alg.c
        |   runtime·hashinit()
        ├─  src/pkg/runtime/proc.c
        |   runtime·schedinit()
        ├─  src/pkg/runtime/proc.c
        |   runtime·newproc()
        └─
```

按照`proc.c`中所说, 

1. call osinit
2. call schedinit
3. make & queue new G
4. call runtime·mstart

The new G calls runtime·main.

看样子是先创建任务对象g, 再启动m去执行ta.

我想这个流程实际是写在`asm_adm64.s`的第一个函数`_rt0_go`中的.

rt0_linux_amd64.s -> _rt0_amd64_linux()

rt0_linux_amd64.s -> main()

asm_amd64.s -> _rt0_go()

------

全局 m g 对象, 在 src/pkg/runtime/runtime.h 中, 通过如下语句声明

```c++
extern	register	G*	g;
extern	register	M*	m;
```

在 src/pkg/runtime/asm_amd64.s -> _rt0_go() 启动过程中(runtime·osinit 和 runtime·schedinit 之前)被赋值

```asm
	LEAQ	runtime·g0(SB), CX
	MOVQ	CX, g(BX)
	LEAQ	runtime·m0(SB), AX
	MOVQ	AX, m(BX)
```

