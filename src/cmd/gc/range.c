// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
 * range
 */

#include <u.h>
#include <libc.h>
#include "go.h"

// caller:
// 	1. src/cmd/gc/typecheck1.c -> typecheck1()
void typecheckrange(Node *n)
{
	char *why;
	Type *t, *t1, *t2;
	Node *v1, *v2;
	NodeList *ll;

	// delicate little dance.  see typecheckas2
	for(ll=n->list; ll; ll=ll->next)
		if(ll->n->defn != n)
			typecheck(&ll->n, Erv | Easgn);

	typecheck(&n->right, Erv);
	if((t = n->right->type) == T)
		goto out;
	if(isptr[t->etype] && isfixedarray(t->type))
		t = t->type;
	n->type = t;

	switch(t->etype) {
	default:
		yyerror("cannot range over %lN", n->right);
		goto out;

	case TARRAY:
		t1 = types[TINT];
		t2 = t->type;
		break;

	case TMAP:
		t1 = t->down;
		t2 = t->type;
		break;

	case TCHAN:
		if(!(t->chan & Crecv)) {
			yyerror("invalid operation: range %N (receive from send-only type %T)", n->right, n->right->type);
			goto out;
		}
		t1 = t->type;
		t2 = nil;
		if(count(n->list) == 2)
			goto toomany;
		break;

	case TSTRING:
		t1 = types[TINT];
		t2 = runetype;
		break;
	}

	if(count(n->list) > 2) {
	toomany:
		yyerror("too many variables in range");
	}

	v1 = n->list->n;
	v2 = N;
	if(n->list->next)
		v2 = n->list->next->n;

	// this is not only a optimization but also a requirement in the spec.
	// "if the second iteration variable is the blank identifier, the range
	// clause is equivalent to the same clause with only the first variable
	// present."
	if(isblank(v2)) {
		n->list = list1(v1);
		v2 = N;
	}

	if(v1->defn == n)
		v1->type = t1;
	else if(v1->type != T && assignop(t1, v1->type, &why) == 0)
		yyerror("cannot assign type %T to %lN in range%s", t1, v1, why);
	if(v2) {
		if(v2->defn == n)
			v2->type = t2;
		else if(v2->type != T && assignop(t2, v2->type, &why) == 0)
			yyerror("cannot assign type %T to %lN in range%s", t2, v2, why);
	}

out:
	typechecklist(n->nbody, Etop);

	// second half of dance
	n->typecheck = 1;
	for(ll=n->list; ll; ll=ll->next)
		if(ll->n->typecheck == 0)
			typecheck(&ll->n, Erv | Easgn);
}

// caller:
// 	1. src/cmd/gc/walk_stmt.c -> walkstmt() 只有这一处
void walkrange(Node *n)
{
	Node *ohv1, *hv1, *hv2;	// hidden (old) val 1, 2
	Node *ha, *hit;	// hidden aggregate, iterator
	Node *hn, *hp;	// hidden len, pointer
	Node *hb;  // hidden bool
	Node *a, *v1, *v2;	// not hidden aggregate, val 1, 2
	Node *fn, *tmp;
	// init 变量是 `for init ; test ; incr {}` 中的 init 部分
	// body 则是 `for xxx := range XXX {}` 中的 body 部分
	// (range 毕竟是与 for {} 共同存在的, 所有 range, 最终都要当作常规的 for{} 循环处理)
	NodeList *body, *init;
	Type *th, *t;
	int lno;

	// t 指的是 `for xxx := range XXX {}` 中 XXX 对象的 Type 类型
	t = n->type;
	init = nil;

	// a 指的是 `for xxx := range XXX {}` 中 XXX 对象的 Node 类型表示
	a = n->right;
	lno = setlineno(a);
	if(t->etype == TSTRING && !eqtype(t, types[TSTRING])) {
		a = nod(OCONV, n->right, N);
		a->type = types[TSTRING];
	}

	// n->list 指的是 `for xxx := range XXX {}` 中的 xxx 部分
	v1 = n->list->n;
	v2 = N;
	if(n->list->next) {
		v2 = n->list->next->n;
	}
	// n->list has no meaning anymore, clear it
	// to avoid erroneous processing by racewalk.
	n->list = nil;
	hv2 = N;

	if(v2 == N && t->etype == TARRAY) {
		// will have just one reference to argument.
		// no need to make a potentially expensive copy.
		ha = a;
	} else {
		ha = temp(a->type);
		init = list(init, nod(OAS, ha, a));
	}

	switch(t->etype) {
		default:
		{
			fatal("walkrange");
		}
		// for i, item := range ARRAY {}
		case TARRAY:
		{
			hv1 = temp(types[TINT]);
			hn = temp(types[TINT]); // hn: 承载 array length 的长度属性
			hp = nil;

			// init 变量是 `for init ; test ; incr {}` 中的 init 部分
			init = list(init, nod(OAS, hv1, N));
			init = list(init, nod(OAS, hn, nod(OLEN, ha, N)));
			if(v2) {
				hp = temp(ptrto(n->type->type));
				tmp = nod(OINDEX, ha, nodintconst(0));
				tmp->bounded = 1;
				init = list(init, nod(OAS, hp, nod(OADDR, tmp, N)));
			}
			// 如下是对 `for init ; test ; incr {}` 中 test 和 incr 部分的处理.
			n->ntest = nod(OLT, hv1, hn);
			n->nincr = nod(OASOP, hv1, nodintconst(1));
			n->nincr->etype = OADD;

			// 运行到这里, 最终的 for{} 循环可以说是变成如下这样:
			//
			// for hv1 := 0, hn = len(ha); hv1 < hn; hv1 += 1 {}
			//
			// hv1 算是数组索引游标, hn 为数组长度

			// 处理并赋值 `for xxx := range XXX {}` 中的 body 部分
			if(v2 == N) {
				body = list1(nod(OAS, v1, hv1));
			}
			else {
				a = nod(OAS2, N, N);
				a->list = list(list1(v1), v2);
				a->rlist = list(list1(hv1), nod(OIND, hp, N));
				body = list1(a);

				tmp = nod(OADD, hp, nodintconst(t->type->width));
				tmp->type = hp->type;
				tmp->typecheck = 1;
				tmp->right->type = types[tptr];
				tmp->right->typecheck = 1;
				body = list(body, nod(OAS, hp, tmp));
			}
			break;
		}

		case TMAP:
		{
			th = typ(TARRAY);
			th->type = ptrto(types[TUINT8]);
			// see ../../pkg/runtime/hashmap.c:/hash_iter
			// Size of hash_iter in # of pointers.
			th->bound = 11;
			hit = temp(th);

			fn = syslook("mapiterinit", 1);
			argtype(fn, t->down);
			argtype(fn, t->type);
			argtype(fn, th);
			init = list(init, mkcall1(fn, T, nil, typename(t), ha, nod(OADDR, hit, N)));
			n->ntest = nod(ONE, nod(OINDEX, hit, nodintconst(0)), nodnil());

			fn = syslook("mapiternext", 1);
			argtype(fn, th);
			n->nincr = mkcall1(fn, T, nil, nod(OADDR, hit, N));

			if(v2 == N) {
				fn = syslook("mapiter1", 1);
				argtype(fn, th);
				argtype(fn, t->down);
				a = nod(OAS, v1, mkcall1(fn, t->down, nil, nod(OADDR, hit, N)));
			} else {
				fn = syslook("mapiter2", 1);
				argtype(fn, th);
				argtype(fn, t->down);
				argtype(fn, t->type);
				a = nod(OAS2, N, N);
				a->list = list(list1(v1), v2);
				a->rlist = list1(mkcall1(fn, getoutargx(fn->type), nil, nod(OADDR, hit, N)));
			}
			body = list1(a);
			break;
		}

		case TCHAN:
		{
			hv1 = temp(t->type);
			hb = temp(types[TBOOL]);

			n->ntest = nod(ONE, hb, nodbool(0));
			a = nod(OAS2RECV, N, N);
			a->typecheck = 1;
			a->list = list(list1(hv1), hb);
			a->rlist = list1(nod(ORECV, ha, N));
			n->ntest->ninit = list1(a);
			body = list1(nod(OAS, v1, hv1));
			break;
		}

		case TSTRING:
		{
			ohv1 = temp(types[TINT]);

			hv1 = temp(types[TINT]);
			init = list(init, nod(OAS, hv1, N));

			if(v2 == N)
				a = nod(OAS, hv1, mkcall("stringiter", types[TINT], nil, ha, hv1));
			else {
				hv2 = temp(runetype);
				a = nod(OAS2, N, N);
				a->list = list(list1(hv1), hv2);
				fn = syslook("stringiter2", 0);
				a->rlist = list1(mkcall1(fn, getoutargx(fn->type), nil, ha, hv1));
			}
			n->ntest = nod(ONE, hv1, nodintconst(0));
			n->ntest->ninit = list(list1(nod(OAS, ohv1, hv1)), a);

			body = list1(nod(OAS, v1, ohv1));
			if(v2 != N)
				body = list(body, nod(OAS, v2, hv2));
			break;
		}
	} // switch {} 结束

	n->op = OFOR;
	typechecklist(init, Etop);
	n->ninit = concat(n->ninit, init);
	typechecklist(n->ntest->ninit, Etop);
	typecheck(&n->ntest, Erv);
	typecheck(&n->nincr, Etop);
	typechecklist(body, Etop);
	n->nbody = concat(body, n->nbody);
	walkstmt(&n);

	lineno = lno;
}

