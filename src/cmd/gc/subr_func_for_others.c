#include	<u.h>
#include	<libc.h>
#include	"go.h"
#include	"md5.h"

/*
 * return !(op)
 * eg == <=> !=
 */
int brcom(int a)
{
	switch(a) {
	case OEQ:	return ONE;
	case ONE:	return OEQ;
	case OLT:	return OGE;
	case OGT:	return OLE;
	case OLE:	return OGT;
	case OGE:	return OLT;
	}
	fatal("brcom: no com for %A\n", a);
	return a;
}

/*
 * return reverse(op)
 * eg a op b <=> b r(op) a
 */
int brrev(int a)
{
	switch(a) {
	case OEQ:	return OEQ;
	case ONE:	return ONE;
	case OLT:	return OGT;
	case OGT:	return OLT;
	case OLE:	return OGE;
	case OGE:	return OLE;
	}
	fatal("brcom: no rev for %A\n", a);
	return a;
}

////////////////////////////////////////////////////////////////////////////////

void frame(int context)
{
	char *p;
	NodeList *l;
	Node *n;
	int flag;

	p = "stack";
	l = nil;
	if(curfn)
		l = curfn->dcl;
	if(context) {
		p = "external";
		l = externdcl;
	}

	flag = 1;
	for(; l; l=l->next) {
		n = l->n;
		switch(n->op) {
		case ONAME:
			if(flag)
				print("--- %s frame ---\n", p);
			print("%O %S G%d %T\n", n->op, n->sym, n->vargen, n->type);
			flag = 0;
			break;

		case OTYPE:
			if(flag)
				print("--- %s frame ---\n", p);
			print("%O %T\n", n->op, n->type);
			flag = 0;
			break;
		}
	}
}

Type* aindex(Node *b, Type *t)
{
	Type *r;
	int64 bound;

	bound = -1;	// open bound
	typecheck(&b, Erv);
	if(b != nil) {
		switch(consttype(b)) {
		default:
			yyerror("array bound must be an integer expression");
			break;
		case CTINT:
		case CTRUNE:
			bound = mpgetfix(b->val.u.xval);
			if(bound < 0)
				yyerror("array bound must be non negative");
			break;
		}
	}

	// fixed array
	r = typ(TARRAY);
	r->type = t;
	r->bound = bound;
	return r;
}

void setmaxarg(Type *t)
{
	int64 w;

	dowidth(t);
	w = t->argwid;
	if(t->argwid >= MAXWIDTH)
		fatal("bad argwid %T", t);
	if(w > maxarg)
		maxarg = w;
}

/*
 * compute a hash value for type t.
 * if t is a method type, ignore the receiver
 * so that the hash can be used in interface checks.
 * %T already contains
 * all the necessary logic to generate a representation
 * of the type that completely describes it.
 * using smprint here avoids duplicating that code.
 * using md5 here is overkill, but i got tired of
 * accidental collisions making the runtime think
 * two types are equal when they really aren't.
 */
uint32 typehash(Type *t)
{
	char *p;
	MD5 d;

	if(t->thistuple) {
		// hide method receiver from Tpretty
		t->thistuple = 0;
		p = smprint("%-uT", t);
		t->thistuple = 1;
	} else {
		p = smprint("%-uT", t);
	}
	//print("typehash: %s\n", p);
	md5reset(&d);
	md5write(&d, (uchar*)p, strlen(p));
	free(p);
	return md5sum(&d);
}
