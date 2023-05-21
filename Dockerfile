## docker build -t registry.cn-hangzhou.aliyuncs.com/generals-space/golang-src:v1.2.2 .
## docker run -d --name golang-src.v1.2 --privileged -p 2222:22 -v /usr/local/go:/usr/local/go registry.cn-hangzhou.aliyuncs.com/generals-space/golang-src:latest
FROM registry.cn-hangzhou.aliyuncs.com/generals-space/remote-dev:latest as gopath_extension
RUN cd /root/ && git clone https://gitee.com/skeyes/go-vs-extension.git

FROM registry.cn-hangzhou.aliyuncs.com/generals-space/remote-dev:latest

## 这里声明的环境变量, 在容器启动后仍然有效
ENV GOROOT /usr/local/go
## dist 命令会移动到此目录
ENV GO_TOOL ${GOROOT}/pkg/tool
## 6g, 6l 等命令会在此目录
ENV GO_TOOLLIST ${GO_TOOL}/linux_amd64

## 工程内部的 gopath 目录, 也要追加到 GOPATH 路径列表中
ENV GO_PATH_LOCAL ${GOROOT}/gopath
## vscode golang 插件所需的子插件, 如代码补全, 智能提示等工具
ENV GO_PATH_EXTENSION /usr/local/gopath_extension

ENV GOPATH /usr/local/gopath:${GO_PATH_LOCAL}:${GO_PATH_EXTENSION}

## 将 dist, 6g, 6l 等命令加入 PATH
ENV PATH $PATH:${GOROOT}/bin:${GO_TOOL}:${GO_TOOLLIST}
## 将各 GOPATH 下的 bin 目录加入 PATH
ENV PATH $PATH:/usr/local/gopath/bin:${GO_PATH_LOCAL}/bin:${GO_PATH_EXTENSION}/bin

RUN mkdir -p /usr/local/gopath_extension/{bin,vsix}
RUN echo 'source /usr/local/go/init.sh' >> /etc/profile; 
COPY --from=gopath_extension /root/go-vs-extension/bin/go-outline               /usr/local/gopath_extension/bin/
COPY --from=gopath_extension /root/go-vs-extension/bin/gocode                   /usr/local/gopath_extension/bin/
COPY --from=gopath_extension /root/go-vs-extension/bin/godef                    /usr/local/gopath_extension/bin/
COPY --from=gopath_extension /root/go-vs-extension/vsix/golang.Go-0.14.4.vsix   /usr/local/gopath_extension/vsix/
COPY --from=gopath_extension /root/go-vs-extension/vsix/cpptools-linux.vsix     /usr/local/gopath_extension/vsix/
COPY --from=gopath_extension /root/go-vs-extension/vsix/git-graph-1.30.0.vsix   /usr/local/gopath_extension/vsix/

WORKDIR /usr/local/go
