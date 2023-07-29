// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include	<u.h>
#include	<libc.h>
#include	"declare.h"
#include	"y.tab.h"

// dcl -> declare 声明

// 	@param s: 重复声明的对象, 有可能是 函数名, 变量名 等.
// 	@param where: 重复对象可能的情况, "in this block", "as imported package name" 两种
//
// caller:
// 	1. declare()
// 	2. src/cmd/gc/export.c -> importsym()
// 	3. src/cmd/gc/subr_package.c -> importdot()
// 	4. src/cmd/gc/go.y -> import_stmt()
void redeclare(Sym *s, char *where)
{
	Strlit *pkgstr;
	// line1 发生重复声明错误所在的行, 如 ../../xxx.go:123
	int line1;
	// line2 同名函数初次声明所在的行, 如 ../../xxx.go:122
	int line2;

	if(s->lastlineno == 0) {
		pkgstr = s->origpkg ? s->origpkg->path : s->pkg->path;
		yyerror(
			"%S redeclared %s\n"
			"\tprevious declaration during import \"%Z\"",
			s, where, pkgstr
		);
	} else {
		line1 = parserline();
		line2 = s->lastlineno;

		// When an import and a declaration collide in separate files,
		// present the import as the "redeclared", because the declaration
		// is visible where the import is, but not vice versa.
		// See issue 4510.
		if(s->def == N) {
			line2 = line1;
			line1 = s->lastlineno;
		}

		yyerrorl(
			line1, "%S redeclared %s\n"
			"\tprevious declaration at %L",
			s, where, line2
		);
	}
}

static int vargen;

// 	@param n: 变量, 类型或函数对象
//
// caller:
// 	1. addvar()
//
// declare individual names - var, const, typ
// 
void declare(Node *n, int ctxt)
{
	Sym *s;
	int gen;
	static int typegen;

	if(ctxt == PDISCARD) {
		return;
	}

	if(isblank(n)) {
		return;
	}

	n->lineno = parserline();
	s = n->sym;

	// kludgy: typecheckok means we're past parsing. 
	// Eg genwrapper may declare out of package names later.
	if(importpkg == nil && !typecheckok && s->pkg != localpkg) {
		yyerror("cannot declare name %S", s);
	}

	if(ctxt == PEXTERN && strcmp(s->name, "init") == 0) {
		yyerror("cannot declare init - must be func", s);
	}

	gen = 0;
	if(ctxt == PEXTERN) {
		externdcl = list(externdcl, n);
		if(dflag()) {
			print("\t%L global decl %S %p\n", lineno, s, n);
		}
	} else {
		if(curfn == nil && ctxt == PAUTO) {
			fatal("automatic outside function");
		}
		if(curfn != nil) {
			curfn->dcl = list(curfn->dcl, n);
		}
		if(n->op == OTYPE) {
			gen = ++typegen;
		}
		else if(n->op == ONAME && ctxt == PAUTO && strstr(s->name, "·") == nil) {
			gen = ++vargen;
		}
		pushdcl(s);
		n->curfn = curfn;
	}
	if(ctxt == PAUTO) {
		n->xoffset = 0;
	}

	if(s->block == block) {
		// (同一个 package 下的)同名函数重复声明
		//
		// functype will print errors about duplicate function arguments.
		// Don't repeat the error here.
		if(ctxt != PPARAM && ctxt != PPARAMOUT) {
			redeclare(s, "in this block");
		}
	}

	s->block = block;
	s->lastlineno = parserline();
	s->def = n;
	n->vargen = gen;
	n->funcdepth = funcdepth;
	n->class = ctxt;

	autoexport(n, ctxt);
}

// caller:
// 	1. ifacedcl()
// 	2. funchdr()
void funcargs(Node *nt)
{
	Node *n, *nn;
	NodeList *l;
	int gen;

	if(nt->op != OTFUNC)
		fatal("funcargs %O", nt->op);

	// re-start the variable generation number
	// we want to use small numbers for the return variables,
	// so let them have the chunk starting at 1.
	vargen = count(nt->rlist);

	// declare the receiver and in arguments.
	// no n->defn because type checking of func header
	// will not fill in the types until later
	if(nt->left != N) {
		n = nt->left;
		if(n->op != ODCLFIELD)
			fatal("funcargs receiver %O", n->op);
		if(n->left != N) {
			n->left->op = ONAME;
			n->left->ntype = n->right;
			declare(n->left, PPARAM);
			if(dclcontext == PAUTO)
				n->left->vargen = ++vargen;
		}
	}
	for(l=nt->list; l; l=l->next) {
		n = l->n;
		if(n->op != ODCLFIELD)
			fatal("funcargs in %O", n->op);
		if(n->left != N) {
			n->left->op = ONAME;
			n->left->ntype = n->right;
			declare(n->left, PPARAM);
			if(dclcontext == PAUTO)
				n->left->vargen = ++vargen;
		}
	}

	// declare the out arguments.
	gen = count(nt->list);
	int i = 0;
	for(l=nt->rlist; l; l=l->next) {
		n = l->n;

		if(n->op != ODCLFIELD)
			fatal("funcargs out %O", n->op);

		if(n->left == N) {
			// give it a name so escape analysis has nodes to work with
			snprint(namebuf, sizeof(namebuf), "~anon%d", gen++);
			n->left = newname(lookup(namebuf));
			// TODO: n->left->missing = 1;
		} 

		n->left->op = ONAME;

		if(isblank(n->left)) {
			// Give it a name so we can assign to it during return.
			// preserve the original in ->orig
			nn = nod(OXXX, N, N);
			*nn = *n->left;
			n->left = nn;
			
			snprint(namebuf, sizeof(namebuf), "~anon%d", gen++);
			n->left->sym = lookup(namebuf);
		}

		n->left->ntype = n->right;
		declare(n->left, PPARAMOUT);
		if(dclcontext == PAUTO)
			n->left->vargen = ++i;
	}
}
