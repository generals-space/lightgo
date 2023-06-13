#include "typecheck.h"

static int ntypecheckdeftype;
static NodeList *methodqueue;

// caller:
//  1. typecheckdeftype()
//  2. queuemethod()
static void domethod(Node *n)
{
	Node *nt;
	Type *t;

	nt = n->type->nname;
	typecheck(&nt, Etype);
	if(nt->type == T) {
		// type check failed; leave empty func
		n->type->etype = TFUNC;
		n->type->nod = N;
		return;
	}
	
	// If we have
	//	type I interface {
	//		M(_ int)
	//	}
	// then even though I.M looks like it doesn't care about the
	// value of its argument, a specific implementation of I may
	// care.  The _ would suppress the assignment to that argument
	// while generating a call, so remove it.
	for(t=getinargx(nt->type)->type; t; t=t->down) {
		if(t->sym != nil && strcmp(t->sym->name, "_") == 0)
			t->sym = nil;
	}

	*n->type = *nt->type;
	n->type->nod = N;
	checkwidth(n->type);
}

static NodeList *mapqueue;

// caller:
//  1. typecheckdeftype()
void copytype(Node *n, Type *t)
{
	int maplineno, embedlineno, lno;
	NodeList *l;

	if(t->etype == TFORW) {
		// This type isn't computed yet; when it is, update n.
		t->copyto = list(t->copyto, n);
		return;
	}

	maplineno = n->type->maplineno;
	embedlineno = n->type->embedlineno;

	l = n->type->copyto;
	*n->type = *t;

	t = n->type;
	t->sym = n->sym;
	t->local = n->local;
	t->vargen = n->vargen;
	t->siggen = 0;
	t->method = nil;
	t->xmethod = nil;
	t->nod = N;
	t->printed = 0;
	t->deferwidth = 0;
	t->copyto = nil;
	
	// Update nodes waiting on this type.
	for(; l; l=l->next)
		copytype(l->n, t);

	// Double-check use of type as embedded type.
	lno = lineno;
	if(embedlineno) {
		lineno = embedlineno;
		if(isptr[t->etype])
			yyerror("embedded type cannot be a pointer");
	}
	lineno = lno;
	
	// Queue check for map until all the types are done settling.
	if(maplineno) {
		t->maplineno = maplineno;
		mapqueue = list(mapqueue, n);
	}
}

// caller:
//  1. typecheck1()
//  1. typecheckdef()
void typecheckdeftype(Node *n)
{
	int lno;
	Type *t;
	NodeList *l;

	ntypecheckdeftype++;
	lno = lineno;
	setlineno(n);
	n->type->sym = n->sym;
	n->typecheck = 1;
	typecheck(&n->ntype, Etype);
	if((t = n->ntype->type) == T) {
		n->diag = 1;
		n->type = T;
		goto ret;
	}
	if(n->type == T) {
		n->diag = 1;
		goto ret;
	}

	// copy new type and clear fields
	// that don't come along.
	// anything zeroed here must be zeroed in
	// typedcl2 too.
	copytype(n, t);

ret:
	lineno = lno;

	// if there are no type definitions going on, it's safe to
	// try to resolve the method types for the interfaces
	// we just read.
	if(ntypecheckdeftype == 1) {
		while((l = methodqueue) != nil) {
			methodqueue = nil;
			for(; l; l=l->next)
				domethod(l->n);
		}
		for(l=mapqueue; l; l=l->next) {
			lineno = l->n->type->maplineno;
			maptype(l->n->type, types[TBOOL]);
		}
		lineno = lno;
	}
	ntypecheckdeftype--;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
//  1. src/cmd/gc/dcl.c -> interfacefield()
void queuemethod(Node *n)
{
	if(ntypecheckdeftype == 0) {
		domethod(n);
		return;
	}
	methodqueue = list(methodqueue, n);
}
