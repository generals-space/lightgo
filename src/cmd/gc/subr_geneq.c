#include	"subr.h"

static Node* eqfield(Node *p, Node *q, Node *field, Node *eq);
static Node* eqmem(Node *p, Node *q, Node *field, vlong size, Node *eq);

/*
 * Generate a helper function to check equality of two values of type t.
 */
void geneq(Sym *sym, Type *t)
{
	Node *n, *fn, *np, *neq, *nq, *tfn, *nif, *ni, *nx, *ny, *nrange;
	Type *t1, *first;
	int old_safemode;
	int64 size;
	int64 offend;

	if(debug['r'])
		print("geneq %S %T\n", sym, t);

	lineno = 1;  // less confusing than end of input
	dclcontext = PEXTERN;
	markdcl();

	// func sym(eq *bool, s uintptr, p, q *T)
	fn = nod(ODCLFUNC, N, N);
	fn->nname = newname(sym);
	fn->nname->class = PFUNC;
	tfn = nod(OTFUNC, N, N);
	fn->nname->ntype = tfn;

	n = nod(ODCLFIELD, newname(lookup("eq")), typenod(ptrto(types[TBOOL])));
	tfn->list = list(tfn->list, n);
	neq = n->left;
	n = nod(ODCLFIELD, newname(lookup("s")), typenod(types[TUINTPTR]));
	tfn->list = list(tfn->list, n);
	n = nod(ODCLFIELD, newname(lookup("p")), typenod(ptrto(t)));
	tfn->list = list(tfn->list, n);
	np = n->left;
	n = nod(ODCLFIELD, newname(lookup("q")), typenod(ptrto(t)));
	tfn->list = list(tfn->list, n);
	nq = n->left;

	funchdr(fn);

	// geneq is only called for types that have equality but
	// cannot be handled by the standard algorithms,
	// so t must be either an array or a struct.
	switch(t->etype) {
	default:
		fatal("geneq %T", t);
	case TARRAY:
		if(isslice(t))
			fatal("geneq %T", t);
		// An array of pure memory would be handled by the
		// standard memequal, so the element type must not be
		// pure memory.  Even if we unrolled the range loop,
		// each iteration would be a function call, so don't bother
		// unrolling.
		nrange = nod(ORANGE, N, nod(OIND, np, N));
		ni = newname(lookup("i"));
		ni->type = types[TINT];
		nrange->list = list1(ni);
		nrange->colas = 1;
		colasdefn(nrange->list, nrange);
		ni = nrange->list->n;
		
		// if p[i] != q[i] { *eq = false; return }
		nx = nod(OINDEX, np, ni);
		nx->bounded = 1;
		ny = nod(OINDEX, nq, ni);
		ny->bounded = 1;

		nif = nod(OIF, N, N);
		nif->ntest = nod(ONE, nx, ny);
		nif->nbody = list(nif->nbody, nod(OAS, nod(OIND, neq, N), nodbool(0)));
		nif->nbody = list(nif->nbody, nod(ORETURN, N, N));
		nrange->nbody = list(nrange->nbody, nif);
		fn->nbody = list(fn->nbody, nrange);

		// *eq = true;
		fn->nbody = list(fn->nbody, nod(OAS, nod(OIND, neq, N), nodbool(1)));
		break;

	case TSTRUCT:
		// Walk the struct using memequal for runs of AMEM
		// and calling specific equality tests for the others.
		// Skip blank-named fields.
		first = T;
		offend = 0;
		for(t1=t->type;; t1=t1->down) {
			if(t1 != T && algtype1(t1->type, nil) == AMEM && !isblanksym(t1->sym)) {
				offend = t1->width + t1->type->width;
				if(first == T)
					first = t1;
				// If it's a memory field but it's padded, stop here.
				if(ispaddedfield(t1, t->width))
					t1 = t1->down;
				else
					continue;
			}
			// Run memequal for fields up to this one.
			// TODO(rsc): All the calls to newname are wrong for
			// cross-package unexported fields.
			if(first != T) {
				if(first->down == t1) {
					fn->nbody = list(fn->nbody, eqfield(np, nq, newname(first->sym), neq));
				} else if(first->down->down == t1) {
					fn->nbody = list(fn->nbody, eqfield(np, nq, newname(first->sym), neq));
					first = first->down;
					if(!isblanksym(first->sym))
						fn->nbody = list(fn->nbody, eqfield(np, nq, newname(first->sym), neq));
				} else {
					// More than two fields: use memequal.
					size = offend - first->width; // first->width is offset
					fn->nbody = list(fn->nbody, eqmem(np, nq, newname(first->sym), size, neq));
				}
				first = T;
			}
			if(t1 == T)
				break;
			if(isblanksym(t1->sym))
				continue;

			// Check this field, which is not just memory.
			fn->nbody = list(fn->nbody, eqfield(np, nq, newname(t1->sym), neq));
		}

		// *eq = true;
		fn->nbody = list(fn->nbody, nod(OAS, nod(OIND, neq, N), nodbool(1)));
		break;
	}

	if(debug['r'])
		dumplist("geneq body", fn->nbody);

	funcbody(fn);
	curfn = fn;
	fn->dupok = 1;
	typecheck(&fn, Etop);
	typechecklist(fn->nbody, Etop);
	curfn = nil;
	
	// Disable safemode while compiling this code: the code we
	// generate internally can refer to unsafe.Pointer.
	// In this case it can happen if we need to generate an ==
	// for a struct containing a reflect.Value, which itself has
	// an unexported field of type unsafe.Pointer.
	old_safemode = safemode;
	safemode = 0;
	funccompile(fn, 0);
	safemode = old_safemode;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
//  1. geneq()
//
// Return node for
//	if p.field != q.field { *eq = false; return }
static Node* eqfield(Node *p, Node *q, Node *field, Node *eq)
{
	Node *nif, *nx, *ny;

	nx = nod(OXDOT, p, field);
	ny = nod(OXDOT, q, field);
	nif = nod(OIF, N, N);
	nif->ntest = nod(ONE, nx, ny);
	nif->nbody = list(nif->nbody, nod(OAS, nod(OIND, eq, N), nodbool(0)));
	nif->nbody = list(nif->nbody, nod(ORETURN, N, N));
	return nif;
}

// caller:
//  1. eqmem() 只有这一处
static Node* eqmemfunc(vlong size, Type *type)
{
	char buf[30];
	Node *fn;

	switch(size) {
	default:
		fn = syslook("memequal", 1);
		break;
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		snprint(buf, sizeof buf, "memequal%d", (int)size*8);
		fn = syslook(buf, 1);
		break;
	}
	argtype(fn, type);
	argtype(fn, type);
	return fn;
}

// caller:
//  1. geneq() 只有这一处
//
// Return node for
//	if memequal(size, &p.field, &q.field, eq); !*eq { return }
static Node* eqmem(Node *p, Node *q, Node *field, vlong size, Node *eq)
{
	Node *nif, *nx, *ny, *call;

	nx = nod(OADDR, nod(OXDOT, p, field), N);
	nx->etype = 1;  // does not escape
	ny = nod(OADDR, nod(OXDOT, q, field), N);
	ny->etype = 1;  // does not escape
	typecheck(&nx, Erv);
	typecheck(&ny, Erv);

	call = nod(OCALL, eqmemfunc(size, nx->type->type), N);
	call->list = list(call->list, eq);
	call->list = list(call->list, nodintconst(size));
	call->list = list(call->list, nx);
	call->list = list(call->list, ny);

	nif = nod(OIF, N, N);
	nif->ninit = list(nif->ninit, call);
	nif->ntest = nod(ONOT, nod(OIND, eq, N), N);
	nif->nbody = list(nif->nbody, nod(ORETURN, N, N));
	return nif;
}
