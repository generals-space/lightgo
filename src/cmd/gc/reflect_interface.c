#include "reflect.h"

/*
 * return methods of interface type t, sorted by name.
 */
Sig* imethods(Type *t)
{
	Sig *a, *all, *last;
	Type *f;
	Sym *method, *isym;

	all = nil;
	last = nil;
	for(f=t->type; f; f=f->down) {
		if(f->etype != TFIELD)
			fatal("imethods: not field");
		if(f->type->etype != TFUNC || f->sym == nil)
			continue;
		method = f->sym;
		a = mal(sizeof(*a));
		a->name = method->name;
		if(!exportname(method->name)) {
			if(method->pkg == nil)
				fatal("imethods: missing package");
			a->pkg = method->pkg;
		}
		a->mtype = f->type;
		a->offset = 0;
		a->type = methodfunc(f->type, nil);

		if(last && sigcmp(last, a) >= 0)
			fatal("sigcmp vs sortinter %s %s", last->name, a->name);
		if(last == nil)
			all = a;
		else
			last->link = a;
		last = a;

		// Compiler can only refer to wrappers for
		// named interface types and non-blank methods.
		if(t->sym == S || isblanksym(method))
			continue;

		// NOTE(rsc): Perhaps an oversight that
		// IfaceType.Method is not in the reflect data.
		// Generate the method body, so that compiled
		// code can refer to it.
		isym = methodsym(method, t, 0);
		if(!(isym->flags & SymSiggen)) {
			isym->flags |= SymSiggen;
			genwrapper(t, f, isym, 0);
		}
	}
	return all;
}
