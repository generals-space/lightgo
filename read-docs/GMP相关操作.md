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

用常规多线程+任务队列, 模拟一下GMP模型:

1. OS线程从单任务队列中取任务, 这种形式, 一个任务将占用一个OS线程, 直到完成, 没有阻塞时自动切换的能力;
2. 对任务进行分段, 以便可以切换线程执行时, 能实现断点续传;

