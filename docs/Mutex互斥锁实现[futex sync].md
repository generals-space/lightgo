参考文章

1. [[Pthread] Linux中的线程同步机制(一) -- Futex](https://blog.csdn.net/Javadino/article/details/2891385)
2. [[Pthread] Linux中的线程同步机制(二)--In Glibc](https://blog.csdn.net/Javadino/article/details/2891388)
3. [[Pthread] Linux中的线程同步机制(三)--Practice](https://blog.csdn.net/Javadino/article/details/2891399)
4. [Futex设计与实现](https://www.jianshu.com/p/d17a6152740c)
5. [各种锁及其Java实现！](https://zhuanlan.zhihu.com/p/71156910)
    - 偏向锁 → 轻量级锁 → 重量级锁

golang runtime 底层声明了`Lock`对象, 并借助`futex`系统调用实现了在`Lock`对象上的休眠与唤醒.

具体的实现流程在`src/pkg/runtime/lock_futex.c` -> `runtime·lock()`.

由于`futex`的休眠与唤醒, 作为系统调用是比较费时的, 因此在`runtime·lock()`模仿了Java的渐进锁的实现方式.
