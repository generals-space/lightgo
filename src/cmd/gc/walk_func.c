#include	"walk.h"

static NodeList* paramstoheap(Type **argin, int out);
static NodeList* returnsfromheap(Type **argin);

// caller:
// 	1. walk() 只有这一处
//
// take care of migrating any function in/out args
// between the stack and the heap.  adds code to
// curfn's before and after lists.
// 
void heapmoves(void)
{
	NodeList *nn;
	int32 lno;

	lno = lineno;
	lineno = curfn->lineno;
	nn = paramstoheap(getthis(curfn->type), 0);
	nn = concat(nn, paramstoheap(getinarg(curfn->type), 0));
	nn = concat(nn, paramstoheap(getoutarg(curfn->type), 1));
	curfn->enter = concat(curfn->enter, nn);
	lineno = curfn->endlineno;
	curfn->exit = returnsfromheap(getoutarg(curfn->type));
	lineno = lno;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. heapmoves() 只有这一处
//
// walk through argin parameters.
// generate and return code to allocate
// copies of escaped parameters to the heap.
// 
static NodeList* paramstoheap(Type **argin, int out)
{
	Type *t;
	Iter savet;
	Node *v;
	NodeList *nn;

	nn = nil;
	for(t = structfirst(&savet, argin); t != T; t = structnext(&savet)) {
		v = t->nname;
		if(v && v->sym && v->sym->name[0] == '~')
			v = N;
		// In precisestack mode, the garbage collector assumes results
		// are always live, so zero them always.
		if(out && (precisestack_enabled || (v == N && hasdefer))) {
			// Defer might stop a panic and show the
			// return values as they exist at the time of panic.
			// Make sure to zero them on entry to the function.
			nn = list(nn, nod(OAS, nodarg(t, 1), N));
		}
		if(v == N || !(v->class & PHEAP))
			continue;

		// generate allocation & copying code
		if(v->alloc == nil)
			v->alloc = callnew(v->type);
		nn = list(nn, nod(OAS, v->heapaddr, v->alloc));
		if((v->class & ~PHEAP) != PPARAMOUT)
			nn = list(nn, nod(OAS, v, v->stackparam));
	}
	return nn;
}

//
// caller:
// 	1. heapmoves() 只有这一处
//
// walk through argout parameters copying back to stack
//
static NodeList* returnsfromheap(Type **argin)
{
	Type *t;
	Iter savet;
	Node *v;
	NodeList *nn;

	nn = nil;
	for(t = structfirst(&savet, argin); t != T; t = structnext(&savet)) {
		v = t->nname;
		if(v == N || v->class != (PHEAP|PPARAMOUT))
			continue;
		nn = list(nn, nod(OAS, v->stackparam, v));
	}
	return nn;
}
