# golang

## 项目目的

基于 golang v1.2 版本, 进行源码阅读与改造, 之后将 go 1.2 作为小项目的主语言, 不停迭代并深入挖掘其底层原理.

即使是早期版本, golang 也有诸多无可比拟的优点:

1. 语法简单(极致简洁), 无过多黑魔法(指 python, js);
2. 编译结果为单文件, 无需依赖包.
3. cgo内嵌c, 可以混合编程;
4. ...

## 为什么选择此版本

1. 基于软件工程开发规律, golang v1.0 版本即进入稳定版本;
2. v1.0 还不够完善, 主要是由于此版本还未实现 GMP 框架, 而 GMP 正是 golang 区别于其他语言的最大亮点;
3. v1.5 版本实现自举, 使用 golang 编译器代替了 gcc 编译器, 隐藏了太多 clang 层面的细节;

golang演进历史

- 0.x
    - 单线程调度器
    - 非稳定版本
- 1.0
    - 多线程调度器(GMP系统还不完整, 只有 GM, 没有 P)
- 1.1
    - 引入了处理器 P, 构成了目前的 GMP 模型
- 1.2
    - **当前版本**
- 1.3
    - 优化GC流程, 并发清理(不过仍需要STW)
- 1.4
    - 抢占调度
    - 连续栈
- 1.5
    - 实现自举, 可用 go v1.4 编译生成 golang 环境
    - 优化GC流程, 引入三色标记
- 1.6
    - 引入 vendor 本地库目录

为了研究更多底层细节(包括gcc编译, gdb调试, 汇编语言, 词法及语法分析规则等), 可以接受不完善的GC流程和调度框架, 因此最终选择了 go v1.2.

## 所做的变更

- 源码阅读, 添加注释, 拆分过长的源文件;
    - GMP调度框架, 内存管理, GC流程
    - 类型系统, `runtime/reflect`
    - 原生类型底层实现, slice/hashmap/channel
    - 汇编语言
- 放弃跨平台可移植性, 移除平台无关代码, 只保留`linux amd64`架构;
- 补充标准库函数, 实现向后兼容(go v1.2+后续新增的"函数/方法/变量/结构"等, 被放在独立的`_compatible.go`文件中);
- 借助`bison`语法分析器, 实现语法层面的向后兼容;

## 启动开发环境

golang v1.2 过于陈旧, 当前的 vscode go 扩展已经无法正确安装, 因此借助 docker 封装了一个远程容器开发环境, 集成了合适版本的扩展包.

首先启动容器环境

```
docker run -d --name golang-src --privileged -p 2222:22 -v /usr/local/go1.2:/usr/local/go registry.cn-hangzhou.aliyuncs.com/generals-space/golang-src:latest
```

然后进入容器, 克隆当前仓库到`/usr/local/go`目录

```
git clone https://gitee.com/skeyes/gods.git /usr/local/go
```

上述容器中内置了`sshd`服务, 可使用 vscode 通过 remote ssh 插件连接进入.

然后在`/usr/local/gopath_extension/vsix`下寻找并安装vscode开发插件, 即可实现代码提示和补全.

## 编译golang源码

在当前目录下直接执行`make`即可, `go`二进制可执行文件将出现在`bin`子目录. 

由于容器环境已经将`/usr/local/go/bin`加入了`PATH`环境变量, 因此可以直接执行`go`命令应用最新的二进制程序.
