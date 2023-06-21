#include "typecheck.h"

// assign -> assignment 赋值语句, 包括简单赋值和多重赋值.
//
// 简单赋值: 如 x = y or x := y
// 多重赋值: 如 x, y, z = xx, yy, zz

// caller:
//  1. typecheck1()
//  1. checkassignlist()
//  1. typecheckas()
void checkassign(Node *n)
{
	if(islvalue(n)) {
		return;
	}
	if(n->op == OINDEXMAP) {
		n->etype = 1;
		return;
	}
	yyerror("cannot assign to %N", n);
}

/*
 * type check assignment.
 * if this assignment is the definition of a var on the left side,
 * fill in the var's type.
 */

// as -> assignment 赋值(简单赋值, 如 x = y or x := y)
// 与之相对的是多重赋值, 如 x, y, z = xx, yy, zz
//
// 	@param n: n->op = OAS
//
// caller:
//  1. typecheck1() 只有这一处
void typecheckas(Node *n)
{
	// delicate little dance.
	// the definition of n may refer to this assignment as its definition,
	// in which case it will call typecheckas.
	// in that case, do not call typecheck back, or it will cycle.
	// if the variable has a type (ntype) then typechecking
	// will not look at defn,
	// so it is okay (and desirable, so that the conversion below happens).
	n->left = resolve(n->left);
	if(n->left->defn != n || n->left->ntype) {
		typecheck(&n->left, Erv | Easgn);
	}

	checkassign(n->left);
	typecheck(&n->right, Erv);
	if(n->right && n->right->type != T) {
		if(n->left->type != T) {
			n->right = assignconv(n->right, n->left->type, "assignment");
		}
	}
	if(n->left->defn == n && n->left->ntype == N) {
		defaultlit(&n->right, T);
		n->left->type = n->right->type;
	}

	// second half of dance.
	// now that right is done, typecheck the left just to get it over with. 
	// see dance above.
	n->typecheck = 1;
	if(n->left->typecheck == 0) {
		typecheck(&n->left, Erv | Easgn);
	}
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. typechecklist()
// 	2. typecheckas2()
void typechecklist(NodeList *l, int top)
{
	for(; l; l=l->next) {
		typecheck(&l->n, top);
	}
}

// caller:
//  1. typecheckas2() 只有这一处
static void checkassignto(Type *src, Node *dst)
{
	char *why;

	if(assignop(src, dst->type, &why) == 0) {
		yyerror("cannot assign %T to %lN in multiple assignment%s", src, dst, why);
		return;
	}
}

// caller:
//  1. typecheckas2() 只有这一处
static void checkassignlist(NodeList *l)
{
	for(; l; l=l->next) {
		checkassign(l->n);
	}
}

// as -> assignment 赋值(多重赋值, 如 x, y, z = xx, yy, zz)
// 与之相对的是简单赋值, 如 x = y or x := y
//
// 	@param n: n->op = OAS2
//
// caller:
//  1. typecheck1() 只有这一处
void typecheckas2(Node *n)
{
	int cl, cr;
	NodeList *ll, *lr;
	Node *l, *r;
	Iter s;
	Type *t;

	for(ll=n->list; ll; ll=ll->next) {
		// delicate little dance.
		ll->n = resolve(ll->n);
		if(ll->n->defn != n || ll->n->ntype)
			typecheck(&ll->n, Erv | Easgn);
	}
	cl = count(n->list);
	cr = count(n->rlist);
	checkassignlist(n->list);
	if(cl > 1 && cr == 1)
		typecheck(&n->rlist->n, Erv | Efnstruct);
	else
		typechecklist(n->rlist, Erv);

	if(cl == cr) {
		// easy
		for(ll=n->list, lr=n->rlist; ll; ll=ll->next, lr=lr->next) {
			if(ll->n->type != T && lr->n->type != T)
				lr->n = assignconv(lr->n, ll->n->type, "assignment");
			if(ll->n->defn == n && ll->n->ntype == N) {
				defaultlit(&lr->n, T);
				ll->n->type = lr->n->type;
			}
		}
		goto out;
	}


	l = n->list->n;
	r = n->rlist->n;

	// m[i] = x, ok
	if(cl == 1 && cr == 2 && l->op == OINDEXMAP) {
		if(l->type == T)
			goto out;
		yyerror("assignment count mismatch: %d = %d (use delete)", cl, cr);
		goto out;
	}

	// x,y,z = f()
	if(cr == 1) {
		if(r->type == T)
			goto out;
		switch(r->op) {
		case OCALLMETH:
		case OCALLINTER:
		case OCALLFUNC:
			if(r->type->etype != TSTRUCT || r->type->funarg == 0)
				break;
			cr = structcount(r->type);
			if(cr != cl)
				goto mismatch;
			n->op = OAS2FUNC;
			t = structfirst(&s, &r->type);
			for(ll=n->list; ll; ll=ll->next) {
				if(ll->n->type != T)
					checkassignto(t->type, ll->n);
				if(ll->n->defn == n && ll->n->ntype == N)
					ll->n->type = t->type;
				t = structnext(&s);
			}
			goto out;
		}
	}

	// x, ok = y
	if(cl == 2 && cr == 1) {
		if(r->type == T)
			goto out;
		switch(r->op) {
		case OINDEXMAP:
			n->op = OAS2MAPR;
			goto common;
		case ORECV:
			n->op = OAS2RECV;
			goto common;
		case ODOTTYPE:
			n->op = OAS2DOTTYPE;
			r->op = ODOTTYPE2;
		common:
			if(l->type != T)
				checkassignto(r->type, l);
			if(l->defn == n)
				l->type = r->type;
			l = n->list->next->n;
			if(l->type != T)
				checkassignto(types[TBOOL], l);
			if(l->defn == n && l->ntype == N)
				l->type = types[TBOOL];
			goto out;
		}
	}

mismatch:
	yyerror("assignment count mismatch: %d = %d", cl, cr);

out:
	// second half of dance
	n->typecheck = 1;
	for(ll=n->list; ll; ll=ll->next)
		if(ll->n->typecheck == 0)
			typecheck(&ll->n, Erv | Easgn);
}
