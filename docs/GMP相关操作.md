参考文章

1. [自己对go协程的理解](https://www.jianshu.com/p/4267cfbbc2d1)
    - qq群"天天"大神写的, 比较亲民
2. [go语言调度器源代码情景分析](https://mp.weixin.qq.com/mp/homepage?sn=8fc2b63f53559bc0cee292ce629c4788&__biz=MzU1OTg5NDkzOA%3D%3D&scene=18&hid=1&devicetype=iOS13.1.2&version=17000831&lang=zh_CN&nettype=WIFI&ascene=0&session_us=gh_ceeb25947b8b&fontScale=100&pass_ticket=CiGLC18BKjQ8hvKyiMSivuT%2ByXVZrOOaysEtqZt15G6c55gdqBGw7H11c4lzLcEG&wx_header=1&scene=1)
    - 微信系列文章

## M <=> P

acquirep(p): 将当前m与p绑定, m-p = p; p->m = m

releasep(): 将当前m与其绑定的p解绑, m->p = nil; p->m = nil

## 

runtime·malg: 创建新的g对象, 并为其分配初始栈空间.

runtime·newproc1(fn): 调用`runtime·malg`创建新的g对象, 来执行fn函数


GMP是谁订阅谁? 是M不断寻找待执行的G, 还是G不断寻找可执行M?
    - M遍历G, 那多余的M是根据什么判断出来的, 会自动退出吗? 如何实现的?

G对象怎么判断自己进入阻塞, 如何告诉M去执行其他G? 当前谁告知阻塞的G已经完成了, 可以继续执行的?

调度器会在什么情况下新建/回收 m 对象, 根据什么判断??? 

```
startm()
|
---- newm(fn, p)
     |
     ---- runtime·newosproc()
          |
          ---- runtime·clone(runtime·mstart)
               |
               ---- runtime·mstart()
                    |
                    ---- m->mstartfn() + schedule()
```

------

m与g绑定的调度场景有哪些? 即, m 主动去寻找待执行的 g 有哪几种可能:

见 src/pkg/runtime/proc.c -> schedule()

```c++
// caller: 
// 	1. runtime·mstart() 新建的 m 对象完成并启动后, 调用此函数参与调度循环.
// 	2. park0() 好像只与 channel 有关
// 	3. runtime·gosched0() 开发者在代码中, 或是gc过程中在start the world后, 通知 m0 主动进行重新调度;
// 	4. goexit0() g 对象(如go func()) 执行结束时, 需要为此 m 对象寻找新的 g 任务
// 	5. exitsyscall0()
static void
schedule(void)
```

------

golang 运行时中, 是否存在 g0, m0 这种"主协程"的概念, 主要用来做什么?

有, g0 == m0.g0, 此关系一直成立. 但是 m 中还存在一个 m.curg, 这个才是当前 m 绑定的 g.

m0 可以与 g0 解绑(即 m0.curg != g0), 然后去执行其他 g 任务.

只有 m0 和 g0 绑定时, 才可以执行一此特殊的操作.

所谓的主线程, 全局唯一, 具有特殊性.

1. 主线程的线程 tid 即是进程 pid, 而子线程的 tid 则不一样.
2. 主线程才能接收并处理来自控制台终端的 signal 信号.
3. gc时, STW 完成后, 只剩下 m0, 之后实际的 gc 行为也是由 m0 发起.

可以通过`if(g == m->g0)`, `if(g != m->g0)`查看有哪些操作必须由 g0 执行.

比如`runtime·mcall(fn)`就是切换到 m->g0 的上下文, 执行目标函数.

------

用常规多线程+任务队列, 模拟一下GMP模型:

1. OS线程从单任务队列中取任务, 这种形式, 一个任务将占用一个OS线程, 直到完成, 没有阻塞时自动切换的能力;
2. 对任务进行分段, 以便可以切换线程执行时, 能实现断点续传;
