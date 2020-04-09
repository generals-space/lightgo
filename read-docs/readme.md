
`src/pkg`目录下存在`.go`的标准库源码, 但如果想修改这些源码仍然需要重新编译才能生效. (实验过`sync/mutex.go`).

编译方法

```
cd src
./make.bash
```

> 需要安装`gcc`

