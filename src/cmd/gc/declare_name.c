#include	<u.h>
#include	<libc.h>

#include	"go.h"

// 
// this generates a new name node, typically for labels or other one-off names.
// 
Node* newname(Sym *s)
{
	Node *n;

	if(s == S) {
		fatal("newname nil");
	}

	n = nod(ONAME, N, N);
	n->sym = s;
	n->type = T;
	n->addable = 1;
	n->ullman = 1;
	n->xoffset = 0;
	return n;
}

// caller:
// 	1. src/cmd/gc/go.y -> simple_stmt{}
// 	1. src/cmd/gc/go.y -> dcl_name{}
//
// this generates a new name node for a name being declared.
// 
Node* dclname(Sym *s)
{
	Node *n;

	n = newname(s);
	n->op = ONONAME;	// caller will correct it
	return n;
}

Node* typenod(Type *t)
{
	// if we copied another type with *t = *u
	// then t->nod might be out of date, so
	// check t->nod->type too
	if(t->nod == N || t->nod->type != t) {
		t->nod = nod(OTYPE, N, N);
		t->nod->type = t;
		t->nod->sym = t->sym;
	}
	return t->nod;
}

// 
// this will return an old name that has already been pushed on the
// declaration list.
// a diagnostic is generated if no name has been defined.
// 
Node* oldname(Sym *s)
{
	Node *n;
	Node *c;

	n = s->def;
	if(n == N) {
		// maybe a top-level name will come along
		// to give this a definition later.
		// walkdef will check s->def again once
		// all the input source has been processed.
		n = newname(s);
		n->op = ONONAME;
		n->iota = iota;	// save current iota value in const declarations
	}
	if(curfn != nil && n->funcdepth > 0 && n->funcdepth != funcdepth && n->op == ONAME) {
		// inner func is referring to var in outer func.
		//
		// TODO(rsc): If there is an outer variable x and we
		// are parsing x := 5 inside the closure, until we get to
		// the := it looks like a reference to the outer x so we'll
		// make x a closure variable unnecessarily.
		if(n->closure == N || n->closure->funcdepth != funcdepth) {
			// create new closure var.
			c = nod(ONAME, N, N);
			c->sym = s;
			c->class = PPARAMREF;
			c->isddd = n->isddd;
			c->defn = n;
			c->addable = 0;
			c->ullman = 2;
			c->funcdepth = funcdepth;
			c->outer = n->closure;
			n->closure = c;
			n->addrtaken = 1;
			c->closure = n;
			c->xoffset = 0;
			curfn->cvars = list(curfn->cvars, c);
		}
		// return ref to closure var, not original
		return n->closure;
	}
	return n;
}
