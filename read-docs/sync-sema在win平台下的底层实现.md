# sync-sema在win平台下的底层实现

参考文章

1. [使用信号量 进行windows线程同步 (Semaphore)](https://blog.csdn.net/lvbian/article/details/11806871)
    - semaphore
2. [Windows线程同步（下）](https://www.cnblogs.com/predator-wang/p/5132401.html)
    - event
3. [Windows核心编程读书笔记4（第7、8、9章）](http://www.youngroe.com/2015/10/23/Read-Notes/Read-Notes-Windows-Via-C-C-4/)


```c++
//创建信号量, 返回值即为信号量对象
// SD    信号量安全属性,一般内核对象都会具有此数据结构
// initial count  初始化信号量的大小
// maximum count  信号量最大值,
// object name    信号量的名称(内核对象名称,跨进程访问使用,此处匿名)
HANDLE CreateSemaphore(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCTSTR lpName
);

//等候任何可等候的内核对象
// handle to object    内核对象句柄(此句柄只是进程内有效), 即信号量对象
// time-out interval   等候的超时时间
DWORD WaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
);

//释放信号量
// handle to semaphore   目标信号量句柄
// count increment amount  信号量数量(要操作的数量) 
//                         如果大于初始化信号量的最大值,则函数执行失败,信号量不做变.
// previous count   在操作执行之前,信号量的数量
BOOL ReleaseSemaphore(
    HANDLE hSemaphore,
    LONG lReleaseCount,
    LPLONG lpPreviousCount
);
```

每使用WaitForSingleObjecst等候一次,信号量减一,等到信号量为0时,线程阻塞. ReleaseSemaphore负责释放信号量.

但是需要注意的是, 在`os_windows.c`中, sema的create, sleep 和 wakeup 目标都是 Event类型, 而不是信号量类型.

另外, SetEvent() 作为唤醒操作, 其实是像内核对象 waitsema 发送一个信号, 因WaitForXXX而阻塞的线程会按照先入先出的顺序依次唤醒.

> 内核对象触发后，Wait在上面的线程被唤醒，决定哪一个线程首先被唤醒的规则基本上就是等待顺序的先入先出，和线程的优先级等无关。 --参考文章3
