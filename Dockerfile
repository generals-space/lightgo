## docker build -t registry.cn-hangzhou.aliyuncs.com/generals-space/golang-src:v1.2.1 .
## docker run -d --name golang-src.v1.2 -p 2222:22 --privileged registry.cn-hangzhou.aliyuncs.com/generals-space/golang-src:latest
FROM registry.cn-hangzhou.aliyuncs.com/generals-space/remote-dev:latest

## 这里声明的环境变量, 在容器启动后仍然有效
ENV GOROOT /root/go
## 不设置GOPATH, 默认为/root/go
ENV GOPATH /root/go/gopath
ENV GO_BIN /root/go/bin
## dist 命令会移动到此目录
ENV GO_TOOL /root/go/pkg/tool
## 6g, 6l 等命令会在此目录
ENV GO_TOOLLIST /root/go/pkg/tool/linux_amd64

ENV PATH ${PATH}:${GO_BIN}:${GO_TOOL}:${GO_TOOLLIST}:/root/gotoolpath/bin

WORKDIR /root/go
COPY . .

