## godoc

`godoc --http=:6060` and then visiting http://localhost:6060/doc/install/source.

## 使用rdtsc测量函数的执行时间

参考文章

1. [使用rdtsc测量函数的执行时间](https://www.jianshu.com/p/0c519fc6248b)

## 多线程服务中, GC操作是由哪个线程发起的? 发起GC的线程在STW时, 其实就是让其他线程停止运行吗?

是由独立的, 专用的GC线程, 还是由主线程发起?

## 堆是进程间共享吗? 父进程fork子进程时会把堆对象也复制给子进程吗? 还是说是整个进程中唯一的?

进程是操作系统中资源分配的最小单位, 所以堆应该是进程独有的.

## `runtime/symtab.c`中定义程序启动时检测函数符号表(编译好的二进制文件装载到内存时函数地址是可以确定的), 但其中只有检测的过程, 没有加载的过程, 那么加载的过程是OS来做的吗? 还是在编译时期就已经确定, 运行时只是简单装载呢?

## golang runtime 如何自定义栈以及完成栈切换的?

我知道 g 任务都有自己独立的栈, 并将栈基, 栈顶等地址存储在 g 对象中. 但是 m <-> g 切换时, 也会同时发生栈的上下文切换, 目前还不清楚该机制.

相关函数: runtime·gogo()
