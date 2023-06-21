#include "typecheck.h"

static NodeList*	typecheckdefstack;

// caller:
//  1. typecheck1() 只有这一处
Node* typecheckdef(Node *n)
{
	int lno, nerrors0;
	Node *e;
	Type *t;
	NodeList *l;

	lno = lineno;
	setlineno(n);

	if(n->op == ONONAME) {
		if(!n->diag) {
			n->diag = 1;
			if(n->lineno != 0)
				lineno = n->lineno;
			yyerror("undefined: %S", n->sym);
		}
		return n;
	}

	if(n->walkdef == 1)
		return n;

	l = mal(sizeof *l);
	l->n = n;
	l->next = typecheckdefstack;
	typecheckdefstack = l;

	if(n->walkdef == 2) {
		flusherrors();
		print("typecheckdef loop:");
		for(l=typecheckdefstack; l; l=l->next)
			print(" %S", l->n->sym);
		print("\n");
		fatal("typecheckdef loop");
	}
	n->walkdef = 2;

	if(n->type != T || n->sym == S)	// builtin or no name
		goto ret;

	switch(n->op) {
	default:
		fatal("typecheckdef %O", n->op);

	case OGOTO:
	case OLABEL:
		// not really syms
		break;

	case OLITERAL:
		if(n->ntype != N) {
			typecheck(&n->ntype, Etype);
			n->type = n->ntype->type;
			n->ntype = N;
			if(n->type == T) {
				n->diag = 1;
				goto ret;
			}
		}
		e = n->defn;
		n->defn = N;
		if(e == N) {
			lineno = n->lineno;
			dump("typecheckdef nil defn", n);
			yyerror("xxx");
		}
		typecheck(&e, Erv | Eiota);
		if(isconst(e, CTNIL)) {
			yyerror("const initializer cannot be nil");
			goto ret;
		}
		if(e->type != T && e->op != OLITERAL || !isgoconst(e)) {
			yyerror("const initializer %N is not a constant", e);
			goto ret;
		}
		t = n->type;
		if(t != T) {
			if(!okforconst[t->etype]) {
				yyerror("invalid constant type %T", t);
				goto ret;
			}
			if(!isideal(e->type) && !eqtype(t, e->type)) {
				yyerror("cannot use %lN as type %T in const initializer", e, t);
				goto ret;
			}
			convlit(&e, t);
		}
		n->val = e->val;
		n->type = e->type;
		break;

	case ONAME:
		if(n->ntype != N) {
			typecheck(&n->ntype, Etype);
			n->type = n->ntype->type;

			if(n->type == T) {
				n->diag = 1;
				goto ret;
			}
		}
		if(n->type != T)
			break;
		if(n->defn == N) {
			if(n->etype != 0)	// like OPRINTN
				break;
			if(nsavederrors+nerrors > 0) {
				// Can have undefined variables in x := foo
				// that make x have an n->ndefn == nil.
				// If there are other errors anyway, don't
				// bother adding to the noise.
				break;
			}
			fatal("var without type, init: %S", n->sym);
		}
		if(n->defn->op == ONAME) {
			typecheck(&n->defn, Erv);
			n->type = n->defn->type;
			break;
		}
		typecheck(&n->defn, Etop);	// fills in n->type
		break;

	case OTYPE:
		if(curfn)
			defercheckwidth();
		n->walkdef = 1;
		n->type = typ(TFORW);
		n->type->sym = n->sym;
		nerrors0 = nerrors;
		typecheckdeftype(n);
		if(n->type->etype == TFORW && nerrors > nerrors0) {
			// Something went wrong during type-checking,
			// but it was reported. Silence future errors.
			n->type->broke = 1;
		}
		if(curfn)
			resumecheckwidth();
		break;

	case OPACK:
		// nothing to see here
		break;
	}

ret:
	if(n->op != OLITERAL && n->type != T && isideal(n->type))
		fatal("got %T for %N", n->type, n);
	if(typecheckdefstack->n != n)
		fatal("typecheckdefstack mismatch");
	l = typecheckdefstack;
	typecheckdefstack = l->next;

	lineno = lno;
	n->walkdef = 1;
	return n;
}
