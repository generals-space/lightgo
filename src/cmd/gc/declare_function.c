#include	<u.h>
#include	<libc.h>

#include	"declare.h"

static	void	funcargs2(Type*);

// caller:
// 	1. src/cmd/gc/go.y -> interfacedcl{} 只有这一处
// 
// declare the arguments in an interface field declaration.
// 
void ifacedcl(Node *n)
{
	if(n->op != ODCLFIELD || n->right == N)
		fatal("ifacedcl");

	dclcontext = PPARAM;
	markdcl();
	funcdepth++;
	n->outer = curfn;
	curfn = n;
	funcargs(n->right);

	// funcbody is normally called after the parser has
	// seen the body of a function but since an interface
	// field declaration does not have a body, we must
	// call it now to pop the current declaration context.
	dclcontext = PAUTO;
	funcbody(n);
}

// 	@param n: 这是一个函数类型(ODCLFUNC)的 Node
//
// caller:
// 	1. src/cmd/gc/go.y -> fndcl{}
//
// declare the function proper and declare the arguments.
// called in extern-declaration context
// returns in auto-declaration context.
// 
void funchdr(Node *n)
{
	// change the declaration context from extern to auto
	if(funcdepth == 0 && dclcontext != PEXTERN) {
		fatal("funchdr: dclcontext");
	}

	dclcontext = PAUTO;
	markdcl();
	funcdepth++;

	n->outer = curfn;
	curfn = n;

	if(n->nname) {
		funcargs(n->nname->ntype);
	}
	else if (n->ntype) {
		funcargs(n->ntype);
	}
	else {
		funcargs2(n->type);
	}
}

// caller:
// 	1. funchdr() 只有这一处
//
// Same as funcargs, except run over an already constructed TFUNC.
// This happens during import, where the hidden_fndcl rule has
// used functype directly to parse the function's type.
// 
static void funcargs2(Type *t)
{
	Type *ft;
	Node *n;

	if(t->etype != TFUNC)
		fatal("funcargs2 %T", t);
	
	if(t->thistuple)
		for(ft=getthisx(t)->type; ft; ft=ft->down) {
			if(!ft->nname || !ft->nname->sym)
				continue;
			n = ft->nname;  // no need for newname(ft->nname->sym)
			n->type = ft->type;
			declare(n, PPARAM);
		}

	if(t->intuple)
		for(ft=getinargx(t)->type; ft; ft=ft->down) {
			if(!ft->nname || !ft->nname->sym)
				continue;
			n = ft->nname;
			n->type = ft->type;
			declare(n, PPARAM);
		}

	if(t->outtuple)
		for(ft=getoutargx(t)->type; ft; ft=ft->down) {
			if(!ft->nname || !ft->nname->sym)
				continue;
			n = ft->nname;
			n->type = ft->type;
			declare(n, PPARAMOUT);
		}
}

// caller:
// 	1. src/cmd/gc/go.y -> xfndcl()
// 	2. src/cmd/gc/go.y -> hidden_import()
//
// finish the body.
// called in auto-declaration context.
// returns in extern-declaration context.
// 
void funcbody(Node *n)
{
	// change the declaration context from auto to extern
	if(dclcontext != PAUTO)
		fatal("funcbody: dclcontext");
	popdcl();
	funcdepth--;
	curfn = n->outer;
	n->outer = N;
	if(funcdepth == 0)
		dclcontext = PEXTERN;
}

/*
 * check that the list of declarations is either all anonymous or all named
 */

static Node* findtype(NodeList *l)
{
	for(; l; l=l->next)
		if(l->n->op == OKEY)
			return l->n->right;
	return N;
}

// checkarglist 
//
// 	@param all: 函数的参数列表, 即 func xxx(a string, b int) 中括号的部分
// 	@param input: 1表示为传入的参数列表, 0则表示为返回值列表.
//
// caller:
// 	1. src/cmd/gc/go.y -> fndcl{}
// 	2. src/cmd/gc/go.y -> fntype{}
// 	3. src/cmd/gc/go.y -> fnres{}
// 	4. src/cmd/gc/go.y -> indcl{}
NodeList* checkarglist(NodeList *all, int input)
{
	// named 表示当前的参数列表是匿名参数还是命名参数.
	int named;
	// 参数名称
	Node *n;
	// 参数类型
	Node *t;
	// 参数列表
	NodeList *l;
	Node *nextt;

	// named 表示当前的参数列表是匿名参数还是命名参数.
	// 一般匿名参数列表只在函数声明(无参数体)中出现, 如 func xxx(string, int)(error)
	//
	// 关于作为判断的 OKEY, 就是"参数名 参数类型"的组合节点, 
	// 可以见 src/cmd/gc/go.y -> arg_type{} 块的定义.
	named = 0;
	for(l=all; l; l=l->next) {
		if(l->n->op == OKEY) {
			named = 1;
			break;
		}
	}
	if(named) {
		n = N;
		for(l=all; l; l=l->next) {
			n = l->n;
			if(n->op != OKEY && n->sym == S) {
				yyerror("mixed named and unnamed function parameters");
				break;
			}
		}
		if(l == nil && n != N && n->op != OKEY) {
			yyerror("final function parameter must have type");
		}
	}

	nextt = nil;
	for(l=all; l; l=l->next) {
		// can cache result from findtype to avoid quadratic behavior here,
		// but unlikely to matter.
		n = l->n;
		if(named) {
			if(n->op == OKEY) {
				// 按照 OKEY 的定义, 以及参数名称与类型的常用格式, 
				// 可以确定 left 为参数名, right 为参数类型.
				t = n->right;
				n = n->left;
				nextt = nil;
			} else {
				if(nextt == nil) {
					nextt = findtype(l);
				}
				t = nextt;
			}
		} else {
			t = n;
			n = N;
		}

		// during import l->n->op is OKEY, but l->n->left->sym == S
		// means it was a '?',
		// not that it was a lone type This doesn't matter for the exported
		// declarations,
		// which are parsed by rules that don't use checkargs,
		// but can happen for func literals in the inline bodies.
		// TODO(rsc) this can go when typefmt case TFIELD in exportmode
		// fmt.c prints _ instead of ?
		if(importpkg && n->sym == S) {
			n = N;
		}

		if(n != N && n->sym == S) {
			t = n;
			n = N;
		}
		if(n != N) {
			n = newname(n->sym);
		}
		n = nod(ODCLFIELD, n, t);
		if(n->right != N && n->right->op == ODDD) {
			if(!input) {
				yyerror("cannot use ... in output argument list");
			}
			else if(l->next != nil) {
				yyerror("can only use ... as final argument in list");
			}
			n->right->op = OTARRAY;
			n->right->right = n->right->left;
			n->right->left = N;
			n->isddd = 1;
			if(n->left != N) {
				n->left->isddd = 1;
			}
		}
		l->n = n;
	}
	return all;
}
