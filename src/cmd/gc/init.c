// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <u.h>
#include <libc.h>
#include "go.h"

// renameinit 编译期间遇到名为 init 的函数的 Sym 对象, 将其重命名.
// 因为 golang 中同一个 package 下允许多个名为 init() 的函数存在, 因此需要给ta们加上后缀.
//
// caller:
// 	1. src/cmd/gc/go.y -> fndcl{}, 只有这一处
//
// a function named init is a special case.
// it is called by the initialization before main is run. 
// to make it unique within a package and also uncallable, 
// the name, normally "pkg.init", is altered to "pkg.init·1".
// 
Sym* renameinit(void)
{
	// 	@todo: 这里 static 变量, 是可以累加的?
	static int initgen;

	snprint(namebuf, sizeof(namebuf), "init·%d", ++initgen);
	return lookup(namebuf);
}

// caller:
// 	1. fninit() 只有这一处
/*
 * hand-craft the following initialization code
 *	var initdone· uint8 				(1)
 *	func init()					(2)
 *		if initdone· != 0 {			(3)
 *			if initdone· == 2		(4)
 *				return
 *			throw();			(5)
 *		}
 *		initdone· = 1;				(6)
 *		// over all matching imported symbols
 *			<pkg>.init()			(7)
 *		{ <init stmts> }			(8)
 *		init·<n>() // if any			(9)
 *		initdone· = 2;				(10)
 *		return					(11)
 *	}
 */
static int anyinit(NodeList *n)
{
	uint32 h;
	Sym *s;
	NodeList *l;

	// are there any interesting init statements
	for(l=n; l; l=l->next) {
		switch(l->n->op) {
		case ODCLFUNC:
		case ODCLCONST:
		case ODCLTYPE:
		case OEMPTY:
			break;
		case OAS:
			if(isblank(l->n->left) && candiscard(l->n->right))
				break;
			// fall through
		default:
			return 1;
		}
	}

	// is this main
	if(strcmp(localpkg->name, "main") == 0) {
		return 1;
	}

	// is there an explicit init function
	snprint(namebuf, sizeof(namebuf), "init·1");
	s = lookup(namebuf);
	if(s->def != N) {
		return 1;
	}

	// are there any imported init functions
	for(h=0; h<NHASH; h++) {
		for(s = hash[h]; s != S; s = s->link) {
			if(s->name[0] != 'i' || strcmp(s->name, "init") != 0) {
				continue;
			}
			if(s->def == N) {
				continue;
			}
			return 1;
		}
	}

	// then none
	return 0;
}

// 每当使用 6g 编译某个 package 时, 都会调用本函数对该 package 中 init() 函数进行处理.
// (对于一个完整的工程, 会根据各个 package 的引用顺序, 依次编译.)
//
// 	@param n: src/cmd/gc/go.h -> xtop
//
// caller:
// 	1. src/cmd/gc/lex.c -> main() 只有这一处
void fninit(NodeList *n)
{

	int i;
	Node *gatevar;
	Node *a, *b, *fn;
	NodeList *r;
	uint32 h;
	Sym *s, *initsym;

	if(debug['A']) {
		// sys.go or unsafe.go during compiler build
		return;
	}

	n = initfix(n);
	if(!anyinit(n)) {
		return;
	}

	r = nil;

	// (1)
	snprint(namebuf, sizeof(namebuf), "initdone·");
	gatevar = newname(lookup(namebuf));
	addvar(gatevar, types[TUINT8], PEXTERN);

	// (2)
	maxarg = 0;
	snprint(namebuf, sizeof(namebuf), "init");

	// 与开发者级别的各 package 中的 init() 函数不同, fn 是一个内置的全局 init() 函数.
	// 这里就是初始化了一个全局的 fn 对象, 把前者中所有的 init() 函数, 
	// 都按照 package 的引入顺序添加到这个 fn 对象的函数体中,
	// 然后在程序启动时由 runtime 统一调用.
	//
	// 见 src/pkg/runtime/proc.c -> main·init
	//
	fn = nod(ODCLFUNC, N, N);
	// lookup 查询内置的全局 init 函数, 如果没找到就初始化并返回一个空的对象.
	initsym = lookup(namebuf);
	fn->nname = newname(initsym);
	fn->nname->defn = fn;
	fn->nname->ntype = nod(OTFUNC, N, N);
	declare(fn->nname, PFUNC);
	funchdr(fn);

	// (3)
	a = nod(OIF, N, N);
	a->ntest = nod(ONE, gatevar, nodintconst(0));
	r = list(r, a);

	// (4)
	b = nod(OIF, N, N);
	b->ntest = nod(OEQ, gatevar, nodintconst(2));
	b->nbody = list1(nod(ORETURN, N, N));
	a->nbody = list1(b);

	// (5)
	b = syslook("throwinit", 0);
	b = nod(OCALL, b, N);
	a->nbody = list(a->nbody, b);

	// (6)
	a = nod(OAS, gatevar, nodintconst(1));
	r = list(r, a);

	// (7)
	// 遍历
	for(h=0; h<NHASH; h++) {
		for(s = hash[h]; s != S; s = s->link) {
			// 忽略掉函数名不为 init() 的函数
			if(s->name[0] != 'i' || strcmp(s->name, "init") != 0) {
				continue;
			}
			// 忽略掉函数体为 nil 的函数
			if(s->def == N) {
				continue;
			}
			// initsym 是上面刚创建的全局 init 主调函数, 需要避开.
			if(s == initsym) {
				continue;
			}

			// could check that it is fn of no args/returns
			a = nod(OCALL, s->def, N);
			// 	@compatible: 在 v1.5+ 版本中, 按照 package 的引入顺序执行对应的 init() 函数,
			// 在此之前各 package 的 init() 函数执行顺序都是与 package 的引入顺序相反的.
			//
			// r = list(r, a);
			// 这里将 a(init()函数) 添加到 r 的链表开头, 而不是末尾.
			r = list_insert(r, a);
		}
	}

	// (8)
	r = concat(r, n);

	// (9)
	// 遍历 localpkg 中所有的 init() 函数, 将他们都添加到 fn 中.
	// 在编译期, 编译工具会调用 renameinit() 函数将开发者级别的 init() 函数,
	// 重命名为 pkg.init·1 这种形式, 使得开发者可以在同一个 package 定义多个 init() 函数.
	//
	// could check that it is fn of no args/returns
	for(i=1;; i++) {
		snprint(namebuf, sizeof(namebuf), "init·%d", i);
		s = lookup(namebuf);
		if(s->def == N) {
			break;
		}
		a = nod(OCALL, s->def, N);
		r = list(r, a);
	}

	// (10)
	a = nod(OAS, gatevar, nodintconst(2));
	r = list(r, a);

	// (11)
	a = nod(ORETURN, N, N);
	r = list(r, a);
	exportsym(fn->nname);

	fn->nbody = r;
	funcbody(fn);

	curfn = fn;
	typecheck(&fn, Etop);
	typechecklist(r, Etop);
	curfn = nil;
	funccompile(fn, 0);
}
