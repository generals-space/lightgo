# vscode-go插件

vscode: 版本: 1.71.2 提交: 74b1f979648cc44d385a2286793c226e611f59e7

vscode-go: [golang.Go-0.14.4.vsix](https://github.com/golang/vscode-go/releases/download/v0.14.4/golang.Go-0.14.4.vsix)
C/C++: [v1.12.4](https://github.com/microsoft/vscode-cpptools/releases/download/v1.12.4/cpptools-linux.vsix)
Git-Graph: [v1.30.0]()

本文的目的就是, 就算不能在线调试, 至少写代码的时候能有代码提示.

## 代码提示工具及版本

这些工具是需要通过`go install`命令安装的, 如果不想将ta们直接装在 gopath 中(毕竟 gopath 中的库是用来引用的, 而这些工具只是用来执行的), 则将ta们下载到 gotoolpath 目录中.

额外设置 go tool path, 添加到 GOPATH 变量中.

```bash
export LOCAL_GO_TOOL_PATH=${GOROOT}/gotoolpath
export GOPATH=${GOPATH}:${LOCAL_GO_TOOL_PATH}
```

同时, 在`.vscode/settings.json`中, 添加如下配置

```json
{
    "go.toolsGopath": "${workspaceFolder}/gotoolpath"
}
```

使用 git clone 下载代码

```bash
git clone https://github.com/nsf/gocode.git ${LOCAL_GO_TOOL_PATH}/src/github.com/nsf/gocode
cd ${LOCAL_GO_TOOL_PATH}/src/github.com/nsf/gocode
git checkout -b v.20150303 v.20150303
```

```bash
git clone https://github.com/ramya-rao-a/go-outline.git ${LOCAL_GO_TOOL_PATH}/src/github.com/ramya-rao-a/go-outline
cd ${LOCAL_GO_TOOL_PATH}/src/github.com/ramya-rao-a/go-outline
git checkout -b 1.0.0 1.0.0

git clone https://github.com/golang/tools.git ${LOCAL_GO_TOOL_PATH}/src/golang.org/x/tools
cd ${LOCAL_GO_TOOL_PATH}/src/golang.org/x/tools
git checkout -b release-branch.go1.7 origin/release-branch.go1.7
```

```bash
git clone https://github.com/rogpeppe/godef.git ${LOCAL_GO_TOOL_PATH}/src/github.com/rogpeppe/godef
cd ${LOCAL_GO_TOOL_PATH}/src/github.com/rogpeppe/godef
git checkout -b v1.0.0 v1.0.0
mv vendor/9fans.net ${LOCAL_GO_TOOL_PATH}/src/
```

```bash
git clone https://github.com/uudashr/gopkgs.git ${LOCAL_GO_TOOL_PATH}/src/github.com/uudashr/gopkgs
cd ${LOCAL_GO_TOOL_PATH}/src/github.com/uudashr/gopkgs
git checkout -b v2.1.2 v2.1.2
git clone https://github.com/karrick/godirwalk.git ${LOCAL_GO_TOOL_PATH}/src/github.com/karrick/godirwalk
cd ${LOCAL_GO_TOOL_PATH}/src/github.com/karrick/godirwalk
git checkout -b v1.17.0 v1.17.0
git clone https://github.com/pkg/errors.git ${LOCAL_GO_TOOL_PATH}/src/github.com/pkg/errors
cd ${LOCAL_GO_TOOL_PATH}/src/github.com/pkg/errors
git checkout -b v0.9.1 v0.9.1
```

```bash
git clone https://github.com/golang/lint.git ${LOCAL_GO_TOOL_PATH}/src/golang.org/x/lint
cd ${LOCAL_GO_TOOL_PATH}/src/golang.org/x/lint
git checkout -b gov1.2 4b1924a45790d40960cee8a5ff2957c373ed535b
```

> `github.com/sqs/goreturns`的依赖与其他插件不太好兼容, 先不装了.

```
go install github.com/nsf/gocode
go install github.com/ramya-rao-a/go-outline
go install github.com/rogpeppe/godef
go install github.com/uudashr/gopkgs
go install golang.org/x/lint
```

不过, 虽然 gopkgs 和 lint 在 go install 没有报错, 但`${LOCAL_GO_TOOL_PATH}/bin`并没有生成二进制包...

最终只有 gocode, godef, go-outline 3个.

用 git clone 下来后, 删除了一些仓库中没有用到的包, 做了一下精简, 添加到 go 本身的仓库中了.

## 
