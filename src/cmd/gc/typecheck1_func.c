#include "typecheck.h"

// caller:
// 	1. typekind() 只有这一处
static char* _typekind[] = {
	[TINT]		= "int",
	[TUINT]		= "uint",
	[TINT8]		= "int8",
	[TUINT8]	= "uint8",
	[TINT16]	= "int16",
	[TUINT16]	= "uint16",
	[TINT32]	= "int32",
	[TUINT32]	= "uint32",
	[TINT64]	= "int64",
	[TUINT64]	= "uint64",
	[TUINTPTR]	= "uintptr",
	[TCOMPLEX64]	= "complex64",
	[TCOMPLEX128]	= "complex128",
	[TFLOAT32]	= "float32",
	[TFLOAT64]	= "float64",
	[TBOOL]		= "bool",
	[TSTRING]	= "string",
	[TPTR32]	= "pointer",
	[TPTR64]	= "pointer",
	[TUNSAFEPTR]	= "unsafe.Pointer",
	[TSTRUCT]	= "struct",
	[TINTER]	= "interface",
	[TCHAN]		= "chan",
	[TMAP]		= "map",
	[TARRAY]	= "array",
	[TFUNC]		= "func",
	[TNIL]		= "nil",
	[TIDEAL]	= "untyped number",
};

// 根据目标 type 对象的 etype 成员, 返回对应的类型名称, 如"struct", "array", "func"等.
//
// caller:
// 	1. typecheck1() 只有这一处
char* typekind(Type *t)
{
	int et;
	static char buf[50];
	char *s;
	
	if(isslice(t)) {
		return "slice";
	}
	et = t->etype;
	if(0 <= et && et < nelem(_typekind) && (s=_typekind[et]) != nil) {
		return s;
	}
	snprint(buf, sizeof buf, "etype=%d", et);
	return buf;
}

// caller:
// 	1. typecheck1() 只有这一处
//
// indexlit implements typechecking of untyped values as
// array/slice indexes. It is equivalent to defaultlit
// except for constants of numerical kind, which are acceptable
// whenever they can be represented by a value of type int.
void indexlit(Node **np)
{
	Node *n;

	n = *np;
	if(n == N || !isideal(n->type))
		return;
	switch(consttype(n)) {
	case CTINT:
	case CTRUNE:
	case CTFLT:
	case CTCPLX:
		defaultlit(np, types[TINT]);
		break;
	}
	defaultlit(np, T);
}

// caller:
//  1. typecheck1() 只有这一处
int checksliceindex(Node *r, Type *tp)
{
	Type *t;

	if((t = r->type) == T)
		return -1;
	if(!isint[t->etype]) {
		yyerror("invalid slice index %N (type %T)", r, t);
		return -1;
	}
	if(r->op == OLITERAL) {
		if(mpgetfix(r->val.u.xval) < 0) {
			yyerror("invalid slice index %N (index must be non-negative)", r);
			return -1;
		} else if(tp != nil && tp->bound > 0 && mpgetfix(r->val.u.xval) > tp->bound) {
			yyerror("invalid slice index %N (out of bounds for %d-element array)", r, tp->bound);
			return -1;
		} else if(mpcmpfixfix(r->val.u.xval, maxintval[TINT]) > 0) {
			yyerror("invalid slice index %N (index too large)", r);
			return -1;
		}
	}
	return 0;
}

// caller:
//  1. typecheck1() 只有这一处
int checksliceconst(Node *lo, Node *hi)
{
	if(lo != N && hi != N && lo->op == OLITERAL && hi->op == OLITERAL
	   && mpcmpfixfix(lo->val.u.xval, hi->val.u.xval) > 0) {
		yyerror("invalid slice index: %N > %N", lo, hi);
		return -1;
	}
	return 0;
}

// 处理 defer() 语句.
//
// caller:
//  1. typecheck1() 只有这一处
void checkdefergo(Node *n)
{
	char *what;
	
	what = "defer";
	if(n->op == OPROC)
		what = "go";

	switch(n->left->op) {
	case OCALLINTER:
	case OCALLMETH:
	case OCALLFUNC:
	case OCLOSE:
	case OCOPY:
	case ODELETE:
	case OPANIC:
	case OPRINT:
	case OPRINTN:
	case ORECOVER:
		// ok
		break;
	case OAPPEND:
	case OCAP:
	case OCOMPLEX:
	case OIMAG:
	case OLEN:
	case OMAKE:
	case OMAKESLICE:
	case OMAKECHAN:
	case OMAKEMAP:
	case ONEW:
	case OREAL:
	case OLITERAL: // conversion or unsafe.Alignof, Offsetof, Sizeof
		if(n->left->orig != N && n->left->orig->op == OCONV)
			goto conv;
		yyerror("%s discards result of %N", what, n->left);
		break;
	default:
	conv:
		// type is broken or missing, most likely a method call on a broken type
		// we will warn about the broken type elsewhere.
		// no need to emit a potentially confusing error
		if(n->left->type == T || n->left->type->broke)
			break;

		if(!n->diag) {
			// The syntax made sure it was a call, so this must be
			// a conversion.
			n->diag = 1;
			yyerror("%s requires function call, not conversion", what);
		}
		break;
	}
}

// caller:
//  1. typecheck1() 只有这一处
void implicitstar(Node **nn)
{
	Type *t;
	Node *n;

	// insert implicit * if needed for fixed array
	n = *nn;
	t = n->type;
	if(t == T || !isptr[t->etype])
		return;
	t = t->type;
	if(t == T)
		return;
	if(!isfixedarray(t))
		return;
	n = nod(OIND, n, N);
	n->implicit = 1;
	typecheck(&n, Erv);
	*nn = n;
}

// caller:
//  1. typecheck1() 只有这一处
int onearg(Node *n, char *f, ...)
{
	va_list arg;
	char *p;

	if(n->left != N)
		return 0;
	if(n->list == nil) {
		va_start(arg, f);
		p = vsmprint(f, arg);
		va_end(arg);
		yyerror("missing argument to %s: %N", p, n);
		return -1;
	}
	if(n->list->next != nil) {
		va_start(arg, f);
		p = vsmprint(f, arg);
		va_end(arg);
		yyerror("too many arguments to %s: %N", p, n);
		n->left = n->list->n;
		n->list = nil;
		return -1;
	}
	n->left = n->list->n;
	n->list = nil;
	return 0;
}

// caller:
//  1. typecheck1() 只有这一处
int twoarg(Node *n)
{
	if(n->left != N)
		return 0;
	if(n->list == nil) {
		yyerror("missing argument to %O - %N", n->op, n);
		return -1;
	}
	n->left = n->list->n;
	if(n->list->next == nil) {
		yyerror("missing argument to %O - %N", n->op, n);
		n->list = nil;
		return -1;
	}
	if(n->list->next->next != nil) {
		yyerror("too many arguments to %O - %N", n->op, n);
		n->list = nil;
		return -1;
	}
	n->right = n->list->next->n;
	n->list = nil;
	return 0;
}

// 判断变量类型是否一致, 因为之后要进行赋值.
//
// 从目前来看, 该函数有如下2种使用场景:
// 	1. 检测传入的参数列表的类型, 与函数声明中的参数列表是否一致;
// 	2. 检测函数返回的变量的类型, 是函数声明中的返回值列表是否一致;
//
// 	@param op: 只有2个选项: OCALL(传入参数)/ORETURN(返回值);
// 	@param call: 如果 op 是 OCALL, 则为主调函数的 Node, 如果 op 是 ORETURN, 则为 nil.
// 	@param tstruct: 目标函数声明中定义的参数列表或返回值列表(即形参)
// 	@param nl: 实际传入的参数列表或返回值列表(即实参)
// 	@param desc: 当 op == OCALL 时, desc = "function argument", 当 op == ORETURN 时,
// 	desc = "return argument"
//
// caller:
//  1. typecheck1() 只有这一处
//
// typecheck assignment: type list = expression list
void typecheckaste(int op, Node *call, int isddd, Type *tstruct, NodeList *nl, char *desc)
{
	Type *t;
	// 形参中的某个参数的类型
	Type *tl;
	// 实参中的某个参数的类型
	Type *tn;
	Node *n;
	int lno;
	char *why;

	lno = lineno;

	if(tstruct->broke) {
		goto out;
	}

	if(nl != nil && nl->next == nil && (n = nl->n)->type != T) {
		if(n->type->etype == TSTRUCT && n->type->funarg) {
			tn = n->type->type;
			for(tl=tstruct->type; tl; tl=tl->down) {
				if(tl->isddd) {
					for(; tn; tn=tn->down) {
						if(assignop(tn->type, tl->type->type, &why) == 0) {
							if(call != N) {
								yyerror(
									"cannot use %T as type %T in argument to %N%s", 
									tn->type, tl->type->type, call, why
								);
							}
							else {
								yyerror(
									"cannot use %T as type %T in %s%s", 
									tn->type, tl->type->type, desc, why
								);
							}
						}
					}
					goto out;
				}
				if(tn == T) {
					goto notenough;
				}
				if(assignop(tn->type, tl->type, &why) == 0) {
					if(call != N) {
						yyerror(
							"cannot use %T as type %T in argument to %N%s", 
							tn->type, tl->type, call, why
						);
					}
					else {
						yyerror(
							"cannot use %T as type %T in %s%s", 
							tn->type, tl->type, desc, why
						);
					}
				}
				tn = tn->down;
			}
			if(tn != T) {
				goto toomany;
			}
			goto out;
		}
	}

	for(tl=tstruct->type; tl; tl=tl->down) {
		t = tl->type;
		if(tl->isddd) {
			if(isddd) {
				if(nl == nil) {
					goto notenough;
				}
				if(nl->next != nil) {
					goto toomany;
				}
				n = nl->n;
				setlineno(n);
				if(n->type != T) {
					nl->n = assignconv(n, t, desc);
				}
				goto out;
			}
			for(; nl; nl=nl->next) {
				n = nl->n;
				setlineno(nl->n);
				if(n->type != T) {
					nl->n = assignconv(n, t->type, desc);
				}
			}
			goto out;
		}
		if(nl == nil) {
			goto notenough;
		}
		n = nl->n;
		setlineno(n);
		if(n->type != T) {
			nl->n = assignconv(n, t, desc);
		}
		nl = nl->next;
	}
	if(nl != nil) {
		goto toomany;
	}
	if(isddd) {
		if(call != N) {
			yyerror("invalid use of ... in call to %N", call);
		}
		else {
			yyerror("invalid use of ... in %O", op);
		}
	}

out:
	lineno = lno;
	return;

notenough:
	if(call != N) {
		yyerror("not enough arguments in call to %N", call);
	}
	else {
		yyerror("not enough arguments to %O", op);
	}
	goto out;

toomany:
	if(call != N) {
		yyerror("too many arguments in call to %N", call);
	}
	else {
		yyerror("too many arguments to %O", op);
	}
	goto out;
}

/*
 * type check function definition
 */

// caller:
//  1. typecheck1() 只有这一处
void typecheckfunc(Node *n)
{
	Type *t, *rcvr;

	typecheck(&n->nname, Erv | Easgn);
	if((t = n->nname->type) == T)
		return;
	n->type = t;
	t->nname = n->nname;
	rcvr = getthisx(t)->type;
	if(rcvr != nil && n->shortname != N && !isblank(n->shortname))
		addmethod(n->shortname->sym, t, 1, n->nname->nointerface);
}

// caller:
//  1. typecheck1() 只有这一处
void stringtoarraylit(Node **np)
{
	int32 i;
	NodeList *l;
	Strlit *s;
	char *p, *ep;
	Rune r;
	Node *nn, *n;

	n = *np;
	if(n->left->op != OLITERAL || n->left->val.ctype != CTSTR)
		fatal("stringtoarraylit %N", n);

	s = n->left->val.u.sval;
	l = nil;
	p = s->s;
	ep = s->s + s->len;
	i = 0;
	if(n->type->type->etype == TUINT8) {
		// raw []byte
		while(p < ep)
			l = list(l, nod(OKEY, nodintconst(i++), nodintconst((uchar)*p++)));
	} else {
		// utf-8 []rune
		while(p < ep) {
			p += chartorune(&r, p);
			l = list(l, nod(OKEY, nodintconst(i++), nodintconst(r)));
		}
	}
	nn = nod(OCOMPLIT, N, typenod(n->type));
	nn->list = l;
	typecheck(&nn, Erv);
	*np = nn;
}
