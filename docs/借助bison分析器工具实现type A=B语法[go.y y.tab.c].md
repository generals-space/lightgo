# 借助bison分析器工具实现type A=B语法

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

centos 7 原生的 bison 版本已经是 3.x 了, 为了保证一致性, 需要从源码进行编译 2.5 版本.

```bash
yum install -y m4
cd bison-2.5
./configure --prefix=/usr/local/bison
make -j4 && make install
```

### 编译报错"gets is a security hole - use fgets instead"

```log
In file included from fseterr.h:20:0,
                 from fseterr.c:20:
./stdio.h:496:1: error: 'gets' undeclared here (not in a function)
 _GL_WARN_ON_USE (gets, "gets is a security hole - use fgets instead");
 ^
make[4]: *** [fseterr.o] Error 1
make[4]: Leaving directory `/root/bison-2.5/lib'
make[3]: *** [all-recursive] Error 1
make[3]: Leaving directory `/root/bison-2.5/lib'
make[2]: *** [all] Error 2
make[2]: Leaving directory `/root/bison-2.5/lib'
make[1]: *** [all-recursive] Error 1
make[1]: Leaving directory `/root/bison-2.5'
make: *** [all] Error 2
```

### 解决方法

编辑`bison-2.5/lib/stdio.in.h`文件

找到如下行

```c++
_GL_WARN_ON_USE (gets, "gets is a security hole - use fgets instead");
```

修改为

```c++
// _GL_WARN_ON_USE (gets, "gets is a security hole - use fgets instead");
#if defined(__GLIBC__) && !defined(__UCLIBC__) && !__GLIBC_PREREQ(2, 16)
_GL_WARN_ON_USE (gets, "gets is a security hole - use fgets instead");
#endif
```

然后重新编译即可.

> 2.5.1 编译时不会有上述错误, 不过为了与原本`go.y`中的定义一致, 还是使用 2.5 版本.

## 2. 修改 go.y 语法文件

该文件的语法类似于 makefile, bash shell 等.

重点在于`typedcl`规则块, 原来为

```makefile
typedcl:
	typedclname ntype
	{
		$$ = typedcl1($1, $2, 1);
	}
```

该块声明了`type myInt int`的类型别名定义语法, 不过其实并没有`type`关键字, 需要由主调函数提供.

- `$1`: 即`typedclname`, 表示新的类型别名
- `$2`: 即`ntype`, 表示源类型

我们要新增一种`type myInt = int`的语法, 可以将其修改为

```makefile
// type 类型定义(别名)
// 注意: 如下规则中不包含 type 关键字, type 关键字可见 typedcl() 的主调函数, 用 LTYPE 表示.
typedcl:
	// type myType int
	typedclname ntype
	{
		$$ = typedcl1($1, $2, 1);
	}
|	// type myType int
	typedclname '=' ntype
	{
		$$ = typedcl1($1, $3, 1);
	}
```

## 3. bison 重新构建 y.tab.c

```bash
cd src/cmd/gc
bison go.y --output=y.tab.c --defines=y.tab.h
```

但是生成的`y.tab.c`还需要修改一些东西, 否则 make 整体编译时会报错, 当然, 是在编译`gc`工具时报的.

### 3.1

#### 3.1.1 问题描述

```log
$ make
cmd/gc
y.tab.c: In function 'yyparse':
y.tab.c:5531:9: error: passing argument 1 of 'yyerror' discards 'const' qualifier from pointer target type [-Werror]
         yyerror (yymsgp);
         ^
In file included from go.y:28:0:
/usr/local/go/src/cmd/gc/go.h:1601:6: note: expected 'char *' but argument is of type 'const char *'
 void yyerror(char *fmt, ...);
      ^
cc1: all warnings being treated as errors
```

#### 3.1.2 解决方法

编辑 y.tab.c, 找到如下内容

```c++
        char const *yymsgp = YY_("syntax error");
        // ...省略
        yyerror (yymsgp);
```

将其修改为

```c++
        char *yymsgp = YY_("syntax error");
        // ...省略
        yyerror (yymsgp);
```

即, 移除`const`关键字

### 3.2

#### 3.2.1

```log
cmd/6g
/usr/local/go/pkg/obj/linux_amd64/libgc.a(lex.o): In function `yytinit':
/usr/local/go/src/cmd/gc/lex.c:2360: undefined reference to `yytname'
/usr/local/go/src/cmd/gc/lex.c:2359: undefined reference to `yytname'
/usr/local/go/pkg/obj/linux_amd64/libgc.a(subr_func_log.o): In function `yyerror':
/usr/local/go/src/cmd/gc/subr_func_log.c:116: undefined reference to `yystate'
/usr/local/go/src/cmd/gc/subr_func_log.c:149: undefined reference to `yystate'
collect2: error: ld returned 1 exit status
```

#### 3.2.2

编辑 y.tab.c, 找到如下内容

```c++
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] = {}
```

将其修改为

```c++
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
const char *yytname[] = {}
```

### 3.3

#### 3.3.1

```log
cmd/6g
/usr/local/go/pkg/obj/linux_amd64/libgc.a(subr_func_log.o): In function `yyerror':
/usr/local/go/src/cmd/gc/subr_func_log.c:116: undefined reference to `yystate'
/usr/local/go/src/cmd/gc/subr_func_log.c:149: undefined reference to `yystate'
collect2: error: ld returned 1 exit status
```

#### 3.3.2

编辑 y.tab.c, 找到如下内容

```c++
/* Number of syntax errors so far.  */
int yynerrs;
```

将其修改为

```c++
/* Number of syntax errors so far.  */
int yynerrs, yystate;
```
