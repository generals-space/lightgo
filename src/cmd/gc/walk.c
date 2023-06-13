// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
#include	"walk.h"

/*
 * walk the whole tree of the body of an expression or simple statement.
 * the types expressions are calculated.
 * compile-time constants are evaluated.
 * complex side effects like statements are appended to init
 */

// walk.c 的主线是 walk(), 然后是 walkexpr() 和 walkstmt(),
// 分别为 expression 和 statement 的缩写.

// walk 对目标 fn 中可能存在的已声明但未使用的变量进行最终类型检查
//
// 	@param fn: 这个对象应该包含了一个函数的整体作用域.
//
// caller:
// 	1. src/cmd/gc/pgen.c -> compile() 只有这一处
void walk(Node *fn)
{
	char s[50];
	NodeList *l;
	int lno;

	// 设置全局变量
	curfn = fn;

	if(debug['W']) {
		snprint(s, sizeof(s), "\nbefore %S", curfn->nname->sym);
		dumplist(s, curfn->nbody);
	}

	// 设置全局变量
	lno = lineno;

	// 对当前 fn 中可能存在的已声明但未使用的变量进行最终类型检查.
	//
	//
	// Final typecheck for any unused variables.
	// It's hard to be on the heap when not-used,
	// but best to be consistent about &~PHEAP here and below.
	for(l=fn->dcl; l; l=l->next) {
		if(l->n->op == ONAME && (l->n->class&~PHEAP) == PAUTO) {
			typecheck(&l->n, Erv | Easgn);
		}
	}

	// Propagate the used flag for typeswitch variables up to the NONAME in it's definition.
	for(l=fn->dcl; l; l=l->next) {
		if(l->n->op == ONAME && (l->n->class&~PHEAP) == PAUTO && l->n->defn && l->n->defn->op == OTYPESW && l->n->used) {
			l->n->defn->left->used++;
		}
	}

	for(l=fn->dcl; l; l=l->next) {
		// 1. 不是变量类型
		// 2. 是变量类型, 但不是局部变量(PAUTO)
		// 3. 是局部变量, 但有'&'取指针行为
		// 4. 已经被引用过
		if(l->n->op != ONAME || (l->n->class&~PHEAP) != PAUTO || l->n->sym->name[0] == '&' || l->n->used) {
			continue;
		}
		if(l->n->defn && l->n->defn->op == OTYPESW) {
			if(l->n->defn->left->used) {
				continue;
			}
			lineno = l->n->defn->left->lineno;
			if(!nostrictmode) {
				yyerror("%S declared and not used", l->n->sym);
			} else {
				mywarn("%S declared and not used\n", l->n->sym);
			}
			l->n->defn->left->used = 1; // suppress repeats
		} else {
			lineno = l->n->lineno;
			if (!nostrictmode) {
				yyerror("%S declared and not used", l->n->sym);
			} else {
				mywarn("%S declared and not used\n", l->n->sym);
			}
		}
	}

	lineno = lno;
	if(nerrors != 0) {
		return;
	}
	walkstmtlist(curfn->nbody);
	if(debug['W']) {
		snprint(s, sizeof(s), "after walk %S", curfn->nname->sym);
		dumplist(s, curfn->nbody);
	}
	heapmoves();
	if(debug['W'] && curfn->enter != nil) {
		snprint(s, sizeof(s), "enter %S", curfn->nname->sym);
		dumplist(s, curfn->enter);
	}
}

// caller:
// 	1. walkstmt()
// 	2. walkexpr()
void walkexprlist(NodeList *l, NodeList **init)
{
	for(; l; l=l->next) {
		walkexpr(&l->n, init);
	}
}

// caller:
// 	1. walkstmt()
// 	2. walkexpr()
void walkexprlistsafe(NodeList *l, NodeList **init)
{
	for(; l; l=l->next) {
		l->n = safeexpr(l->n, init);
		walkexpr(&l->n, init);
	}
}
