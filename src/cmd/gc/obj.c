// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <u.h>
#include <libc.h>
#include "go.h"

/*
 * architecture-independent object file output
 */

static	void	outhist(Biobuf *b);
static	void	dumpglobls(void);

// 执行`6g main.go`进行源码编译时, 生成并输出`main.6`文件(OBJ文件)
//
// 注意: main.6 文件是可以打开的, 并不是完全的二进制文件.
//
// caller:
// 	1. src/cmd/gc/lex.c -> main() 只有这一处
void dumpobj(void)
{
	NodeList *externs, *tmp;

	bout = Bopen(outfile, OWRITE);
	if(bout == nil) {
		flusherrors();
		print("can't create %s: %r\n", outfile);
		errorexit();
	}

	// 下面通过`Bprint()`写入的部分, 可以使用`head main.6`查看(会有一点乱码信息).
	Bprint(bout, "go object %s %s %s %s\n", getgoos(), thestring, getgoversion(), expstring());
	Bprint(bout, "  exports automatically generated from\n");
	Bprint(bout, "  %s in package \"%s\"\n", curio.infile, localpkg->name);
	dumpexport();
	Bprint(bout, "\n!\n");

	outhist(bout);

	externs = nil;
	if(externdcl != nil) {
		externs = externdcl->end;
	}

	dumpglobls();
	dumptypestructs();

	// Dump extra globals.
	tmp = externdcl;
	if(externs != nil) {
		externdcl = externs->next;
	}
	dumpglobls();
	externdcl = tmp;

	dumpdata();
	dumpfuncs();

	Bterm(bout);
}

// caller:
// 	1. dumpobj() 只有这一处
static void dumpglobls(void)
{
	Node *n;
	NodeList *l;

	// add globals
	for(l=externdcl; l; l=l->next) {
		n = l->n;
		if(n->op != ONAME) {
			continue;
		}

		if(n->type == T) {
			fatal("external %N nil type\n", n);
		}
		if(n->class == PFUNC)
			continue;
		if(n->sym->pkg != localpkg)
			continue;
		dowidth(n->type);

		ggloblnod(n);
	}

	for(l=funcsyms; l; l=l->next) {
		n = l->n;
		dsymptr(n->sym, 0, n->sym->def->shortname->sym, 0);
		ggloblsym(n->sym, widthptr, 1, 1);
	}
}

void Bputname(Biobuf *b, Sym *s)
{
	Bprint(b, "%s", s->pkg->prefix);
	BPUTC(b, '.');
	Bwrite(b, s->name, strlen(s->name)+1);
}

// caller:
// 	1. outhist() 只有这一处
// 	执行`6g main.go`进行源码编译时, 生成并输出`main.6`文件(OBJ文件)
static void outzfile(Biobuf *b, char *p)
{
	// print("outzfile: %s\n", p);
	char *q;

	// 下面这个 while 循环有点类似于 linux 中的 basename 命令,
	// 寻找字符串 p 中, 以 '/' 分割的数组中的最后一个元素.
	while(p) {
		q = utfrune(p, '/');
		// 如果 q 为 nil, 表示没找到, 里面不包含 /, 可以直接使用
		if(!q) {
			zfile(b, p, strlen(p));
			return;
		}
		// 这种应该是 p 的末尾是 / 的情况吧
		if(q > p) {
			zfile(b, p, q-p);
		}
		// 指针后移, 继续寻找.
		p = q + 1;
	}
}

// caller:
// 	1. dumpobj() 只有这一处
// 	执行`6g main.go`进行源码编译时, 生成并输出`main.6`文件(OBJ文件)
static void outhist(Biobuf *b)
{
	Hist *h;
	char *p;
	char *tofree;
	int n;
	static int first = 1;
	static char *goroot, *goroot_final;

	// 忽略, 跳过
	if(first) {
		// Decide whether we need to rewrite paths from $GOROOT to $GOROOT_FINAL.
		first = 0;
		// 通过环境变量覆盖?
		goroot = getenv("GOROOT");
		goroot_final = getenv("GOROOT_FINAL");
		if(goroot == nil) {
			goroot = "";
		}
		if(goroot_final == nil) {
			goroot_final = goroot;
		}
		if(strcmp(goroot, goroot_final) == 0) {
			goroot = nil;
			goroot_final = nil;
		}
	}

	tofree = nil;
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		// print("hist name: %s\n", p);
		if(p) {
			if(goroot != nil) {
				n = strlen(goroot);
				if(strncmp(p, goroot, strlen(goroot)) == 0 && p[n] == '/') {
					tofree = smprint("%s%s", goroot_final, p+n);
					p = tofree;
				}
			}

			if(p[0] == '/') {
				// 绝对路径
				// full rooted name, like /home/rsc/dir/file.go
				zfile(b, "/", 1);	// leading "/"
				outzfile(b, p+1);
			} else {
				// 相对路径
				// relative name, like dir/file.go
				if(h->offset >= 0 && pathname && pathname[0] == '/') {
					zfile(b, "/", 1);	// leading "/"
					outzfile(b, pathname+1);
				}
				outzfile(b, p);
			}
		}
		zhist(b, h->line, h->offset);
		if(tofree) {
			free(tofree);
			tofree = nil;
		}
	}
}

void ieeedtod(uint64 *ieee, double native)
{
	double fr, ho, f;
	int exp;
	uint32 h, l;
	uint64 bits;

	if(native < 0) {
		ieeedtod(ieee, -native);
		*ieee |= 1ULL<<63;
		return;
	}
	if(native == 0) {
		*ieee = 0;
		return;
	}
	fr = frexp(native, &exp);
	f = 2097152L;		/* shouldn't use fp constants here */
	fr = modf(fr*f, &ho);
	h = ho;
	h &= 0xfffffL;
	f = 65536L;
	fr = modf(fr*f, &ho);
	l = ho;
	l <<= 16;
	l |= (int32)(fr*f);
	bits = ((uint64)h<<32) | l;
	if(exp < -1021) {
		// gradual underflow
		bits |= 1LL<<52;
		bits >>= -1021 - exp;
		exp = -1022;
	}
	bits |= (uint64)(exp+1022L) << 52;
	*ieee = bits;
}

int duint8(Sym *s, int off, uint8 v)
{
	return duintxx(s, off, v, 1);
}

int duint16(Sym *s, int off, uint16 v)
{
	return duintxx(s, off, v, 2);
}

int duint32(Sym *s, int off, uint32 v)
{
	return duintxx(s, off, v, 4);
}

int duint64(Sym *s, int off, uint64 v)
{
	return duintxx(s, off, v, 8);
}

int duintptr(Sym *s, int off, uint64 v)
{
	return duintxx(s, off, v, widthptr);
}

Sym* stringsym(char *s, int len)
{
	static int gen;
	Sym *sym;
	int off, n, m;
	struct {
		Strlit lit;
		char buf[110];
	} tmp;
	Pkg *pkg;

	if(len > 100) {
		// huge strings are made static to avoid long names
		snprint(namebuf, sizeof(namebuf), ".gostring.%d", ++gen);
		pkg = localpkg;
	} else {
		// small strings get named by their contents,
		// so that multiple modules using the same string
		// can share it.
		tmp.lit.len = len;
		memmove(tmp.lit.s, s, len);
		tmp.lit.s[len] = '\0';
		snprint(namebuf, sizeof(namebuf), "\"%Z\"", &tmp.lit);
		pkg = gostringpkg;
	}
	sym = pkglookup(namebuf, pkg);
	
	// SymUniq flag indicates that data is generated already
	if(sym->flags & SymUniq) {
		return sym;
	}
	sym->flags |= SymUniq;
	sym->def = newname(sym);

	off = 0;
	
	// string header
	off = dsymptr(sym, off, sym, widthptr+widthint);
	off = duintxx(sym, off, len, widthint);
	
	// string data
	for(n=0; n<len; n+=m) {
		m = 8;
		if(m > len-n) {
			m = len-n;
		}
		off = dsname(sym, off, s+n, m);
	}
	off = duint8(sym, off, 0);  // terminating NUL for runtime
	off = (off+widthptr-1)&~(widthptr-1);  // round to pointer alignment
	ggloblsym(sym, off, 1, 1);

	return sym;	
}
