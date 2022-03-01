将当前目录追加到GOPATH路径列表中

```
export GOPATH=$GOPATH:$(pwd)
```

这样可以不用把代码手动移动到原来的 GOPATH 目录下就可以执行了.
