#!/bin/bash

## 使用方法: source init.sh

export GOROOT=$(pwd)
export GO_BIN=${GOROOT}/bin
export GO_TOOL=${GOROOT}/pkg/tool           ## dist 命令会移动到此目录
export GO_TOOLLIST=${GO_TOOL}/linux_amd64   ## 6g, 6l 等命令会在此目录

## 将本地 gopath 目录追加到GOPATH路径列表中
export LOCAL_GO_PATH=${GOROOT}/gopath

export PATH=$PATH:${GO_BIN}:${GO_TOOL}:${GO_TOOLLIST}
export GOPATH=$GOPATH:${LOCAL_GO_PATH}
