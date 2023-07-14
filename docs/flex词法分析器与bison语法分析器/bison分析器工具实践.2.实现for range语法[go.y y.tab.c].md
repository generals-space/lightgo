# bison分析器工具实践.2.实现for range语法[go.y y.tab.c]

参考文章

1. [bison源码包列表](https://ftp.gnu.org/gnu/bison/)
2. [How to download and install Bison (GNU Parser Generator) on Ubuntu Linux](https://geeksww.com/tutorials/miscellaneous/bison_gnu_parser_generator/installation/installing_bison_gnu_parser_generator_ubuntu_linux.php)
    - bison 编译方法
3. [CentOS7 编译安装libiconv报错：./stdio.h:1010:1: error: 'gets' undeclared here (not in a function)](https://www.cnblogs.com/ybyqjzl/articles/10370231.html)
    - 与参考文章3方法相同
4. [centos7下编译安装libiconv-1.14 error: ‘gets’ undeclared here (not in a function)](https://www.rootop.org/pages/3532.html)
    - 与参考文章2方法相同
5. [GCC compiler and converting const char* to char *](https://stackoverflow.com/questions/8905364/gcc-compiler-and-converting-const-char-to-char)
6. [const分析](https://blog.csdn.net/zqs656546/article/details/107063373)
7. [解决 expected 'char * const*' but argument is of type 'char **'](http://lifeislife.cn/2021/09/08/%E8%A7%A3%E5%86%B3expected-char-const-but-argument-is-of-type-char/)

目前 golang v1.2 只支持如下的 type 类型别名定义方式

```go
type (
	myInt int
	myBool bool
)
type myString string
```

在 golang v1.9.0 版本时, 新增了如下同样功能的语法

```go
type (
	myInt = int
	myBool = bool
)
type myString = string
```

## 1. 构建 bison-2.5 工具

见上一篇文章

## 2. 修改 go.y 语法文件

该文件的语法类似于 makefile, bash shell 等.

重点在于`range_stmt`规则块, 原来有2种语法模式.

```makefile
range_stmt:
	// k, v = range XXX
	expr_list '=' LRANGE expr {}
	// LCOLAS 即为 ':=' 符号
	// item, ok := range XXX
|	expr_list LCOLAS LRANGE expr {}
```

新增如下块

```
	// for range XXX 
|	LRANGE expr
	{
		$$ = nod(ORANGE, N, $2);
		$$->list = list1(nod(ONAME, N, N));
		$$->etype = 0;	// := flag
	}
```

这里模拟了第一种情况中的 `for _ = range XXX`, 只不过`=`可以省略, 而`_`部分通过`list1(nod(ONAME, N, N))`注入, 这样就不需要开发者显式写了.

> `_` 用 ONAME 表示.

## 3. bison 重新构建 y.tab.c

```bash
cd src/cmd/gc
bison go.y --output=y.tab.c --defines=y.tab.h
```

但是生成的`y.tab.c`还需要修改一些东西, 否则 make 整体编译时会报错, 当然, 是在编译`gc`工具时报的.

见上一篇文章.
