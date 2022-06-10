runtime·MHeap_Scavenger()
|
├── forcegchelper()
|   |   超过2分钟没有进行常规 gc 时, 调用此函数进行例行 gc(也叫强制 gc)
|   |
|   ├── runtime·gc()
|   |   |
|   |   |
|   |   ├── runtime·semacquire() 获取全局锁
|   |   |
|   |   ├── runtime·stoptheworld()
|   |   |
|   |   ├── mgc() 基本没做什么事情, 只是调用了如下2个函数.
|   |   |   |
|   |   |   ├── gc(): 确定参与 gc 的协程数量, 调用 addroots() 添加根节点.
|   |   |   |   |
|   |   |   |   ├── addroots()
|   |   |   |   |
|   |   |   |   ├── gchelperstart()
|   |   |   |   |
|   |   |   |   ├── 打印 gc stats 调试信息
|   |   |   |   |
|   |   |   |   ├── runtime·MProf_GC(): 记录 gc 相关的 prof 数据信息.
|   |   |   |
|   |   |   |
|   |   |   ├── runtime·gogo()
|   |   |
|   |   |
|   |   ├── runtime·semrelease() 释放全局锁
|   |   |
|   |   ├── runtime·starttheworld()
|   |
|   ├── runtime·notewakeup()
|   
|   
|   
├── scavenge()
    |   将 mheap 中, 各级别 span list, 以及 large list 中长时间未被使用的成员, 归还给OS.
    |
    ├── runtime·SysUnused(): 使用系统调用, 将指定块的内存, 归还给OS.
    |   
    ├── 
|   
├──

