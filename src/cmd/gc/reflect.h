#include <u.h>
#include <libc.h>

#include "go.h"

enum {
	KindBool = 1,
	KindInt,
	KindInt8,
	KindInt16,
	KindInt32,
	KindInt64,
	KindUint,
	KindUint8,
	KindUint16,
	KindUint32,
	KindUint64,
	KindUintptr,
	KindFloat32,
	KindFloat64,
	KindComplex64,
	KindComplex128,
	KindArray,
	KindChan,
	KindFunc,
	KindInterface,
	KindMap,
	KindPtr,
	KindSlice,
	KindString,
	KindStruct,
	KindUnsafePointer,

	KindNoPointers = 1<<7,
};

// 这里用 extern 修饰, 真正声明在 reflect.c, 而不是在这里声明.
// 因为 reflect.h 被多个文件引用, 多个文件都用到了 signatlist 变量.
// 这些文件每次 include reflect.h, 都会声明一次 signatlist.
// 因此会在编译时期报错: multiple definition of `signatlist'
//
extern NodeList*	signatlist;

Type* mapbucket(Type *t);
Type* hmap(Type *t);

Sym*	dtypesym(Type*);
Sym*	dgcsym(Type*);
int dcommontype(Sym *s, int ot, Type *t);
int dextratype(Sym *sym, int off, Type *t, int ptroff);
int dgopkgpath(Sym *s, int ot, Pkg *pkg);
int sigcmp(Sig *a, Sig *b);
Sym* typelinksym(Type *t);
Sig* imethods(Type *t);
int gcinline(Type *t);
void dimportpath(Pkg *p);
