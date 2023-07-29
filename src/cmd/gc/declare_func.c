#include	<u.h>

#include	<libc.h>

#include	"go.h"

int dflag(void)
{
	if(!debug['d'])
		return 0;
	if(debug['y'])
		return 1;
	if(incannedimport)
		return 0;
	return 1;
}

/*
 * declaration stack & operations
 */

static void dcopy(Sym *a, Sym *b)
{
	a->pkg = b->pkg;
	a->name = b->name;
	a->def = b->def;
	a->block = b->block;
	a->lastlineno = b->lastlineno;
}

// caller:
// 	1. pushdcl()
// 	2. markdcl()
static Sym* push(void)
{
	Sym *d;

	d = mal(sizeof(*d));
	d->lastlineno = lineno;
	d->link = dclstack;
	dclstack = d;
	return d;
}

Sym* pushdcl(Sym *s)
{
	Sym *d;

	d = push();
	dcopy(d, s);
	if(dflag())
		print("\t%L push %S %p\n", lineno, s, s->def);
	return d;
}

void popdcl(void)
{
	Sym *d, *s;
	int lno;

	// if(dflag())
	// 	print("revert\n");

	for(d=dclstack; d!=S; d=d->link) {
		if(d->name == nil)
			break;
		s = pkglookup(d->name, d->pkg);
		lno = s->lastlineno;
		dcopy(s, d);
		d->lastlineno = lno;
		if(dflag())
			print("\t%L pop %S %p\n", lineno, s, s->def);
	}
	if(d == S)
		fatal("popdcl: no mark");
	dclstack = d->link;
	block = d->block;
}

void poptodcl(void)
{
	// pop the old marker and push a new one
	// (cannot reuse the existing one)
	// because we use the markers to identify blocks
	// for the goto restriction checks.
	popdcl();
	markdcl();
}

void markdcl(void)
{
	Sym *d;

	d = push();
	d->name = nil;		// used as a mark in fifo
	d->block = block;

	blockgen++;
	block = blockgen;

//	if(dflag())
//		print("markdcl\n");
}

void dumpdcl(char *st)
{
	Sym *s, *d;
	int i;

	USED(st);

	i = 0;
	for(d=dclstack; d!=S; d=d->link) {
		i++;
		print("    %.2d %p", i, d);
		if(d->name == nil) {
			print("\n");
			continue;
		}
		print(" '%s'", d->name);
		s = pkglookup(d->name, d->pkg);
		print(" %lS\n", s);
	}
}

// caller:
//  1. src/cmd/gc/go.y -> xdcl_list{}
//  2. src/cmd/gc/lex.c -> main()
void testdclstack(void)
{
	Sym *d;

	for(d=dclstack; d!=S; d=d->link) {
		if(d->name == nil) {
			if(nerrors != 0)
				errorexit();
			yyerror("mark left on the stack");
			continue;
		}
	}
}

// caller:
// 	1. src/cmd/gc/go.y -> indcl{}
// 	1. src/cmd/gc/go.y -> hidden_interfacedcl{}
Node* fakethis(void)
{
	Node *n;

	n = nod(ODCLFIELD, N, typenod(ptrto(typ(TSTRUCT))));
	return n;
}
