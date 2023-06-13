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
static int samecheap(Node *a, Node *b)
{
	Node *ar, *br;
	while(a != N && b != N && a->op == b->op) {
		switch(a->op) {
		default:
			return 0;
		case ONAME:
			return a == b;
		case ODOT:
		case ODOTPTR:
			ar = a->right;
			br = b->right;
			if(ar->op != ONAME || br->op != ONAME || ar->sym != br->sym)
				return 0;
			break;
		case OINDEX:
			ar = a->right;
			br = b->right;
			if(!isconst(ar, CTINT) || !isconst(br, CTINT) || mpcmpfixfix(ar->val.u.xval, br->val.u.xval) != 0)
				return 0;
			break;
		}
		a = a->left;
		b = b->left;
	}
	return 0;
}

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

Node* mapfndel(char *name, Type *t)
{
	Node *fn;

	if(t->etype != TMAP)
		fatal("mapfn %T", t);
	fn = syslook(name, 1);
	argtype(fn, t->down);
	argtype(fn, t->type);
	argtype(fn, t->down);
	return fn;
}

// caller:
// 	1. walkexpr()
//
// Generate frontend part for OSLICE[3][ARR|STR]
// 
Node* sliceany(Node* n, NodeList **init)
{
	int bounded, slice3;
	Node *src, *lb, *hb, *cb, *bound, *chk, *chk0, *chk1, *chk2;
	int64 lbv, hbv, cbv, bv, w;
	Type *bt;

//	print("before sliceany: %+N\n", n);

	src = n->left;
	lb = n->right->left;
	slice3 = n->op == OSLICE3 || n->op == OSLICE3ARR;
	if(slice3) {
		hb = n->right->right->left;
		cb = n->right->right->right;
	} else {
		hb = n->right->right;
		cb = N;
	}

	bounded = n->etype;
	
	if(n->op == OSLICESTR)
		bound = nod(OLEN, src, N);
	else
		bound = nod(OCAP, src, N);

	typecheck(&bound, Erv);
	walkexpr(&bound, init);  // if src is an array, bound will be a const now.

	// static checks if possible
	bv = 1LL<<50;
	if(isconst(bound, CTINT)) {
		if(!smallintconst(bound))
			yyerror("array len too large");
		else
			bv = mpgetfix(bound->val.u.xval);
	}

	if(isconst(cb, CTINT)) {
		cbv = mpgetfix(cb->val.u.xval);
		if(cbv < 0 || cbv > bv) {
			yyerror("slice index out of bounds");
			cbv = -1;
		}
	}
	if(isconst(hb, CTINT)) {
		hbv = mpgetfix(hb->val.u.xval);
		if(hbv < 0 || hbv > bv) {
			yyerror("slice index out of bounds");
			hbv = -1;
		}
	}
	if(isconst(lb, CTINT)) {
		lbv = mpgetfix(lb->val.u.xval);
		if(lbv < 0 || lbv > bv) {
			yyerror("slice index out of bounds");
			lbv = -1;
		}
		if(lbv == 0)
			lb = N;
	}

	// dynamic checks convert all bounds to unsigned to save us the bound < 0 comparison
	// generate
	//     if hb > bound || lb > hb { panicslice() }
	chk = N;
	chk0 = N;
	chk1 = N;
	chk2 = N;

	bt = types[simtype[TUINT]];
	if(cb != N && cb->type->width > 4)
		bt = types[TUINT64];
	if(hb != N && hb->type->width > 4)
		bt = types[TUINT64];
	if(lb != N && lb->type->width > 4)
		bt = types[TUINT64];

	bound = cheapexpr(conv(bound, bt), init);

	if(cb != N) {
		cb = cheapexpr(conv(cb, bt), init);
		if(!bounded)
			chk0 = nod(OLT, bound, cb);
	} else if(slice3) {
		// When we figure out what this means, implement it.
		fatal("slice3 with cb == N"); // rejected by parser
	}
		
	if(hb != N) {
		hb = cheapexpr(conv(hb, bt), init);
		if(!bounded) {
			if(cb != N)
				chk1 = nod(OLT, cb, hb);
			else
				chk1 = nod(OLT, bound, hb);
		}
	} else if(slice3) {
		// When we figure out what this means, implement it.
		fatal("slice3 with hb == N"); // rejected by parser
	} else if(n->op == OSLICEARR) {
		hb = bound;
	} else {
		hb = nod(OLEN, src, N);
		typecheck(&hb, Erv);
		walkexpr(&hb, init);
		hb = cheapexpr(conv(hb, bt), init);
	}

	if(lb != N) {
		lb = cheapexpr(conv(lb, bt), init);
		if(!bounded)
			chk2 = nod(OLT, hb, lb);  
	}

	if(chk0 != N || chk1 != N || chk2 != N) {
		chk = nod(OIF, N, N);
		chk->nbody = list1(mkcall("panicslice", T, init));
		chk->likely = -1;
		if(chk0 != N)
			chk->ntest = chk0;
		if(chk1 != N) {
			if(chk->ntest == N)
				chk->ntest = chk1;
			else
				chk->ntest = nod(OOROR, chk->ntest, chk1);
		}
		if(chk2 != N) {
			if(chk->ntest == N)
				chk->ntest = chk2;
			else
				chk->ntest = nod(OOROR, chk->ntest, chk2);
		}
		typecheck(&chk, Etop);
		walkstmt(&chk);
		*init = concat(*init, chk->ninit);
		chk->ninit = nil;
		*init = list(*init, chk);
	}
	
	// prepare new cap, len and offs for backend cgen_slice
	// cap = bound [ - lo ]
	n->right = N;
	n->list = nil;
	if(!slice3)
		cb = bound;
	if(lb == N)
		bound = conv(cb, types[simtype[TUINT]]);
	else
		bound = nod(OSUB, conv(cb, types[simtype[TUINT]]), conv(lb, types[simtype[TUINT]]));
	typecheck(&bound, Erv);
	walkexpr(&bound, init);
	n->list = list(n->list, bound);

	// len = hi [ - lo]
	if(lb == N)
		hb = conv(hb, types[simtype[TUINT]]);
	else
		hb = nod(OSUB, conv(hb, types[simtype[TUINT]]), conv(lb, types[simtype[TUINT]]));
	typecheck(&hb, Erv);
	walkexpr(&hb, init);
	n->list = list(n->list, hb);

	// offs = [width *] lo, but omit if zero
	if(lb != N) {
		if(n->op == OSLICESTR)
			w = 1;
		else
			w = n->type->type->width;
		lb = conv(lb, types[TUINTPTR]);
		if(w > 1)
			lb = nod(OMUL, nodintconst(w), lb);
		typecheck(&lb, Erv);
		walkexpr(&lb, init);
		n->list = list(n->list, lb);
	}

//	print("after sliceany: %+N\n", n);

	return n;
}

Node* addstr(Node *n, NodeList **init)
{
	Node *r, *cat, *typstr;
	NodeList *in, *args;
	int i, count;

	count = 0;
	for(r=n; r->op == OADDSTR; r=r->left)
		count++;	// r->right
	count++;	// r

	// prepare call of runtime.catstring of type int, string, string, string
	// with as many strings as we have.
	cat = syslook("concatstring", 1);
	cat->type = T;
	cat->ntype = nod(OTFUNC, N, N);
	in = list1(nod(ODCLFIELD, N, typenod(types[TINT])));	// count
	typstr = typenod(types[TSTRING]);
	for(i=0; i<count; i++)
		in = list(in, nod(ODCLFIELD, N, typstr));
	cat->ntype->list = in;
	cat->ntype->rlist = list1(nod(ODCLFIELD, N, typstr));

	args = nil;
	for(r=n; r->op == OADDSTR; r=r->left)
		args = concat(list1(conv(r->right, types[TSTRING])), args);
	args = concat(list1(conv(r, types[TSTRING])), args);
	args = concat(list1(nodintconst(count)), args);

	r = nod(OCALL, cat, N);
	r->list = args;
	typecheck(&r, Erv);
	walkexpr(&r, init);
	r->type = n->type;

	return r;
}

// caller:
// 	1. walkexpr() 只有这一处
void walkrotate(Node **np)
{
	int w, sl, sr, s;
	Node *l, *r;
	Node *n;
	
	n = *np;

	// Want << | >> or >> | << or << ^ >> or >> ^ << on unsigned value.
	l = n->left;
	r = n->right;
	if((n->op != OOR && n->op != OXOR) ||
	   (l->op != OLSH && l->op != ORSH) ||
	   (r->op != OLSH && r->op != ORSH) ||
	   n->type == T || issigned[n->type->etype] ||
	   l->op == r->op) {
		return;
	}

	// Want same, side effect-free expression on lhs of both shifts.
	if(!samecheap(l->left, r->left))
		return;
	
	// Constants adding to width?
	w = l->type->width * 8;
	if(smallintconst(l->right) && smallintconst(r->right)) {
		if((sl=mpgetfix(l->right->val.u.xval)) >= 0 && (sr=mpgetfix(r->right->val.u.xval)) >= 0 && sl+sr == w)
			goto yes;
		return;
	}
	
	// TODO: Could allow s and 32-s if s is bounded (maybe s&31 and 32-s&31).
	return;
	
yes:
	// Rewrite left shift half to left rotate.
	if(l->op == OLSH)
		n = l;
	else
		n = r;
	n->op = OLROT;
	
	// Remove rotate 0 and rotate w.
	s = mpgetfix(n->right->val.u.xval);
	if(s == 0 || s == w)
		n = n->left;

	*np = n;
	return;
}

// caller:
// 	1. walkexpr() 只有这一处
//
// return 1 if integer n must be in range [0, max), 0 otherwise
int bounded(Node *n, int64 max)
{
	int64 v;
	int32 bits;
	int sign;

	if(n->type == T || !isint[n->type->etype])
		return 0;

	sign = issigned[n->type->etype];
	bits = 8*n->type->width;

	if(smallintconst(n)) {
		v = mpgetfix(n->val.u.xval);
		return 0 <= v && v < max;
	}

	switch(n->op) {
	case OAND:
		v = -1;
		if(smallintconst(n->left)) {
			v = mpgetfix(n->left->val.u.xval);
		} else if(smallintconst(n->right)) {
			v = mpgetfix(n->right->val.u.xval);
		}
		if(0 <= v && v < max)
			return 1;
		break;

	case OMOD:
		if(!sign && smallintconst(n->right)) {
			v = mpgetfix(n->right->val.u.xval);
			if(0 <= v && v <= max)
				return 1;
		}
		break;
	
	case ODIV:
		if(!sign && smallintconst(n->right)) {
			v = mpgetfix(n->right->val.u.xval);
			while(bits > 0 && v >= 2) {
				bits--;
				v >>= 1;
			}
		}
		break;
	
	case ORSH:
		if(!sign && smallintconst(n->right)) {
			v = mpgetfix(n->right->val.u.xval);
			if(v > bits)
				return 1;
			bits -= v;
		}
		break;
	}
	
	if(!sign && bits <= 62 && (1LL<<bits) <= max)
		return 1;
	
	return 0;
}

// caller:
// 	1. walkexpr() 只有这一处
//
void usefield(Node *n)
{
	Type *field, *l;

	if(!fieldtrack_enabled)
		return;

	switch(n->op) {
	default:
		fatal("usefield %O", n->op);
	case ODOT:
	case ODOTPTR:
		break;
	}
	
	field = n->paramfld;
	if(field == T)
		fatal("usefield %T %S without paramfld", n->left->type, n->right->sym);
	if(field->note == nil || strstr(field->note->s, "go:\"track\"") == nil)
		return;

	// dedup on list
	if(field->lastfn == curfn)
		return;
	field->lastfn = curfn;
	field->outer = n->left->type;
	if(isptr[field->outer->etype])
		field->outer = field->outer->type;
	if(field->outer->sym == S)
		yyerror("tracked field must be in named struct type");
	if(!exportname(field->sym->name))
		yyerror("tracked field must be exported (upper case)");

	l = typ(0);
	l->type = field;
	l->down = curfn->paramfld;
	curfn->paramfld = l;
}
