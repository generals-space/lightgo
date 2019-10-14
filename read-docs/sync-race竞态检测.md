`sync.Mutex.Lock()`等函数中存在调用`raceAcquire()`的代码, 几乎所有调用`raceAcquire()`的部分都会首先判断`raceenabled`变量.

其实这一部分代码都流程逻辑本身是没有影响的, 只是作为竞态检测的入口. 而竞态检测实际的实现代码在`runtime/race/*.syso`二进制文件中, 因为golang是集成了名为`ThreadSanitizer`的竞态检测器(一个用于C/C++的检测器). 

所有golang源码本身中的竞态检测相关代码, 都只包含了一对`m->racecall = true;`, `m->racecall = false;`, 中间加上对`ThreadSanitizer`检测器的埋点调用, 可以略过, 因为对源码分析并无影响.
