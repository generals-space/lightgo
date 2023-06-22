go v1.13 版本新增了 errors.Is 等函数(wrap.go 文件), 需要引入 reflect 包.

但是 errors 自身是核心标准库, 编译优先级排第2位, 仅次于 runtime 包.

```go
	"pkg/runtime",
	"pkg/errors",
    // ...省略
	"pkg/reflect",
```

如果在 errors 包中再引用 reflect, 就会造成循环引用(因为 reflect 中也引用了 errors).

本来新增的 errors.Is() 等方法放在标准库外也是可以实现的, 就当作一个普通的第3方库就行. 但是有很多工程是将 errors.Is() 作为标准库调用的, 所以只能将其编译到标准库中了.

之后也尝试过先编译原 errors 包, 然后在编译完成后, 拷贝 wrap.go 到 errors 包下, 用`go install errors`重新编译安装一次, 但是会报错, 此方案不可行.

因此只能新建一个 internal 内部包, 将原本 errors 库移到这里, 在编译初始作为核心库加载, 然后再创建一个新的 errors 包, 用以实现新增的 errors.Is 函数.
