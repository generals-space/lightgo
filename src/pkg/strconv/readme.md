# strconv

参考文章

1. [Go 进制转换](https://my.oschina.net/tsh/blog/1619887)
2. [Go语言---strconv包](https://blog.csdn.net/li_101357/article/details/80252653)

引用参考文章1中的说法

`strconv`包括四类函数:

1. `Format`类, 例如`FormatBool(b bool) string`, 将`bool`, `float`, `int`, `uint`类型的转换为`string`
    - `Itoa`是`FormatInt`的封装;
2. `Parse`类, 例如`ParseBool(str string)(value bool, err error)`将字符串转换为`bool`, `float`, `int`, `uint`类型的值, `err`指定是否转换成功
    - `Atoi`是`ParseInt`的封装;
3. `Quote`类, 用于将目标字符串中指定范围的字符转换成转义字符(`\t`, `\n`, `\xFF`, `\u0100`);
4. `Append`类, 目前没看到实际意义;
    - 以`AppendBool(dst []byte, b bool`)为例, 假设`b`为`true`, ta是把`true`转换成字符串"true", 然后再转换成`[]byte{116 114 117 101}`, 追加到`dst`末尾...

