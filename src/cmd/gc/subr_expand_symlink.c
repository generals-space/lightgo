#include	<u.h>
#include	<libc.h>
#include	"go.h"

/*
 * code to help generate trampoline
 * functions for methods on embedded
 * subtypes.
 * these are approx the same as
 * the corresponding adddot routines
 * except that they expect to be called
 * with unique tasks and they return
 * the actual methods.
 */

typedef	struct	Symlink	Symlink;
struct	Symlink
{
	Type*		field;
	uchar		good;
	uchar		followptr;
	Symlink*	link;
};
static	Symlink*	slist;

static void expand0(Type *t, int followptr)
{
	Type *f, *u;
	Symlink *sl;

	u = t;
	if(isptr[u->etype]) {
		followptr = 1;
		u = u->type;
	}

	if(u->etype == TINTER) {
		for(f=u->type; f!=T; f=f->down) {
			if(f->sym->flags & SymUniq)
				continue;
			f->sym->flags |= SymUniq;
			sl = mal(sizeof(*sl));
			sl->field = f;
			sl->link = slist;
			sl->followptr = followptr;
			slist = sl;
		}
		return;
	}

	u = methtype(t, 0);
	if(u != T) {
		for(f=u->method; f!=T; f=f->down) {
			if(f->sym->flags & SymUniq)
				continue;
			f->sym->flags |= SymUniq;
			sl = mal(sizeof(*sl));
			sl->field = f;
			sl->link = slist;
			sl->followptr = followptr;
			slist = sl;
		}
	}
}

static void expand1(Type *t, int d, int followptr)
{
	Type *f, *u;

	if(t->trecur)
		return;
	if(d == 0)
		return;
	t->trecur = 1;

	if(d != nelem(dotlist)-1)
		expand0(t, followptr);

	u = t;
	if(isptr[u->etype]) {
		followptr = 1;
		u = u->type;
	}
	if(u->etype != TSTRUCT && u->etype != TINTER)
		goto out;

	for(f=u->type; f!=T; f=f->down) {
		if(!f->embedded)
			continue;
		if(f->sym == S)
			continue;
		expand1(f->type, d-1, followptr);
	}

out:
	t->trecur = 0;
}

// caller:
// 	1. implements()
void expandmeth(Type *t)
{
	Symlink *sl;
	Type *f;
	int c, d;

	if(t == T || t->xmethod != nil)
		return;

	// mark top-level method symbols
	// so that expand1 doesn't consider them.
	for(f=t->method; f != nil; f=f->down)
		f->sym->flags |= SymUniq;

	// generate all reachable methods
	slist = nil;
	expand1(t, nelem(dotlist)-1, 0);

	// check each method to be uniquely reachable
	for(sl=slist; sl!=nil; sl=sl->link) {
		sl->field->sym->flags &= ~SymUniq;
		for(d=0; d<nelem(dotlist); d++) {
			c = adddot1(sl->field->sym, t, d, &f, 0);
			if(c == 0)
				continue;
			if(c == 1) {
				// addot1 may have dug out arbitrary fields, we only want methods.
				if(f->type->etype == TFUNC && f->type->thistuple > 0) {
					sl->good = 1;
					sl->field = f;
				}
			}
			break;
		}
	}

	for(f=t->method; f != nil; f=f->down)
		f->sym->flags &= ~SymUniq;

	t->xmethod = t->method;
	for(sl=slist; sl!=nil; sl=sl->link) {
		if(sl->good) {
			// add it to the base type method list
			f = typ(TFIELD);
			*f = *sl->field;
			f->embedded = 1;	// needs a trampoline
			if(sl->followptr)
				f->embedded = 2;
			f->down = t->xmethod;
			t->xmethod = f;
		}
	}
}
