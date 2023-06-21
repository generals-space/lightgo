#include	"walk.h"

Node* convas(Node *n, NodeList **init)
{
	Type *lt, *rt;

	if(n->op != OAS)
		fatal("convas: not OAS %O", n->op);

	n->typecheck = 1;

	if(n->left == N || n->right == N)
		goto out;

	lt = n->left->type;
	rt = n->right->type;
	if(lt == T || rt == T)
		goto out;

	if(isblank(n->left)) {
		defaultlit(&n->right, T);
		goto out;
	}

	if(n->left->op == OINDEXMAP) {
		n = mkcall1(mapfn("mapassign1", n->left->left->type), T, init,
			typename(n->left->left->type),
			n->left->left, n->left->right, n->right);
		goto out;
	}

	if(eqtype(lt, rt))
		goto out;

	n->right = assignconv(n->right, lt, "assignment");
	walkexpr(&n->right, init);

out:
	ullmancalc(n);
	return n;
}

Node* conv(Node *n, Type *t)
{
	if(eqtype(n->type, t))
		return n;
	n = nod(OCONV, n, N);
	n->type = t;
	typecheck(&n, Erv);
	return n;
}

////////////////////////////////////////////////////////////////////////////////

Node* mapfn(char *name, Type *t)
{
	Node *fn;

	if(t->etype != TMAP)
		fatal("mapfn %T", t);
	fn = syslook(name, 1);
	argtype(fn, t->down);
	argtype(fn, t->type);
	argtype(fn, t->down);
	argtype(fn, t->type);
	return fn;
}

Node* chanfn(char *name, int n, Type *t)
{
	Node *fn;
	int i;

	if(t->etype != TCHAN)
		fatal("chanfn %T", t);
	fn = syslook(name, 1);
	for(i=0; i<n; i++)
		argtype(fn, t->type);
	return fn;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. mkcall()
// 	2. mkcall1()
static Node* vmkcall(Node *fn, Type *t, NodeList **init, va_list va)
{
	int i, n;
	Node *r;
	NodeList *args;

	if(fn->type == T || fn->type->etype != TFUNC)
		fatal("mkcall %N %T", fn, fn->type);

	args = nil;
	n = fn->type->intuple;
	for(i=0; i<n; i++)
		args = list(args, va_arg(va, Node*));

	r = nod(OCALL, fn, N);
	r->list = args;
	if(fn->type->outtuple > 0)
		typecheck(&r, Erv | Efnstruct);
	else
		typecheck(&r, Etop);
	walkexpr(&r, init);
	r->type = t;
	return r;
}

Node* mkcall(char *name, Type *t, NodeList **init, ...)
{
	Node *r;
	va_list va;

	va_start(va, init);
	r = vmkcall(syslook(name, 0), t, init, va);
	va_end(va);
	return r;
}

Node* mkcall1(Node *fn, Type *t, NodeList **init, ...)
{
	Node *r;
	va_list va;

	va_start(va, init);
	r = vmkcall(fn, t, init, va);
	va_end(va);
	return r;
}

////////////////////////////////////////////////////////////////////////////////
