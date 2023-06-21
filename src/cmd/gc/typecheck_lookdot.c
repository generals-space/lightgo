#include "typecheck.h"

// caller:
//  1. looktypedot()
//  2. lookdot()
Type* lookdot1(Node *errnode, Sym *s, Type *t, Type *f, int dostrcmp)
{
	Type *r;

	r = T;
	for(; f!=T; f=f->down) {
		if(dostrcmp && strcmp(f->sym->name, s->name) == 0)
			return f;
		if(f->sym != s)
			continue;
		if(r != T) {
			if(errnode)
				yyerror("ambiguous selector %N", errnode);
			else if(isptr[t->etype])
				yyerror("ambiguous selector (%T).%S", t, s);
			else
				yyerror("ambiguous selector %T.%S", t, s);
			break;
		}
		r = f;
	}
	return r;
}

// caller:
//  1. typecheck1() 只有这一处
int looktypedot(Node *n, Type *t, int dostrcmp)
{
	Type *f1, *f2;
	Sym *s;
	
	s = n->right->sym;

	if(t->etype == TINTER) {
		f1 = lookdot1(n, s, t, t->type, dostrcmp);
		if(f1 == T)
			return 0;

		n->right = methodname(n->right, t);
		n->xoffset = f1->width;
		n->type = f1->type;
		n->op = ODOTINTER;
		return 1;
	}

	// Find the base type: methtype will fail if t
	// is not of the form T or *T.
	f2 = methtype(t, 0);
	if(f2 == T)
		return 0;

	expandmeth(f2);
	f2 = lookdot1(n, s, f2, f2->xmethod, dostrcmp);
	if(f2 == T)
		return 0;

	// disallow T.m if m requires *T receiver
	if(isptr[getthisx(f2->type)->type->type->etype]
	&& !isptr[t->etype]
	&& f2->embedded != 2
	&& !isifacemethod(f2->type)) {
		yyerror("invalid method expression %N (needs pointer receiver: (*%T).%hS)", n, t, f2->sym);
		return 0;
	}

	n->right = methodname(n->right, t);
	n->xoffset = f2->width;
	n->type = f2->type;
	n->op = ODOTMETH;
	return 1;
}

// caller:
//  1. lookdot() 只有这一处
static Type* derefall(Type* t)
{
	while(t && t->etype == tptr)
		t = t->type;
	return t;
}

// caller:
//  1. typecheck1() 只有这一处
int lookdot(Node *n, Type *t, int dostrcmp)
{
	Type *f1, *f2, *tt, *rcvr;
	Sym *s;

	s = n->right->sym;

	dowidth(t);
	f1 = T;
	if(t->etype == TSTRUCT || t->etype == TINTER)
		f1 = lookdot1(n, s, t, t->type, dostrcmp);

	f2 = T;
	if(n->left->type == t || n->left->type->sym == S) {
		f2 = methtype(t, 0);
		if(f2 != T) {
			// Use f2->method, not f2->xmethod: adddot has
			// already inserted all the necessary embedded dots.
			f2 = lookdot1(n, s, f2, f2->method, dostrcmp);
		}
	}

	if(f1 != T) {
		if(f2 != T)
			yyerror("%S is both field and method",
				n->right->sym);
		if(f1->width == BADWIDTH)
			fatal("lookdot badwidth %T %p", f1, f1);
		n->xoffset = f1->width;
		n->type = f1->type;
		n->paramfld = f1;
		if(t->etype == TINTER) {
			if(isptr[n->left->type->etype]) {
				n->left = nod(OIND, n->left, N);	// implicitstar
				n->left->implicit = 1;
				typecheck(&n->left, Erv);
			}
			n->op = ODOTINTER;
		}
		return 1;
	}

	if(f2 != T) {
		tt = n->left->type;
		dowidth(tt);
		rcvr = getthisx(f2->type)->type->type;
		if(!eqtype(rcvr, tt)) {
			if(rcvr->etype == tptr && eqtype(rcvr->type, tt)) {
				checklvalue(n->left, "call pointer method on");
				if(debug['N'])
					addrescapes(n->left);
				n->left = nod(OADDR, n->left, N);
				n->left->implicit = 1;
				typecheck(&n->left, Etype|Erv);
			} else if(tt->etype == tptr && eqtype(tt->type, rcvr)) {
				n->left = nod(OIND, n->left, N);
				n->left->implicit = 1;
				typecheck(&n->left, Etype|Erv);
			} else if(tt->etype == tptr && tt->type->etype == tptr && eqtype(derefall(tt), rcvr)) {
				yyerror("calling method %N with receiver %lN requires explicit dereference", n->right, n->left);
				while(tt->etype == tptr) {
					n->left = nod(OIND, n->left, N);
					n->left->implicit = 1;
					typecheck(&n->left, Etype|Erv);
					tt = tt->type;
				}
			} else {
				fatal("method mismatch: %T for %T", rcvr, tt);
			}
		}
		n->right = methodname(n->right, n->left->type);
		n->xoffset = f2->width;
		n->type = f2->type;
//		print("lookdot found [%p] %T\n", f2->type, f2->type);
		n->op = ODOTMETH;
		return 1;
	}

	return 0;
}
