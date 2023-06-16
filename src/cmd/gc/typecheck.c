// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// 主流程其实在 typecheck1.c -> typecheck1()

/*
 * type check the whole tree of an expression.
 * calculates expression types.
 * evaluates compile time constants.
 * marks variables that escape the local frame.
 * rewrites n->op to be more specific in some cases.
 */

#include "typecheck.h"

// caller:
// 	1. typecheck()
// 	2. typecheckas()
//
// resolve ONONAME to definition, if any.
Node* resolve(Node *n)
{
	Node *r;

	if(n != N && n->op == ONONAME && n->sym != S && (r = n->sym->def) != N) {
		if(r->op != OIOTA) {
			n = r;
		}
		else if(n->iota >= 0) {
			n = nodintconst(n->iota);
		}
	}
	return n;
}

/*
 * sprint_depchain prints a dependency chain
 * of nodes into fmt.
 * It is used by typecheck in the case of OLITERAL nodes
 * to print constant definition loops.
 */
// caller:
// 	1. typecheck()
// 	1. sprint_depchain()
static void sprint_depchain(Fmt *fmt, NodeList *stack, Node *cur, Node *first)
{
	NodeList *l;

	for(l = stack; l; l=l->next) {
		if(l->n->op == cur->op) {
			if(l->n != first)
				sprint_depchain(fmt, l->next, l->n, first);
			fmtprint(fmt, "\n\t%L: %N uses %N", l->n->lineno, l->n, cur);
			return;
		}
	}
}

/*
 * type check node *np.
 * replaces *np with a new pointer in some cases.
 * returns the final value of *np as a convenience(为了方便起见).
 */
// caller:
// 	1. typecheck1()
// 	2. typechecklist()
// 	3. src/cmd/gc/walk.c -> walk()
Node* typecheck(Node **np, int top)
{
	Node *n;
	int lno;
	Fmt fmt;
	NodeList *l;
	static NodeList *tcstack, *tcfree;

	// cannot type check until all the source has been parsed
	if(!typecheckok) {
		fatal("early typecheck");
	}

	n = *np;
	if(n == N) {
		return N;
	}

	lno = setlineno(n);

	// Skip over parens.
	while(n->op == OPAREN) {
		n = n->left;
	}

	// Resolve definition of name and value of iota lazily.
	n = resolve(n);

	*np = n;

	// Skip typecheck if already done.
	// But re-typecheck ONAME/OTYPE/OLITERAL/OPACK node in case context has changed.
	if(n->typecheck == 1) {
		switch(n->op) {
		case ONAME:
		case OTYPE:
		case OLITERAL:
		case OPACK:
			break;
		default:
			lineno = lno;
			return n;
		}
	}

	if(n->typecheck == 2) {
		// Typechecking loop. Trying printing a meaningful message,
		// otherwise a stack trace of typechecking.
		switch(n->op) {
		case ONAME:
			// We can already diagnose variables used as types.
			if((top & (Erv|Etype)) == Etype)
				yyerror("%N is not a type", n);
			break;
		case OLITERAL:
			if((top & (Erv|Etype)) == Etype) {
				yyerror("%N is not a type", n);
				break;
			}
			fmtstrinit(&fmt);
			sprint_depchain(&fmt, tcstack, n, n);
			yyerrorl(n->lineno, "constant definition loop%s", fmtstrflush(&fmt));
			break;
		}
		if(nsavederrors+nerrors == 0) {
			fmtstrinit(&fmt);
			for(l=tcstack; l; l=l->next)
				fmtprint(&fmt, "\n\t%L %N", l->n->lineno, l->n);
			yyerror("typechecking loop involving %N%s", n, fmtstrflush(&fmt));
		}
		lineno = lno;
		return n;
	}
	n->typecheck = 2;

	if(tcfree != nil) {
		l = tcfree;
		tcfree = l->next;
	} else {
		l = mal(sizeof *l);
	}
	l->next = tcstack;
	l->n = n;
	tcstack = l;

	typecheck1(&n, top);
	*np = n;
	n->typecheck = 1;

	if(tcstack != l) {
		fatal("typecheck stack out of sync");
	}
	tcstack = l->next;
	l->next = tcfree;
	tcfree = l;

	lineno = lno;
	return n;
}

/*
 * does n contain a call or receive operation?
 */
static int callrecvlist(NodeList*);

// caller:
// 	1. typecheck1()
// 	1. callrecvlist()
// 	3. callrecv()
int callrecv(Node *n)
{
	if(n == nil)
		return 0;
	
	switch(n->op) {
	case OCALL:
	case OCALLMETH:
	case OCALLINTER:
	case OCALLFUNC:
	case ORECV:
	case OCAP:
	case OLEN:
	case OCOPY:
	case ONEW:
	case OAPPEND:
	case ODELETE:
		return 1;
	}

	return callrecv(n->left) ||
		callrecv(n->right) ||
		callrecv(n->ntest) ||
		callrecv(n->nincr) ||
		callrecvlist(n->ninit) ||
		callrecvlist(n->nbody) ||
		callrecvlist(n->nelse) ||
		callrecvlist(n->list) ||
		callrecvlist(n->rlist);
}

// caller:
// 	1. callrecv() 只有这一处
static int callrecvlist(NodeList *l)
{
	for(; l; l=l->next)
		if(callrecv(l->n))
			return 1;
	return 0;
}
