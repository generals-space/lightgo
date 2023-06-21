在 make 编译整个 golang 工程时, 将在 pkg/linux_amd64 目录生成所有标准库的 .a 文件(静态链接库, 将多个object文件(.o)文件打包成一份文件).

```
encoding/json.a
os/exec.a
fmt.a
net.a
os.a
```

在开发者编写代码时, 使用`import()`导入的内置标准库, 加载的都是这些 .a 文件.

另外, 还会将`src/pkg/runtime/runtime.h`和`src/pkg/runtime/cgocall.h`两个头文件拷贝到当前目录, 暂不知道是在哪个阶段用到的.
