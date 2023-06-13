#include	"walk.h"

// caller:
//  1. ascompatee() 只有这一处
static Node* ascompatee1(int op, Node *l, Node *r, NodeList **init)
{
	Node *n;
	USED(op);
	
	// convas will turn map assigns into function calls,
	// making it impossible for reorder3 to work.
	n = nod(OAS, l, r);
	if(l->op == OINDEXMAP)
		return n;

	return convas(n, init);
}

////////////////////////////////////////////////////////////////////////////////

NodeList* ascompatee(int op, NodeList *nl, NodeList *nr, NodeList **init)
{
	NodeList *ll, *lr, *nn;

	/*
	 * check assign expression list to
	 * a expression list. called in
	 *	expr-list = expr-list
	 */

	// ensure order of evaluation for function calls
	for(ll=nl; ll; ll=ll->next)
		ll->n = safeexpr(ll->n, init);
	for(lr=nr; lr; lr=lr->next)
		lr->n = safeexpr(lr->n, init);

	nn = nil;
	for(ll=nl, lr=nr; ll && lr; ll=ll->next, lr=lr->next) {
		// Do not generate 'x = x' during return. See issue 4014.
		if(op == ORETURN && ll->n == lr->n)
			continue;
		nn = list(nn, ascompatee1(op, ll->n, lr->n, init));
	}

	// cannot happen: caller checked that lists had same length
	if(ll || lr)
		yyerror(
			"error in shape across %+H %O %+H / %d %d [%s]", 
			nl, op, nr, count(nl), count(nr), curfn->nname->sym->name
		);
	return nn;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. ascompatte() 只有这一处
//
// package all the arguments that match a ... T parameter into a []T.
// 
static NodeList* mkdotargslice(NodeList *lr0, NodeList *nn, Type *l, int fp, NodeList **init, int esc)
{
	Node *a, *n;
	Type *tslice;

	tslice = typ(TARRAY);
	tslice->type = l->type->type;
	tslice->bound = -1;

	if(count(lr0) == 0) {
		n = nodnil();
		n->type = tslice;
	} else {
		n = nod(OCOMPLIT, N, typenod(tslice));
		n->list = lr0;
		n->esc = esc;
		typecheck(&n, Erv);
		if(n->type == T)
			fatal("mkdotargslice: typecheck failed");
		walkexpr(&n, init);
	}

	a = nod(OAS, nodarg(l, fp), n);
	nn = list(nn, convas(a, init));
	return nn;
}

// caller:
// 	1. ascompatte() 只有这一处
//
// helpers for shape errors
// 
static char* dumptypes(Type **nl, char *what)
{
	int first;
	Type *l;
	Iter savel;
	Fmt fmt;

	fmtstrinit(&fmt);
	fmtprint(&fmt, "\t");
	first = 1;
	for(l = structfirst(&savel, nl); l != T; l = structnext(&savel)) {
		if(first)
			first = 0;
		else
			fmtprint(&fmt, ", ");
		fmtprint(&fmt, "%T", l);
	}
	if(first)
		fmtprint(&fmt, "[no arguments %s]", what);
	return fmtstrflush(&fmt);
}

// caller:
// 	1. ascompatte() 只有这一处
//
static char* dumpnodetypes(NodeList *l, char *what)
{
	int first;
	Node *r;
	Fmt fmt;

	fmtstrinit(&fmt);
	fmtprint(&fmt, "\t");
	first = 1;
	for(; l; l=l->next) {
		r = l->n;
		if(first)
			first = 0;
		else
			fmtprint(&fmt, ", ");
		fmtprint(&fmt, "%T", r->type);
	}
	if(first)
		fmtprint(&fmt, "[no arguments %s]", what);
	return fmtstrflush(&fmt);
}

/*
 * check assign expression list to a type list.
 * called in
 *	return expr-list
 *	func(expr-list)
 */
NodeList* ascompatte(int op, Node *call, int isddd, Type **nl, NodeList *lr, int fp, NodeList **init)
{
	int esc;
	Type *l, *ll;
	Node *r, *a;
	NodeList *nn, *lr0, *alist;
	Iter savel;
	char *l1, *l2;

	lr0 = lr;
	l = structfirst(&savel, nl);
	r = N;
	if(lr)
		r = lr->n;
	nn = nil;

	// f(g()) where g has multiple return values
	if(r != N && lr->next == nil && r->type->etype == TSTRUCT && r->type->funarg) {
		// optimization - can do block copy
		if(eqtypenoname(r->type, *nl)) {
			a = nodarg(*nl, fp);
			a->type = r->type;
			nn = list1(convas(nod(OAS, a, r), init));
			goto ret;
		}

		// conversions involved.
		// copy into temporaries.
		alist = nil;
		for(l=structfirst(&savel, &r->type); l; l=structnext(&savel)) {
			a = temp(l->type);
			alist = list(alist, a);
		}
		a = nod(OAS2, N, N);
		a->list = alist;
		a->rlist = lr;
		typecheck(&a, Etop);
		walkstmt(&a);
		*init = list(*init, a);
		lr = alist;
		r = lr->n;
		l = structfirst(&savel, nl);
	}

loop:
	if(l != T && l->isddd) {
		// the ddd parameter must be last
		ll = structnext(&savel);
		if(ll != T)
			yyerror("... must be last argument");

		// special case --
		// only if we are assigning a single ddd
		// argument to a ddd parameter then it is
		// passed thru unencapsulated
		if(r != N && lr->next == nil && isddd && eqtype(l->type, r->type)) {
			a = nod(OAS, nodarg(l, fp), r);
			a = convas(a, init);
			nn = list(nn, a);
			goto ret;
		}

		// normal case -- make a slice of all
		// remaining arguments and pass it to
		// the ddd parameter.
		esc = EscUnknown;
		if(call->right)
			esc = call->right->esc;
		nn = mkdotargslice(lr, nn, l, fp, init, esc);
		goto ret;
	}

	if(l == T || r == N) {
		if(l != T || r != N) {
			l1 = dumptypes(nl, "expected");
			l2 = dumpnodetypes(lr0, "given");
			if(l != T)
				yyerror("not enough arguments to %O\n%s\n%s", op, l1, l2);
			else
				yyerror("too many arguments to %O\n%s\n%s", op, l1, l2);
		}
		goto ret;
	}

	a = nod(OAS, nodarg(l, fp), r);
	a = convas(a, init);
	nn = list(nn, a);

	l = structnext(&savel);
	r = N;
	lr = lr->next;
	if(lr != nil)
		r = lr->n;
	goto loop;

ret:
	for(lr=nn; lr; lr=lr->next)
		lr->n->typecheck = 1;
	return nn;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. ascompatet()
//
// l is an lv and rt is the type of an rv
// return 1 if this implies a function call
// evaluating the lv or a function call
// in the conversion of the types
// 
static int fncall(Node *l, Type *rt)
{
	if(l->ullman >= UINF || l->op == OINDEXMAP)
		return 1;
	if(eqtype(l->type, rt))
		return 0;
	return 1;
}

NodeList* ascompatet(int op, NodeList *nl, Type **nr, int fp, NodeList **init)
{
	Node *l, *tmp, *a;
	NodeList *ll;
	Type *r;
	Iter saver;
	int ucount;
	NodeList *nn, *mm;

	USED(op);

	/*
	 * check assign type list to
	 * a expression list. called in
	 *	expr-list = func()
	 */
	r = structfirst(&saver, nr);
	nn = nil;
	mm = nil;
	ucount = 0;
	for(ll=nl; ll; ll=ll->next) {
		if(r == T)
			break;
		l = ll->n;
		if(isblank(l)) {
			r = structnext(&saver);
			continue;
		}

		// any lv that causes a fn call must be
		// deferred until all the return arguments
		// have been pulled from the output arguments
		if(fncall(l, r->type)) {
			tmp = temp(r->type);
			typecheck(&tmp, Erv);
			a = nod(OAS, l, tmp);
			a = convas(a, init);
			mm = list(mm, a);
			l = tmp;
		}

		a = nod(OAS, l, nodarg(r, fp));
		a = convas(a, init);
		ullmancalc(a);
		if(a->ullman >= UINF)
			ucount++;
		nn = list(nn, a);
		r = structnext(&saver);
	}

	if(ll != nil || r != T)
		yyerror(
			"ascompatet: assignment count mismatch: %d = %d",
			count(nl), structcount(*nr)
		);

	if(ucount) {
		fatal("ascompatet: too many function calls evaluating parameters");
	}
	return concat(nn, mm);
}
