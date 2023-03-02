// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <u.h>
#include <libc.h>
#include "go.h"

/*
 * look for
 *	unsafe.Sizeof
 *	unsafe.Offsetof
 *	unsafe.Alignof
 * rewrite with a constant
 */

// unsafenmagic 这是在编译阶段调用的(使用 6g *.go 进行编译).
// 在进行类型检查时, 遇到函数类型, 会先调用该函数判断是否调用了 Sizeof, Alignof, Offsetof,
// 并在这里完成对 ta 们的替换, 返回主调函数期望得到的大小值, 对齐值和偏移量等信息.
//
// caller:
// 	1. src/cmd/gc/typecheck.c -> typecheck1() 只有这一处
Node* unsafenmagic(Node *nn)
{
	Node *r, *n, *base, *r1;
	Sym *s;
	Type *t, *tr;
	// 返回值 v
	vlong v;
	Val val;
	Node *fn;
	NodeList *args;

	fn = nn->left;
	args = nn->list;

	if(safemode || fn == N || fn->op != ONAME || (s = fn->sym) == S)
		goto no;
	if(s->pkg != unsafepkg)
		goto no;

	if(args == nil) {
		yyerror("missing argument for %S", s);
		goto no;
	}
	r = args->n;

	if(strcmp(s->name, "Sizeof") == 0) {
		typecheck(&r, Erv);
		defaultlit(&r, T);
		// 如果 Sizeof() 的参数是一个结构体, 那么这里 r 就是该 struct{} 的 Node,
		// 而 tr 就是该结构体的 Type 类型对象.
		tr = r->type;
		if(tr == T) {
			goto bad;
		}
		// 计算目标类型的大小, 计算结果会存入 tr->width 字段中.
		dowidth(tr);
		v = tr->width;
		goto yes;
	}
	if(strcmp(s->name, "Offsetof") == 0) {
		// must be a selector.
		if(r->op != OXDOT)
			goto bad;
		// Remember base of selector to find it back after dot insertion.
		// Since r->left may be mutated by typechecking, check it explicitly
		// first to track it correctly.
		typecheck(&r->left, Erv);
		base = r->left;
		typecheck(&r, Erv);
		switch(r->op) {
		case ODOT:
		case ODOTPTR:
			break;
		case OCALLPART:
			yyerror("invalid expression %N: argument is a method value", nn);
			v = 0;
			goto ret;
		default:
			goto bad;
		}
		v = 0;
		// add offsets for inserted dots.
		for(r1=r; r1->left!=base; r1=r1->left) {
			switch(r1->op) {
			case ODOT:
				v += r1->xoffset;
				break;
			case ODOTPTR:
				yyerror("invalid expression %N: selector implies indirection of embedded %N", nn, r1->left);
				goto ret;
			default:
				dump("unsafenmagic", r);
				fatal("impossible %#O node after dot insertion", r1->op);
				goto bad;
			}
		}
		v += r1->xoffset;
		goto yes;
	}
	if(strcmp(s->name, "Alignof") == 0) {
		typecheck(&r, Erv);
		defaultlit(&r, T);
		// Alignof() 的参数一般是 Struct 结构体中的某个成员字段,
		// tr 就是该成员字段的 Type 类型对象.
		tr = r->type;
		if(tr == T) {
			goto bad;
		}

		// make struct { byte; T; }
		t = typ(TSTRUCT);
		t->type = typ(TFIELD);
		// 注意, 直接从 types 数组中取的 Type 对象, width, align 等属性是全部都有的,
		// 与通过 typ() 创建的空对象不一样.
		t->type->type = types[TUINT8];
		t->type->down = typ(TFIELD);
		t->type->down->type = tr;
		// compute struct widths
		dowidth(t);

		// the offset of T is its required alignment
		v = t->type->down->width;
		goto yes;
	}

no:
	return N;

bad:
	yyerror("invalid expression %N", nn);
	v = 0;
	goto ret;

yes:
	if(args->next != nil) {
		yyerror("extra arguments for %S", s);
	}
ret:
	// any side effects disappear; ignore init
	val.ctype = CTINT;
	val.u.xval = mal(sizeof(*n->val.u.xval));
	mpmovecfix(val.u.xval, v);
	n = nod(OLITERAL, N, N);
	n->orig = nn;
	n->val = val;
	n->type = types[TUINTPTR];
	nn->type = types[TUINTPTR];
	return n;
}

int isunsafebuiltin(Node *n)
{
	if(n == N || n->op != ONAME || n->sym == S || n->sym->pkg != unsafepkg)
		return 0;
	if(strcmp(n->sym->name, "Sizeof") == 0)
		return 1;
	if(strcmp(n->sym->name, "Offsetof") == 0)
		return 1;
	if(strcmp(n->sym->name, "Alignof") == 0)
		return 1;
	return 0;
}
