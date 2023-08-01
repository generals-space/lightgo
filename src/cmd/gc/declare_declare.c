#include	<u.h>
#include	<libc.h>

#include	"go.h"

// caller:
// 	1. src/cmd/gc/go.y -> vardcl{} 只有这一处, 变量声明时被调用.
//
// declare variables from grammar
// new_name_list (type | [type] = expr_list)
// 
NodeList* variter(NodeList *vl, Node *t, NodeList *el)
{
	int doexpr;
	Node *v, *e, *as2;
	NodeList *init;

	init = nil;
	doexpr = el != nil;

	if(count(el) == 1 && count(vl) > 1) {
		e = el->n;
		as2 = nod(OAS2, N, N);
		as2->list = vl;
		as2->rlist = list1(e);
		for(; vl; vl=vl->next) {
			v = vl->n;
			v->op = ONAME;
			declare(v, dclcontext);
			v->ntype = t;
			v->defn = as2;
			if(funcdepth > 0)
				init = list(init, nod(ODCL, v, N));
		}
		return list(init, as2);
	}
	
	for(; vl; vl=vl->next) {
		if(doexpr) {
			if(el == nil) {
				yyerror("missing expression in var declaration");
				break;
			}
			e = el->n;
			el = el->next;
		} else
			e = N;

		v = vl->n;
		v->op = ONAME;
		declare(v, dclcontext);
		v->ntype = t;

		if(e != N || funcdepth > 0 || isblank(v)) {
			if(funcdepth > 0)
				init = list(init, nod(ODCL, v, N));
			e = nod(OAS, v, e);
			init = list(init, e);
			if(e->right != N)
				v->defn = e;
		}
	}
	if(el != nil)
		yyerror("extra expression in var declaration");
	return init;
}

// caller:
// 	1. src/cmd/gc/go.y -> constdcl{}
// 	2. src/cmd/gc/go.y -> constdcl1{}
// 
// declare constants from grammar
// new_name_list [[type] = expr_list]
// 
NodeList* constiter(NodeList *vl, Node *t, NodeList *cl)
{
	Node *v, *c;
	NodeList *vv;

	vv = nil;
	if(cl == nil) {
		if(t != N)
			yyerror("const declaration cannot have type without expression");
		cl = lastconst;
		t = lasttype;
	} else {
		lastconst = cl;
		lasttype = t;
	}
	cl = listtreecopy(cl);

	for(; vl; vl=vl->next) {
		if(cl == nil) {
			yyerror("missing value in const declaration");
			break;
		}
		c = cl->n;
		cl = cl->next;

		v = vl->n;
		v->op = OLITERAL;
		declare(v, dclcontext);

		v->ntype = t;
		v->defn = c;

		vv = list(vv, nod(ODCLCONST, v, N));
	}
	if(cl != nil)
		yyerror("extra expression in const declaration");
	iota += 1;
	return vv;
}

// 为"type myType int", 声明新的类型"myType"
//
// 	@param s: myType
//
// caller:
// 	1. src/cmd/gc/go.y -> typedclname{} 只有这一处
//
// new type being defined with name s.
// 
Node* typedcl0(Sym *s)
{
	Node *n;

	n = newname(s);
	n->op = OTYPE;
	declare(n, dclcontext);
	return n;
}

// 处理 type A B 和 type A = B 两种类型别名定义.
//
// 	@param n: 类型 A, 一般是 typedcl0(A) 的返回值
// 	@param t: 类型 B
// 	@param local: 从已知的主调函数来看, 此值只有一种取值: 1
//
// caller:
// 	1. src/cmd/gc/go.y -> typedcl{} 块, 只有这一处
// 
// node n, which was returned by typedcl0
// is being declared to have uncompiled type t.
// return the ODCLTYPE node to use.
// 
Node* typedcl1(Node *n, Node *t, int local)
{
	n->ntype = t;
	n->local = local;
	return nod(ODCLTYPE, n, N);
}

// caller:
// 	1. src/cmd/gc/go.y -> structdcl{}
// 	2. src/cmd/gc/go.y -> embed{}
// 	3. src/cmd/gc/go.y -> hidden_structdcl{}
Node* embedded(Sym *s, Pkg *pkg)
{
	Node *n;
	char *name;

	// Names sometimes have disambiguation junk
	// appended after a center dot.  Discard it when
	// making the name for the embedded struct field.
	enum { CenterDot = 0xB7 };
	name = s->name;
	if(utfrune(s->name, CenterDot)) {
		name = strdup(s->name);
		*utfrune(name, CenterDot) = 0;
	}

	if(exportname(name))
		n = newname(lookup(name));
	else if(s->pkg == builtinpkg)
		// The name of embedded builtins belongs to pkg.
		n = newname(pkglookup(name, pkg));
	else
		n = newname(pkglookup(name, s->pkg));
	n = nod(ODCLFIELD, n, oldname(s));
	n->embedded = 1;
	return n;
}
