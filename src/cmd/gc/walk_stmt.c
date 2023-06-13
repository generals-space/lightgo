#include	"walk.h"

static int samelist(NodeList *a, NodeList *b);
static int paramoutheap(Node *fn);

void walkstmt(Node **np)
{
	NodeList *init;
	NodeList *ll, *rl;
	int cl;
	Node *n, *f;

	n = *np;
	if(n == N)
		return;

	setlineno(n);

	walkstmtlist(n->ninit);

	switch(n->op) {
	default:
		if(n->op == ONAME)
			yyerror("%S is not a top level statement", n->sym);
		else
			yyerror("%O is not a top level statement", n->op);
		dump("nottop", n);
		break;

	case OAS:
	case OASOP:
	case OAS2:
	case OAS2DOTTYPE:
	case OAS2RECV:
	case OAS2FUNC:
	case OAS2MAPR:
	case OCLOSE:
	case OCOPY:
	case OCALLMETH:
	case OCALLINTER:
	case OCALL:
	case OCALLFUNC:
	case ODELETE:
	case OSEND:
	case ORECV:
	case OPRINT:
	case OPRINTN:
	case OPANIC:
	case OEMPTY:
	case ORECOVER:
		if(n->typecheck == 0)
			fatal("missing typecheck: %+N", n);
		init = n->ninit;
		n->ninit = nil;
		walkexpr(&n, &init);
		addinit(&n, init);
		if((*np)->op == OCOPY && n->op == OCONVNOP)
			n->op = OEMPTY; // don't leave plain values as statements.
		break;

	case OBREAK:
	case ODCL:
	case OCONTINUE:
	case OFALL:
	case OGOTO:
	case OLABEL:
	case ODCLCONST:
	case ODCLTYPE:
	case OCHECKNIL:
		break;

	case OBLOCK:
		walkstmtlist(n->list);
		break;

	case OXCASE:
		yyerror("case statement out of place");
		n->op = OCASE;
	case OCASE:
		walkstmt(&n->right);
		break;

	case ODEFER:
		hasdefer = 1;
		switch(n->left->op) {
		case OPRINT:
		case OPRINTN:
			walkexprlist(n->left->list, &n->ninit);
			n->left = walkprint(n->left, &n->ninit, 1);
			break;
		default:
			walkexpr(&n->left, &n->ninit);
			break;
		}
		break;

	case OFOR:
		if(n->ntest != N) {
			walkstmtlist(n->ntest->ninit);
			init = n->ntest->ninit;
			n->ntest->ninit = nil;
			walkexpr(&n->ntest, &init);
			addinit(&n->ntest, init);
		}
		walkstmt(&n->nincr);
		walkstmtlist(n->nbody);
		break;

	case OIF:
		walkexpr(&n->ntest, &n->ninit);
		walkstmtlist(n->nbody);
		walkstmtlist(n->nelse);
		break;

	case OPROC:
		switch(n->left->op) {
		case OPRINT:
		case OPRINTN:
			walkexprlist(n->left->list, &n->ninit);
			n->left = walkprint(n->left, &n->ninit, 1);
			break;
		default:
			walkexpr(&n->left, &n->ninit);
			break;
		}
		break;

	case ORETURN:
		walkexprlist(n->list, &n->ninit);
		if(n->list == nil)
			break;
		if((curfn->type->outnamed && count(n->list) > 1) || paramoutheap(curfn)) {
			// assign to the function out parameters,
			// so that reorder3 can fix up conflicts
			rl = nil;
			for(ll=curfn->dcl; ll != nil; ll=ll->next) {
				cl = ll->n->class & ~PHEAP;
				if(cl == PAUTO)
					break;
				if(cl == PPARAMOUT)
					rl = list(rl, ll->n);
			}
			if(samelist(rl, n->list)) {
				// special return in disguise
				n->list = nil;
				break;
			}
			if(count(n->list) == 1 && count(rl) > 1) {
				// OAS2FUNC in disguise
				f = n->list->n;
				if(f->op != OCALLFUNC && f->op != OCALLMETH && f->op != OCALLINTER)
					fatal("expected return of call, have %N", f);
				n->list = concat(list1(f), ascompatet(n->op, rl, &f->type, 0, &n->ninit));
				break;
			}

			// move function calls out, to make reorder3's job easier.
			walkexprlistsafe(n->list, &n->ninit);
			ll = ascompatee(n->op, rl, n->list, &n->ninit);
			n->list = reorder3(ll);
			break;
		}
		ll = ascompatte(n->op, nil, 0, getoutarg(curfn->type), n->list, 1, &n->ninit);
		n->list = ll;
		break;

	case ORETJMP:
		break;

	case OSELECT:
		walkselect(n);
		break;

	case OSWITCH:
		walkswitch(n);
		break;

	case ORANGE:
		walkrange(n);
		break;

	case OXFALL:
		yyerror("fallthrough statement out of place");
		n->op = OFALL;
		break;
	}

	if(n->op == ONAME)
		fatal("walkstmt ended up with name: %+N", n);
	
	*np = n;
}

void walkstmtlist(NodeList *l)
{
	for(; l; l=l->next) {
		walkstmt(&l->n);
	}
}

////////////////////////////////////////////////////////////////////////////////

// caller:
// 	1. walkstmt() 只有这一处
static int samelist(NodeList *a, NodeList *b)
{
	for(; a && b; a=a->next, b=b->next) {
		if(a->n != b->n) {
			return 0;
		}
	}
	return a == b;
}

// caller:
// 	1. walkstmt() 只有这一处
static int paramoutheap(Node *fn)
{
	NodeList *l;

	for(l=fn->dcl; l; l=l->next) {
		switch(l->n->class) {
		case PPARAMOUT:
		case PPARAMOUT|PHEAP:
			return l->n->addrtaken;
		case PAUTO:
		case PAUTO|PHEAP:
			// stop early - parameters are over
			return 0;
		}
	}
	return 0;
}
