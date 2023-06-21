#include "typecheck.h"

/*
 * lvalue etc
 */
// caller:
//  1. typecheck1()
//  1. checklvalue()
//  1. checkassign()
int islvalue(Node *n)
{
	switch(n->op) {
	case OINDEX:
		if(isfixedarray(n->left->type))
			return islvalue(n->left);
		if(n->left->type != T && n->left->type->etype == TSTRING)
			return 0;
		// fall through
	case OIND:
	case ODOTPTR:
	case OCLOSUREVAR:
		return 1;
	case ODOT:
		return islvalue(n->left);
	case ONAME:
		if(n->class == PFUNC)
			return 0;
		return 1;
	}
	return 0;
}

// caller:
//  1. typecheck1()
//  1. lookdot()
void checklvalue(Node *n, char *verb)
{
	if(!islvalue(n)) {
		yyerror("cannot %s %N", verb, n);
	}
}

// 对 make() 函数创建的 slice, map 或 channel 进行参数检查.
// 对于 slice 是 len 和 cap, 对于 map 是 size, 对于 channel 则是 buffer.
//
// 	@param arg: len, cap, size, buffer 等参数的字符串描述信息.
//
// caller:
// 	1. typecheck1() 只有这一处, 
//  只在 make() 函数的类型检查时调用, 分别对 slice, map, channel 进行参数检查.
int checkmake(Type *t, char *arg, Node *n)
{
	if(n->op == OLITERAL) {
		n->val = toint(n->val);
		if(mpcmpfixc(n->val.u.xval, 0) < 0) {
			yyerror("negative %s argument in make(%T)", arg, t);
			return -1;
		}
		if(mpcmpfixfix(n->val.u.xval, maxintval[TINT]) > 0) {
			yyerror("%s argument too large in make(%T)", arg, t);
			return -1;
		}
		
		// Delay defaultlit until after we've checked range, to avoid
		// a redundant "constant NNN overflows int" error.
		defaultlit(&n, types[TINT]);
		return 0;
	}
	
	// Defaultlit still necessary for non-constant: n might be 1<<k.
	defaultlit(&n, types[TINT]);

	if(!isint[n->type->etype]) {
		yyerror("non-integer %s argument in make(%T) - %T", arg, t, n->type);
		return -1;
	}
	return 0;
}

static void	markbreaklist(NodeList*, Node*);

// caller:
//  1. markbreak() 
//  2. markbreaklist()
static void markbreak(Node *n, Node *implicit)
{
	Label *lab;

	if(n == N)
		return;

	switch(n->op) {
	case OBREAK:
		if(n->left == N) {
			if(implicit)
				implicit->hasbreak = 1;
		} else {
			lab = n->left->sym->label;
			if(lab != L)
				lab->def->hasbreak = 1;
		}
		break;
	
	case OFOR:
	case OSWITCH:
	case OTYPESW:
	case OSELECT:
	case ORANGE:
		implicit = n;
		// fall through
	
	default:
		markbreak(n->left, implicit);
		markbreak(n->right, implicit);
		markbreak(n->ntest, implicit);
		markbreak(n->nincr, implicit);
		markbreaklist(n->ninit, implicit);
		markbreaklist(n->nbody, implicit);
		markbreaklist(n->nelse, implicit);
		markbreaklist(n->list, implicit);
		markbreaklist(n->rlist, implicit);
		break;
	}
}

// caller:
//  1. markbreak()
//  2. isterminating()
static void markbreaklist(NodeList *l, Node *implicit)
{
	Node *n;
	Label *lab;

	for(; l; l=l->next) {
		n = l->n;
		if(n->op == OLABEL && l->next && n->defn == l->next->n) {
			switch(n->defn->op) {
			case OFOR:
			case OSWITCH:
			case OTYPESW:
			case OSELECT:
			case ORANGE:
				lab = mal(sizeof *lab);
				lab->def = n->defn;
				n->left->sym->label = lab;
				markbreak(n->defn, n->defn);
				n->left->sym->label = L;
				l = l->next;
				continue;
			}
		}
		markbreak(n, implicit);
	}
}

// caller:
//  1. isterminating()
//  2. checkreturn()
static int isterminating(NodeList *l, int top)
{
	int def;
	Node *n;

	if(l == nil)
		return 0;
	if(top) {
		while(l->next && l->n->op != OLABEL)
			l = l->next;
		markbreaklist(l, nil);
	}
	while(l->next)
		l = l->next;
	n = l->n;

	if(n == N)
		return 0;

	switch(n->op) {
	// NOTE: OLABEL is treated as a separate statement,
	// not a separate prefix, so skipping to the last statement
	// in the block handles the labeled statement case by
	// skipping over the label. No case OLABEL here.

	case OBLOCK:
		return isterminating(n->list, 0);

	case OGOTO:
	case ORETURN:
	case ORETJMP:
	case OPANIC:
	case OXFALL:
		return 1;

	case OFOR:
		if(n->ntest != N)
			return 0;
		if(n->hasbreak)
			return 0;
		return 1;

	case OIF:
		return isterminating(n->nbody, 0) && isterminating(n->nelse, 0);

	case OSWITCH:
	case OTYPESW:
	case OSELECT:
		if(n->hasbreak)
			return 0;
		def = 0;
		for(l=n->list; l; l=l->next) {
			if(!isterminating(l->n->nbody, 0))
				return 0;
			if(l->n->list == nil) // default
				def = 1;
		}
		if(n->op != OSELECT && !def)
			return 0;
		return 1;
	}
	
	return 0;
}

// caller:
//  1. src/cmd/gc/lex.c -> main() 只有这一处.
void checkreturn(Node *fn)
{
	if(fn->type->outtuple && fn->nbody != nil) {
		if(!isterminating(fn->nbody, 1)) {
			yyerrorl(fn->endlineno, "missing return at end of function");
		}
	}
}
