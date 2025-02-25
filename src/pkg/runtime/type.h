// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
 * Runtime type representation; master is type.go
 *
 * The Type*s here correspond 1-1 to type.go's *rtype.
 */

typedef struct Type Type;
typedef struct UncommonType UncommonType;
typedef struct InterfaceType InterfaceType;
typedef struct Method Method;
typedef struct IMethod IMethod;
typedef struct SliceType SliceType;
typedef struct FuncType FuncType;
typedef struct PtrType PtrType;

// 有对应的 go 结构 src/pkg/reflect/type.go -> rtype{}
//
// Needs to be in sync with typekind.h/CommonSize
struct Type
{
	// size 当前类型的大小, 比如 int 的大小为 8(单位为字节)
	uintptr size;
	uint32 hash;
	uint8 _unused;
	uint8 align;
	uint8 fieldAlign;
	uint8 kind;
	Alg *alg;
	void *gc;
	// string 字段表示当前类型的字符化描述, 其内容可以是:
	// 1. os.File
	// 2. struct { F uintptr; A0 **exec.Cmd }
	// 3. [1]string
	// ...等, 可以使用 %S 打印该字段
	String *string;
	// 一般只有 struct 类型才需要该字段(或是通过 type int NewInt 创建的别名类型),
	// x中存储着当前类型实现的方法, 是一个列表, 且是有序的.
	// 在 reflect 包中, Method(i) 函数查询的就是这个列表, i为该列表中的索引.
	UncommonType *x;
	Type *ptrto;
};

// 见 src/pkg/reflect/type.go -> method{}
//
struct Method
{
	String *name;
	String *pkgPath;
	Type	*mtyp;
	Type *typ;
	void (*ifn)(void);
	void (*tfn)(void);
};

// UncommonType 一般属于 struct 类型(或是通过 type int NewInt 创建的别名类型),
// 这里存储着当前类型实现的方法, 是一个列表, 且是有序的.
//
// 见 src/pkg/reflect/type.go -> uncommonType{}, ta拥有一些属于自己的函数.
struct UncommonType
{
	String *name;
	String *pkgPath;
	Slice mhdr;
	Method m[];
};

// IMethod 是"接口(interface)类型"中的方法列表,
// 与 Method{} 类型相比, 这里是只有声明没有实现的.
//
// 见 src/pkg/reflect/type.go -> imethod{}
struct IMethod
{
	String *name;
	String *pkgPath;
	Type *type;
};

// InterfaceType 一般只属于"接口(interface)"类型,
// 这里存储着当前接口需要实现的方法, 是一个列表, 且是有序的.
//
// 见 src/pkg/reflect/type.go -> interfaceType{}
struct InterfaceType
{
	Type;
	Slice mhdr;
	IMethod m[];
};

// MapType 是编译器级别的对象类型, 其中包含开发者通过 make(map[key]elem) 创建的 map
// key/value 类型信息(类型名称, 占用空间大小等), 以及实际的 Hmap 底层对象.
//
// 见 src/pkg/reflect/type.go -> mapType{}
//
struct MapType
{
	Type;
	// 开发者创建的 map 中, key 的类型信息(如 string, int)
	Type *key;
	// 与 key 类型, 是 map 中的 value 类型信息(string, int, 还可能是 struct)
	Type *elem;
	Type *bucket; // internal type representing a hash bucket
	// hmap 底层实际的 map 对象(实际存储数据的地方)
	//
	// internal type representing a Hmap
	Type *hmap;
};

// 见 src/pkg/reflect/type.go -> chanType{}
//
struct ChanType
{
	Type;
	Type *elem;
	uintptr dir;
};

// 见 src/pkg/reflect/type.go -> sliceType{}
//
struct SliceType
{
	Type;
	Type *elem;
};

// 见 src/pkg/reflect/type.go -> chanType{}
//
struct FuncType
{
	Type;
	bool dotdotdot;
	Slice in;
	Slice out;
};

// 见 src/pkg/reflect/type.go -> chanType{}
//
struct PtrType
{
	Type;
	Type *elem;
};

// Here instead of in runtime.h because it uses the type names.
bool	runtime·addfinalizer(void*, FuncVal *fn, uintptr, Type*, PtrType*);
bool	runtime·getfinalizer(void *p, bool del, FuncVal **fn, uintptr *nret, Type**, PtrType**);
