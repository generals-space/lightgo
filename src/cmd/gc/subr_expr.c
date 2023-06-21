#include	<u.h>
#include	<libc.h>
#include	"go.h"

/*
 * return side effect-free n, appending side effects to init.
 * result is assignable if n is.
 */
Node* safeexpr(Node *n, NodeList **init)
{
	Node *l;
	Node *r;
	Node *a;

	if(n == N)
		return N;

	if(n->ninit) {
		walkstmtlist(n->ninit);
		*init = concat(*init, n->ninit);
		n->ninit = nil;
	}

	switch(n->op) {
	case ONAME:
	case OLITERAL:
		return n;

	case ODOT:
		l = safeexpr(n->left, init);
		if(l == n->left)
			return n;
		r = nod(OXXX, N, N);
		*r = *n;
		r->left = l;
		typecheck(&r, Erv);
		walkexpr(&r, init);
		return r;

	case ODOTPTR:
	case OIND:
		l = safeexpr(n->left, init);
		if(l == n->left)
			return n;
		a = nod(OXXX, N, N);
		*a = *n;
		a->left = l;
		walkexpr(&a, init);
		return a;

	case OINDEX:
	case OINDEXMAP:
		l = safeexpr(n->left, init);
		r = safeexpr(n->right, init);
		if(l == n->left && r == n->right)
			return n;
		a = nod(OXXX, N, N);
		*a = *n;
		a->left = l;
		a->right = r;
		walkexpr(&a, init);
		return a;
	}

	// make a copy; must not be used as an lvalue
	if(islvalue(n))
		fatal("missing lvalue case in safeexpr: %N", n);
	return cheapexpr(n, init);
}

Node* copyexpr(Node *n, Type *t, NodeList **init)
{
	Node *a, *l;
	
	l = temp(t);
	a = nod(OAS, l, n);
	typecheck(&a, Etop);
	walkexpr(&a, init);
	*init = list(*init, a);
	return l;
}

/*
 * return side-effect free and cheap n, appending side effects to init.
 * result may not be assignable.
 */
Node* cheapexpr(Node *n, NodeList **init)
{
	switch(n->op) {
	case ONAME:
	case OLITERAL:
		return n;
	}

	return copyexpr(n, n->type, init);
}

/*
 * return n in a local variable of type t if it is not already.
 * the value is guaranteed not to change except by direct
 * assignment to it.
 */
Node* localexpr(Node *n, Type *t, NodeList **init)
{
	if(n->op == ONAME && !n->addrtaken &&
		(n->class == PAUTO || n->class == PPARAM || n->class == PPARAMOUT) &&
		convertop(n->type, t, nil) == OCONVNOP)
		return n;
	
	return copyexpr(n, t, init);
}
