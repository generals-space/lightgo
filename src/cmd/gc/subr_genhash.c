#include	"subr.h"

static Node* hashfor(Type *t);
static Node* hashmem(Type *t);

/*
 * Generate a helper function to compute the hash of a value of type t.
 */
void genhash(Sym *sym, Type *t)
{
	Node *n, *fn, *np, *nh, *ni, *call, *nx, *na, *tfn;
	Node *hashel;
	Type *first, *t1;
	int old_safemode;
	int64 size, mul, offend;

	if(debug['r'])
		print("genhash %S %T\n", sym, t);

	lineno = 1;  // less confusing than end of input
	dclcontext = PEXTERN;
	markdcl();

	// func sym(h *uintptr, s uintptr, p *T)
	fn = nod(ODCLFUNC, N, N);
	fn->nname = newname(sym);
	fn->nname->class = PFUNC;
	tfn = nod(OTFUNC, N, N);
	fn->nname->ntype = tfn;

	n = nod(ODCLFIELD, newname(lookup("h")), typenod(ptrto(types[TUINTPTR])));
	tfn->list = list(tfn->list, n);
	nh = n->left;
	n = nod(ODCLFIELD, newname(lookup("s")), typenod(types[TUINTPTR]));
	tfn->list = list(tfn->list, n);
	n = nod(ODCLFIELD, newname(lookup("p")), typenod(ptrto(t)));
	tfn->list = list(tfn->list, n);
	np = n->left;

	funchdr(fn);
	typecheck(&fn->nname->ntype, Etype);

	// genhash is only called for types that have equality but
	// cannot be handled by the standard algorithms,
	// so t must be either an array or a struct.
	switch(t->etype) {
	default:
		fatal("genhash %T", t);
	case TARRAY:
		if(isslice(t))
			fatal("genhash %T", t);
		// An array of pure memory would be handled by the
		// standard algorithm, so the element type must not be
		// pure memory.
		hashel = hashfor(t->type);
		n = nod(ORANGE, N, nod(OIND, np, N));
		ni = newname(lookup("i"));
		ni->type = types[TINT];
		n->list = list1(ni);
		n->colas = 1;
		colasdefn(n->list, n);
		ni = n->list->n;

		// *h = *h<<3 | *h>>61
		n->nbody = list(n->nbody,
			nod(OAS,
				nod(OIND, nh, N),
				nod(OOR,
					nod(OLSH, nod(OIND, nh, N), nodintconst(3)),
					nod(ORSH, nod(OIND, nh, N), nodintconst(widthptr*8-3)))));

		// *h *= mul
		// Same multipliers as in runtime.memhash.
		if(widthptr == 4)
			mul = 3267000013LL;
		else
			mul = 23344194077549503LL;
		n->nbody = list(n->nbody,
			nod(OAS,
				nod(OIND, nh, N),
				nod(OMUL, nod(OIND, nh, N), nodintconst(mul))));

		// hashel(h, sizeof(p[i]), &p[i])
		call = nod(OCALL, hashel, N);
		call->list = list(call->list, nh);
		call->list = list(call->list, nodintconst(t->type->width));
		nx = nod(OINDEX, np, ni);
		nx->bounded = 1;
		na = nod(OADDR, nx, N);
		na->etype = 1;  // no escape to heap
		call->list = list(call->list, na);
		n->nbody = list(n->nbody, call);

		fn->nbody = list(fn->nbody, n);
		break;

	case TSTRUCT:
		// Walk the struct using memhash for runs of AMEM
		// and calling specific hash functions for the others.
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
			// Run memhash for fields up to this one.
			if(first != T) {
				size = offend - first->width; // first->width is offset
				hashel = hashmem(first->type);
				// hashel(h, size, &p.first)
				call = nod(OCALL, hashel, N);
				call->list = list(call->list, nh);
				call->list = list(call->list, nodintconst(size));
				nx = nod(OXDOT, np, newname(first->sym));  // TODO: fields from other packages?
				na = nod(OADDR, nx, N);
				na->etype = 1;  // no escape to heap
				call->list = list(call->list, na);
				fn->nbody = list(fn->nbody, call);

				first = T;
			}
			if(t1 == T)
				break;
			if(isblanksym(t1->sym))
				continue;

			// Run hash for this field.
			hashel = hashfor(t1->type);
			// hashel(h, size, &p.t1)
			call = nod(OCALL, hashel, N);
			call->list = list(call->list, nh);
			call->list = list(call->list, nodintconst(t1->type->width));
			nx = nod(OXDOT, np, newname(t1->sym));  // TODO: fields from other packages?
			na = nod(OADDR, nx, N);
			na->etype = 1;  // no escape to heap
			call->list = list(call->list, na);
			fn->nbody = list(fn->nbody, call);
		}
		// make sure body is not empty.
		fn->nbody = list(fn->nbody, nod(ORETURN, N, N));
		break;
	}

	if(debug['r'])
		dumplist("genhash body", fn->nbody);

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

// caller:
//  1. genhash() 只有这一处
static Node* hashfor(Type *t)
{
	int a;
	Sym *sym;
	Node *tfn, *n;

	a = algtype1(t, nil);
	switch(a) {
	case AMEM:
		return hashmem(t);
	case AINTER:
		sym = pkglookup("interhash", runtimepkg);
		break;
	case ANILINTER:
		sym = pkglookup("nilinterhash", runtimepkg);
		break;
	case ASTRING:
		sym = pkglookup("strhash", runtimepkg);
		break;
	case AFLOAT32:
		sym = pkglookup("f32hash", runtimepkg);
		break;
	case AFLOAT64:
		sym = pkglookup("f64hash", runtimepkg);
		break;
	case ACPLX64:
		sym = pkglookup("c64hash", runtimepkg);
		break;
	case ACPLX128:
		sym = pkglookup("c128hash", runtimepkg);
		break;
	default:
		sym = typesymprefix(".hash", t);
		break;
	}

	n = newname(sym);
	n->class = PFUNC;
	tfn = nod(OTFUNC, N, N);
	tfn->list = list(tfn->list, nod(ODCLFIELD, N, typenod(ptrto(types[TUINTPTR]))));
	tfn->list = list(tfn->list, nod(ODCLFIELD, N, typenod(types[TUINTPTR])));
	tfn->list = list(tfn->list, nod(ODCLFIELD, N, typenod(ptrto(t))));
	typecheck(&tfn, Etype);
	n->type = tfn->type;
	return n;
}

// caller:
//  1. genhash()
//  2. hashfor()
static Node* hashmem(Type *t)
{
	Node *tfn, *n;
	Sym *sym;
	
	sym = pkglookup("memhash", runtimepkg);

	n = newname(sym);
	n->class = PFUNC;
	tfn = nod(OTFUNC, N, N);
	tfn->list = list(tfn->list, nod(ODCLFIELD, N, typenod(ptrto(types[TUINTPTR]))));
	tfn->list = list(tfn->list, nod(ODCLFIELD, N, typenod(types[TUINTPTR])));
	tfn->list = list(tfn->list, nod(ODCLFIELD, N, typenod(ptrto(t))));
	typecheck(&tfn, Etype);
	n->type = tfn->type;
	return n;
}
