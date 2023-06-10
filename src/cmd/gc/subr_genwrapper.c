#include	<u.h>
#include	<libc.h>
#include	"go.h"

static NodeList* structargs(Type **tl, int mustname);

/*
 * Generate a wrapper function to convert from
 * a receiver of type T to a receiver of type U.
 * That is,
 *
 *	func (t T) M() {
 *		...
 *	}
 *
 * already exists; this function generates
 *
 *	func (u U) M() {
 *		u.M()
 *	}
 *
 * where the types T and U are such that u.M() is valid
 * and calls the T.M method.
 * The resulting function is for use in method tables.
 *
 *	rcvr - U
 *	method - M func (t T)(), a TFIELD type struct
 *	newnam - the eventual mangled name of this function
 */
void genwrapper(Type *rcvr, Type *method, Sym *newnam, int iface)
{
	Node *this, *fn, *call, *n, *t, *pad, *dot, *as;
	NodeList *l, *args, *in, *out;
	Type *tpad, *methodrcvr;
	int isddd;
	Val v;

	if(0 && debug['r'])
		print("genwrapper rcvrtype=%T method=%T newnam=%S\n",
			rcvr, method, newnam);

	lineno = 1;	// less confusing than end of input

	dclcontext = PEXTERN;
	markdcl();

	this = nod(ODCLFIELD, newname(lookup(".this")), typenod(rcvr));
	this->left->ntype = this->right;
	in = structargs(getinarg(method->type), 1);
	out = structargs(getoutarg(method->type), 0);

	t = nod(OTFUNC, N, N);
	l = list1(this);
	if(iface && rcvr->width < types[tptr]->width) {
		// Building method for interface table and receiver
		// is smaller than the single pointer-sized word
		// that the interface call will pass in.
		// Add a dummy padding argument after the
		// receiver to make up the difference.
		tpad = typ(TARRAY);
		tpad->type = types[TUINT8];
		tpad->bound = types[tptr]->width - rcvr->width;
		pad = nod(ODCLFIELD, newname(lookup(".pad")), typenod(tpad));
		l = list(l, pad);
	}
	t->list = concat(l, in);
	t->rlist = out;

	fn = nod(ODCLFUNC, N, N);
	fn->nname = newname(newnam);
	fn->nname->defn = fn;
	fn->nname->ntype = t;
	declare(fn->nname, PFUNC);
	funchdr(fn);

	// arg list
	args = nil;
	isddd = 0;
	for(l=in; l; l=l->next) {
		args = list(args, l->n->left);
		isddd = l->n->left->isddd;
	}
	
	methodrcvr = getthisx(method->type)->type->type;

	// generate nil pointer check for better error
	if(isptr[rcvr->etype] && rcvr->type == methodrcvr) {
		// generating wrapper from *T to T.
		n = nod(OIF, N, N);
		n->ntest = nod(OEQ, this->left, nodnil());
		// these strings are already in the reflect tables,
		// so no space cost to use them here.
		l = nil;
		v.ctype = CTSTR;
		v.u.sval = strlit(rcvr->type->sym->pkg->name);  // package name
		l = list(l, nodlit(v));
		v.u.sval = strlit(rcvr->type->sym->name);  // type name
		l = list(l, nodlit(v));
		v.u.sval = strlit(method->sym->name);
		l = list(l, nodlit(v));  // method name
		call = nod(OCALL, syslook("panicwrap", 0), N);
		call->list = l;
		n->nbody = list1(call);
		fn->nbody = list(fn->nbody, n);
	}
	
	dot = adddot(nod(OXDOT, this->left, newname(method->sym)));
	
	// generate call
	if(!flag_race && isptr[rcvr->etype] && isptr[methodrcvr->etype] && method->embedded && !isifacemethod(method->type)) {
		// generate tail call: adjust pointer receiver and jump to embedded method.
		dot = dot->left;	// skip final .M
		if(!isptr[dotlist[0].field->type->etype])
			dot = nod(OADDR, dot, N);
		as = nod(OAS, this->left, nod(OCONVNOP, dot, N));
		as->right->type = rcvr;
		fn->nbody = list(fn->nbody, as);
		n = nod(ORETJMP, N, N);
		n->left = newname(methodsym(method->sym, methodrcvr, 0));
		fn->nbody = list(fn->nbody, n);
	} else {
		fn->wrapper = 1; // ignore frame for panic+recover matching
		call = nod(OCALL, dot, N);
		call->list = args;
		call->isddd = isddd;
		if(method->type->outtuple > 0) {
			n = nod(ORETURN, N, N);
			n->list = list1(call);
			call = n;
		}
		fn->nbody = list(fn->nbody, call);
	}

	if(0 && debug['r'])
		dumplist("genwrapper body", fn->nbody);

	funcbody(fn);
	curfn = fn;
	// wrappers where T is anonymous (struct{ NamedType }) can be duplicated.
	if(rcvr->etype == TSTRUCT || isptr[rcvr->etype] && rcvr->type->etype == TSTRUCT)
		fn->dupok = 1;
	typecheck(&fn, Etop);
	typechecklist(fn->nbody, Etop);
	inlcalls(fn);
	curfn = nil;
	funccompile(fn, 0);
}

/*
 * Given funarg struct list, return list of ODCLFIELD Node fn args.
 */
static NodeList* structargs(Type **tl, int mustname)
{
	Iter savet;
	Node *a, *n;
	NodeList *args;
	Type *t;
	char buf[100];
	int gen;

	args = nil;
	gen = 0;
	for(t = structfirst(&savet, tl); t != T; t = structnext(&savet)) {
		n = N;
		if(mustname && (t->sym == nil || strcmp(t->sym->name, "_") == 0)) {
			// invent a name so that we can refer to it in the trampoline
			snprint(buf, sizeof buf, ".anon%d", gen++);
			n = newname(lookup(buf));
		} else if(t->sym)
			n = newname(t->sym);
		a = nod(ODCLFIELD, n, typenod(t->type));
		a->isddd = t->isddd;
		if(n != N)
			n->isddd = t->isddd;
		args = list(args, a);
	}
	return args;
}
