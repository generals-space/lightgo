#include	<u.h>

#include	<libc.h>

#include	"go.h"
/*
 * := declarations
 */

// caller: 
// 	1. colasdefn() 只有这一处
static int colasname(Node *n)
{
	switch(n->op) {
	case ONAME:
	case ONONAME:
	case OPACK:
	case OTYPE:
	case OLITERAL:
		return n->sym != S;
	}
	return 0;
}

// caller:
// 	1. src/cmd/gc/go.y -> range_stmt{} 
void colasdefn(NodeList *left, Node *defn)
{
	int nnew, nerr;
	NodeList *l;
	Node *n;

	nnew = 0;
	nerr = 0;
	for(l=left; l; l=l->next) {
		n = l->n;
		if(isblank(n))
			continue;
		if(!colasname(n)) {
			yyerrorl(defn->lineno, "non-name %N on left side of :=", n);
			nerr++;
			continue;
		}
		if(n->sym->block == block)
			continue;

		nnew++;
		n = newname(n->sym);
		declare(n, dclcontext);
		n->defn = defn;
		defn->ninit = list(defn->ninit, nod(ODCL, n, N));
		l->n = n;
	}
	if(nnew == 0 && nerr == 0)
		yyerrorl(defn->lineno, "no new variables on left side of :=");
}

//
// caller:
// 	1. src/cmd/gc/go.y -> simple_stmt{}
// 	处理 a, b, c := 1, 2, 3 这种同时赋多个值的场景(必须是 := 操作)
// 	2. src/cmd/gc/go.y -> case{}
Node* colas(NodeList *left, NodeList *right, int32 lno)
{
	Node *as;

	as = nod(OAS2, N, N);
	as->list = left;
	as->rlist = right;
	as->colas = 1;
	as->lineno = lno;
	colasdefn(left, as);

	// make the tree prettier; not necessary
	if(count(left) == 1 && count(right) == 1) {
		as->left = as->list->n;
		as->right = as->rlist->n;
		as->list = nil;
		as->rlist = nil;
		as->op = OAS;
	}

	return as;
}
