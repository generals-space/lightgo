# golang

## 启动开发环境

首先启动容器环境

```
docker run -d --name golang-src.v1.2 --privileged -p 2222:22 -v /usr/local/go:/usr/local/go registry.cn-hangzhou.aliyuncs.com/generals-space/golang-src:latest
```

然后进入容器, 克隆当前仓库到`/usr/local/go`目录

```
git clone https://gitee.com/skeyes/gods.git /usr/local/go
```

上述容器中内置的`sshd`服务, 可用vscode remote ssh连接进入.

然后在`/usr/local/gopath_extension/vsix`下寻找并安装vscode开发插件, 即可实现代码提示和补全.

## 编译golang源码

在当前目录下直接执行`make`即可, `go`二进制可执行文件将出现在`bin`子目录. 

由于容器环境已经将`/usr/local/go/bin`加入了`PATH`环境变量, 因此可以直接执行`go`命令应用最新的二进制程序.
