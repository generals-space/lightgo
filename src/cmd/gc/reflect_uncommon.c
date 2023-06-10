#include "reflect.h"

static Sig* methods(Type *t);

/*
 * uncommonType
 * ../../pkg/runtime/type.go:/uncommonType
 */
int dextratype(Sym *sym, int off, Type *t, int ptroff)
{
	int ot, n;
	Sym *s;
	Sig *a, *m;

	m = methods(t);
	if(t->sym == nil && m == nil)
		return off;

	// fill in *extraType pointer in header
	dsymptr(sym, ptroff, sym, off);

	n = 0;
	for(a=m; a; a=a->link) {
		dtypesym(a->type);
		n++;
	}

	ot = off;
	s = sym;
	if(t->sym) {
		ot = dgostringptr(s, ot, t->sym->name);
		if(t != types[t->etype] && t != errortype)
			ot = dgopkgpath(s, ot, t->sym->pkg);
		else
			ot = dgostringptr(s, ot, nil);
	} else {
		ot = dgostringptr(s, ot, nil);
		ot = dgostringptr(s, ot, nil);
	}

	// slice header
	ot = dsymptr(s, ot, s, ot + widthptr + 2*widthint);
	ot = duintxx(s, ot, n, widthint);
	ot = duintxx(s, ot, n, widthint);

	// methods
	for(a=m; a; a=a->link) {
		// method
		// ../../pkg/runtime/type.go:/method
		ot = dgostringptr(s, ot, a->name);
		ot = dgopkgpath(s, ot, a->pkg);
		ot = dsymptr(s, ot, dtypesym(a->mtype), 0);
		ot = dsymptr(s, ot, dtypesym(a->type), 0);
		if(a->isym)
			ot = dsymptr(s, ot, a->isym, 0);
		else
			ot = duintptr(s, ot, 0);
		if(a->tsym)
			ot = dsymptr(s, ot, a->tsym, 0);
		else
			ot = duintptr(s, ot, 0);
	}

	return ot;
}

static Sig* lsort(Sig *l, int(*f)(Sig*, Sig*))
{
	Sig *l1, *l2, *le;

	if(l == 0 || l->link == 0)
		return l;

	l1 = l;
	l2 = l;
	for(;;) {
		l2 = l2->link;
		if(l2 == 0)
			break;
		l2 = l2->link;
		if(l2 == 0)
			break;
		l1 = l1->link;
	}

	l2 = l1->link;
	l1->link = 0;
	l1 = lsort(l, f);
	l2 = lsort(l2, f);

	/* set up lead element */
	if((*f)(l1, l2) < 0) {
		l = l1;
		l1 = l1->link;
	} else {
		l = l2;
		l2 = l2->link;
	}
	le = l;

	for(;;) {
		if(l1 == 0) {
			while(l2) {
				le->link = l2;
				le = l2;
				l2 = l2->link;
			}
			le->link = 0;
			break;
		}
		if(l2 == 0) {
			while(l1) {
				le->link = l1;
				le = l1;
				l1 = l1->link;
			}
			break;
		}
		if((*f)(l1, l2) < 0) {
			le->link = l1;
			le = l1;
			l1 = l1->link;
		} else {
			le->link = l2;
			le = l2;
			l2 = l2->link;
		}
	}
	le->link = 0;
	return l;
}

/*
 * return methods of non-interface type t, sorted by name.
 * generates stub functions as needed.
 */
static Sig* methods(Type *t)
{
	Type *f, *mt, *it, *this;
	Sig *a, *b;
	Sym *method;

	// method type
	mt = methtype(t, 0);
	if(mt == T)
		return nil;
	expandmeth(mt);

	// type stored in interface word
	it = t;
	if(it->width > widthptr)
		it = ptrto(t);

	// make list of methods for t,
	// generating code if necessary.
	a = nil;
	for(f=mt->xmethod; f; f=f->down) {
		if(f->etype != TFIELD)
			fatal("methods: not field %T", f);
		if (f->type->etype != TFUNC || f->type->thistuple == 0)
			fatal("non-method on %T method %S %T\n", mt, f->sym, f);
		if (!getthisx(f->type)->type)
			fatal("receiver with no type on %T method %S %T\n", mt, f->sym, f);
		if(f->nointerface)
			continue;

		method = f->sym;
		if(method == nil)
			continue;

		// get receiver type for this particular method.
		// if pointer receiver but non-pointer t and
		// this is not an embedded pointer inside a struct,
		// method does not apply.
		this = getthisx(f->type)->type->type;
		if(isptr[this->etype] && this->type == t)
			continue;
		if(isptr[this->etype] && !isptr[t->etype]
		&& f->embedded != 2 && !isifacemethod(f->type))
			continue;

		b = mal(sizeof(*b));
		b->link = a;
		a = b;

		a->name = method->name;
		if(!exportname(method->name)) {
			if(method->pkg == nil)
				fatal("methods: missing package");
			a->pkg = method->pkg;
		}
		a->isym = methodsym(method, it, 1);
		a->tsym = methodsym(method, t, 0);
		a->type = methodfunc(f->type, t);
		a->mtype = methodfunc(f->type, nil);

		if(!(a->isym->flags & SymSiggen)) {
			a->isym->flags |= SymSiggen;
			if(!eqtype(this, it) || this->width < types[tptr]->width) {
				compiling_wrappers = 1;
				genwrapper(it, f, a->isym, 1);
				compiling_wrappers = 0;
			}
		}

		if(!(a->tsym->flags & SymSiggen)) {
			a->tsym->flags |= SymSiggen;
			if(!eqtype(this, t)) {
				compiling_wrappers = 1;
				genwrapper(t, f, a->tsym, 0);
				compiling_wrappers = 0;
			}
		}
	}

	return lsort(a, sigcmp);
}
