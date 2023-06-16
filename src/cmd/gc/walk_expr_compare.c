#include	"walk.h"

// caller:
// 	1. walkcompare() 只有这一处
static Node* eqfor(Type *t)
{
	int a;
	Node *n;
	Node *ntype;
	Sym *sym;

	// Should only arrive here with large memory or
	// a struct/array containing a non-memory field/element.
	// Small memory is handled inline, and single non-memory
	// is handled during type check (OCMPSTR etc).
	a = algtype1(t, nil);
	if(a != AMEM && a != -1)
		fatal("eqfor %T", t);

	if(a == AMEM) {
		n = syslook("memequal", 1);
		argtype(n, t);
		argtype(n, t);
		return n;
	}

	sym = typesymprefix(".eq", t);
	n = newname(sym);
	n->class = PFUNC;
	ntype = nod(OTFUNC, N, N);
	ntype->list = list(ntype->list, nod(ODCLFIELD, N, typenod(ptrto(types[TBOOL]))));
	ntype->list = list(ntype->list, nod(ODCLFIELD, N, typenod(types[TUINTPTR])));
	ntype->list = list(ntype->list, nod(ODCLFIELD, N, typenod(ptrto(t))));
	ntype->list = list(ntype->list, nod(ODCLFIELD, N, typenod(ptrto(t))));
	typecheck(&ntype, Etype);
	n->type = ntype->type;
	return n;
}

// caller:
// 	1. walkcompare() 只有这一处
static int countfield(Type *t)
{
	Type *t1;
	int n;
	
	n = 0;
	for(t1=t->type; t1!=T; t1=t1->down) {
		n++;
	}
	return n;
}

// caller:
// 	1. walkcompare() 只有这一处
void walkcompare(Node **np, NodeList **init)
{
	Node *n, *l, *r, *fn, *call, *a, *li, *ri, *expr;
	int andor, i;
	Type *t, *t1;
	static Node *tempbool;

	n = *np;
	
	// Must be comparison of array or struct.
	// Otherwise back end handles it.
	t = n->left->type;
	switch(t->etype) {
	default:
		return;
	case TARRAY:
		if(isslice(t))
			return;
		break;
	case TSTRUCT:
		break;
	}
	
	if(!islvalue(n->left) || !islvalue(n->right))
		goto hard;

	l = temp(ptrto(t));
	a = nod(OAS, l, nod(OADDR, n->left, N));
	a->right->etype = 1;  // addr does not escape
	typecheck(&a, Etop);
	*init = list(*init, a);

	r = temp(ptrto(t));
	a = nod(OAS, r, nod(OADDR, n->right, N));
	a->right->etype = 1;  // addr does not escape
	typecheck(&a, Etop);
	*init = list(*init, a);

	expr = N;
	andor = OANDAND;
	if(n->op == ONE)
		andor = OOROR;

	if(t->etype == TARRAY &&
		t->bound <= 4 &&
		issimple[t->type->etype]) {
		// Four or fewer elements of a basic type.
		// Unroll comparisons.
		for(i=0; i<t->bound; i++) {
			li = nod(OINDEX, l, nodintconst(i));
			ri = nod(OINDEX, r, nodintconst(i));
			a = nod(n->op, li, ri);
			if(expr == N)
				expr = a;
			else
				expr = nod(andor, expr, a);
		}
		if(expr == N)
			expr = nodbool(n->op == OEQ);
		typecheck(&expr, Erv);
		walkexpr(&expr, init);
		expr->type = n->type;
		*np = expr;
		return;
	}
	
	if(t->etype == TSTRUCT && countfield(t) <= 4) {
		// Struct of four or fewer fields.
		// Inline comparisons.
		for(t1=t->type; t1; t1=t1->down) {
			if(isblanksym(t1->sym))
				continue;
			li = nod(OXDOT, l, newname(t1->sym));
			ri = nod(OXDOT, r, newname(t1->sym));
			a = nod(n->op, li, ri);
			if(expr == N)
				expr = a;
			else
				expr = nod(andor, expr, a);
		}
		if(expr == N)
			expr = nodbool(n->op == OEQ);
		typecheck(&expr, Erv);
		walkexpr(&expr, init);
		expr->type = n->type;
		*np = expr;
		return;
	}
	
	// Chose not to inline, but still have addresses.
	// Call equality function directly.
	// The equality function requires a bool pointer for
	// storing its address, because it has to be callable
	// from C, and C can't access an ordinary Go return value.
	// To avoid creating many temporaries, cache one per function.
	if(tempbool == N || tempbool->curfn != curfn)
		tempbool = temp(types[TBOOL]);
	
	call = nod(OCALL, eqfor(t), N);
	a = nod(OADDR, tempbool, N);
	a->etype = 1;  // does not escape
	call->list = list(call->list, a);
	call->list = list(call->list, nodintconst(t->width));
	call->list = list(call->list, l);
	call->list = list(call->list, r);
	typecheck(&call, Etop);
	walkstmt(&call);
	*init = list(*init, call);

	// tempbool cannot be used directly as multiple comparison
	// expressions may exist in the same statement. Create another
	// temporary to hold the value (its address is not taken so it can
	// be optimized away).
	r = temp(types[TBOOL]);
	a = nod(OAS, r, tempbool);
	typecheck(&a, Etop);
	walkstmt(&a);
	*init = list(*init, a);

	if(n->op != OEQ)
		r = nod(ONOT, r, N);
	typecheck(&r, Erv);
	walkexpr(&r, init);
	*np = r;
	return;

hard:
	// Cannot take address of one or both of the operands.
	// Instead, pass directly to runtime helper function.
	// Easier on the stack than passing the address
	// of temporary variables, because we are better at reusing
	// the argument space than temporary variable space.
	fn = syslook("equal", 1);
	l = n->left;
	r = n->right;
	argtype(fn, n->left->type);
	argtype(fn, n->left->type);
	r = mkcall1(fn, n->type, init, typename(n->left->type), l, r);
	if(n->op == ONE) {
		r = nod(ONOT, r, N);
		typecheck(&r, Erv);
	}
	*np = r;
	return;
}
