参考文章

1. [Golang 1.3 sync.Mutex 源码解析](https://studygolang.com/articles/1472)
    - 代码分析的很详细, 透彻.

golang 1.2 的时候还没自旋锁的概念.

`mutex.go`中调用的`runtime_Semacquire()`, 实际需要追踪到`sema.goc`文件中的`runtime·semacquire()`.
