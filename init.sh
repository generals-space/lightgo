#!/bin/bash

## 使用方法: 
## echo 'source /usr/local/go/init.sh' >> /etc/profile
## echo 'source /usr/local/go/init.sh' >> /root/.bashrc

export GOROOT=/usr/local/go
## dist 命令会移动到此目录
export GO_TOOL=${GOROOT}/pkg/tool
## 6g, 6l 等命令会在此目录
export GO_TOOLLIST=${GO_TOOL}/linux_amd64

## 工程内部的 gopath 目录, 也要追加到 GOPATH 路径列表中
export GO_PATH_LOCAL=${GOROOT}/gopath
## vscode golang 插件所需的子插件, 如代码补全, 智能提示等工具
export GO_PATH_EXTENSION=/usr/local/gopath_extension
## 兼容标准库
export GO_PATH_COMPATIBLE=/usr/local/go/compatible

export GOPATH=/usr/local/gopath:${GO_PATH_LOCAL}:${GO_PATH_EXTENSION}:${GO_PATH_COMPATIBLE}

## 将 dist, 6g, 6l 等命令加入 PATH
export PATH=$PATH:${GOROOT}/bin:${GO_TOOL}:${GO_TOOLLIST}
## 将各 GOPATH 下的 bin 目录加入 PATH
export PATH=$PATH:/usr/local/gopath/bin:${GO_PATH_LOCAL}/bin:${GO_PATH_EXTENSION}/bin:/usr/local/bison/bin

function gb() {
    go build -gcflags='-l -N' -o main.gobin $@
}
