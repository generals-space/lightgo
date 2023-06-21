#include	"walk.h"

/*
 * from ascompat[te]
 * evaluating actual function arguments.
 *	f(a,b)
 * if there is exactly one function expr,
 * then it is done first. otherwise must
 * make temp variables
 */
NodeList* reorder1(NodeList *all)
{
	Node *f, *a, *n;
	NodeList *l, *r, *g;
	int c, d, t;

	c = 0;	// function calls
	t = 0;	// total parameters

	for(l=all; l; l=l->next) {
		n = l->n;
		t++;
		ullmancalc(n);
		if(n->ullman >= UINF)
			c++;
	}
	if(c == 0 || t == 1)
		return all;

	g = nil;	// fncalls assigned to tempnames
	f = N;	// last fncall assigned to stack
	r = nil;	// non fncalls and tempnames assigned to stack
	d = 0;
	for(l=all; l; l=l->next) {
		n = l->n;
		if(n->ullman < UINF) {
			r = list(r, n);
			continue;
		}
		d++;
		if(d == c) {
			f = n;
			continue;
		}

		// make assignment of fncall to tempname
		a = temp(n->right->type);
		a = nod(OAS, a, n->right);
		g = list(g, a);

		// put normal arg assignment on list
		// with fncall replaced by tempname
		n->right = a->left;
		r = list(r, n);
	}

	if(f != N)
		g = list(g, f);
	return concat(g, r);
}

static void reorder3save(Node**, NodeList*, NodeList*, NodeList**);
static int aliased(Node*, NodeList*, NodeList*);

/*
 * from ascompat[ee]
 *	a,b = c,d
 * simultaneous assignment. there cannot
 * be later use of an earlier lvalue.
 *
 * function calls have been removed.
 */
NodeList* reorder3(NodeList *all)
{
	NodeList *list, *early, *mapinit;
	Node *l;

	// If a needed expression may be affected by an
	// earlier assignment, make an early copy of that
	// expression and use the copy instead.
	early = nil;
	mapinit = nil;
	for(list=all; list; list=list->next) {
		l = list->n->left;

		// Save subexpressions needed on left side.
		// Drill through non-dereferences.
		for(;;) {
			if(l->op == ODOT || l->op == OPAREN) {
				l = l->left;
				continue;
			}
			if(l->op == OINDEX && isfixedarray(l->left->type)) {
				reorder3save(&l->right, all, list, &early);
				l = l->left;
				continue;
			}
			break;
		}
		switch(l->op) {
		default:
			fatal("reorder3 unexpected lvalue %#O", l->op);
		case ONAME:
			break;
		case OINDEX:
		case OINDEXMAP:
			reorder3save(&l->left, all, list, &early);
			reorder3save(&l->right, all, list, &early);
			if(l->op == OINDEXMAP)
				list->n = convas(list->n, &mapinit);
			break;
		case OIND:
		case ODOTPTR:
			reorder3save(&l->left, all, list, &early);
		}

		// Save expression on right side.
		reorder3save(&list->n->right, all, list, &early);
	}

	early = concat(mapinit, early);
	return concat(early, all);
}

/*
 * if the evaluation of *np would be affected by the 
 * assignments in all up to but not including stop,
 * copy into a temporary during *early and
 * replace *np with that temp.
 */
static void reorder3save(Node **np, NodeList *all, NodeList *stop, NodeList **early)
{
	Node *n, *q;

	n = *np;
	if(!aliased(n, all, stop))
		return;
	
	q = temp(n->type);
	q = nod(OAS, q, n);
	typecheck(&q, Etop);
	*early = list(*early, q);
	*np = q->left;
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. aliased() 只有这一处
// 
// what's the outer value that a write to n affects?
// outer value means containing struct or array.
// 
static Node* outervalue(Node *n)
{	
	for(;;) {
		if(n->op == ODOT || n->op == OPAREN) {
			n = n->left;
			continue;
		}
		if(n->op == OINDEX && isfixedarray(n->left->type)) {
			n = n->left;
			continue;
		}
		break;
	}
	return n;
}

// caller:
// 	1. aliased()
// 	2. varexpr()
//
// does the evaluation of n only refer to variables
// whose addresses have not been taken?
// (and no other memory)
// 
static int varexpr(Node *n)
{
	if(n == N)
		return 1;

	switch(n->op) {
	case OLITERAL:	
		return 1;
	case ONAME:
		switch(n->class) {
		case PAUTO:
		case PPARAM:
		case PPARAMOUT:
			if(!n->addrtaken)
				return 1;
		}
		return 0;

	case OADD:
	case OSUB:
	case OOR:
	case OXOR:
	case OMUL:
	case ODIV:
	case OMOD:
	case OLSH:
	case ORSH:
	case OAND:
	case OANDNOT:
	case OPLUS:
	case OMINUS:
	case OCOM:
	case OPAREN:
	case OANDAND:
	case OOROR:
	case ODOT:  // but not ODOTPTR
	case OCONV:
	case OCONVNOP:
	case OCONVIFACE:
	case ODOTTYPE:
		return varexpr(n->left) && varexpr(n->right);
	}

	// Be conservative.
	return 0;
}

// caller:
// 	1. reorder3save() 只有这一处
//
// Is it possible that the computation of n might be
// affected by writes in as up to but not including stop?
// 
static int aliased(Node *n, NodeList *all, NodeList *stop)
{
	int memwrite, varwrite;
	Node *a;
	NodeList *l;

	if(n == N)
		return 0;

	// Look for obvious aliasing: a variable being assigned
	// during the all list and appearing in n.
	// Also record whether there are any writes to main memory.
	// Also record whether there are any writes to variables
	// whose addresses have been taken.
	memwrite = 0;
	varwrite = 0;
	for(l=all; l!=stop; l=l->next) {
		a = outervalue(l->n->left);
		if(a->op != ONAME) {
			memwrite = 1;
			continue;
		}
		switch(n->class) {
		default:
			varwrite = 1;
			continue;
		case PAUTO:
		case PPARAM:
		case PPARAMOUT:
			if(n->addrtaken) {
				varwrite = 1;
				continue;
			}
			if(vmatch2(a, n)) {
				// Direct hit.
				return 1;
			}
		}
	}

	// The variables being written do not appear in n.
	// However, n might refer to computed addresses
	// that are being written.
	
	// If no computed addresses are affected by the writes, no aliasing.
	if(!memwrite && !varwrite)
		return 0;

	// If n does not refer to computed addresses
	// (that is, if n only refers to variables whose addresses
	// have not been taken), no aliasing.
	if(varexpr(n))
		return 0;

	// Otherwise, both the writes and n refer to computed memory addresses.
	// Assume that they might conflict.
	return 1;
}
