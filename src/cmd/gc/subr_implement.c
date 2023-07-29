#include	<u.h>
#include	<libc.h>
#include	"go.h"

// 一个工具函数
//
// caller:
// 	1. lookdot0()
//
// unicode-aware case-insensitive strcmp
static int ucistrcmp(char *p, char *q)
{
	Rune rp, rq;

	while(*p || *q) {
		if(*p == 0)
			return +1;
		if(*q == 0)
			return -1;
		p += chartorune(&rp, p);
		q += chartorune(&rq, q);
		rp = tolowerrune(rp);
		rq = tolowerrune(rq);
		if(rp < rq)
			return -1;
		if(rp > rq)
			return +1;
	}
	return 0;
}

/*
 * code to resolve elided DOTs in embedded types
 */

// caller:
// 	1. adddot1() 只有这一处
//
// search depth 0 --
// return count of fields+methods
// found with a given name
static int lookdot0(Sym *s, Type *t, Type **save, int ignorecase)
{
	Type *f, *u;
	int c;

	u = t;
	if(isptr[u->etype])
		u = u->type;

	c = 0;
	if(u->etype == TSTRUCT || u->etype == TINTER) {
		for(f=u->type; f!=T; f=f->down)
			if(f->sym == s || (ignorecase && f->type->etype == TFUNC && f->type->thistuple > 0 && ucistrcmp(f->sym->name, s->name) == 0)) {
				if(save)
					*save = f;
				c++;
			}
	}
	u = methtype(t, 0);
	if(u != T) {
		for(f=u->method; f!=T; f=f->down)
			if(f->embedded == 0 && (f->sym == s || (ignorecase && ucistrcmp(f->sym->name, s->name) == 0))) {
				if(save)
					*save = f;
				c++;
			}
	}
	return c;
}

// caller:
// 	1. ifacelookdot()
// 	2. adddot()
// 	3. adddot1()
//
// search depth d for field/method s --
// return count of fields+methods
// found at search depth.
// answer is in dotlist array and
// count of number of ways is returned.
int adddot1(Sym *s, Type *t, int d, Type **save, int ignorecase)
{
	Type *f, *u;
	int c, a;

	if(t->trecur)
		return 0;
	t->trecur = 1;

	if(d == 0) {
		c = lookdot0(s, t, save, ignorecase);
		goto out;
	}

	c = 0;
	u = t;
	if(isptr[u->etype])
		u = u->type;
	if(u->etype != TSTRUCT && u->etype != TINTER)
		goto out;

	d--;
	for(f=u->type; f!=T; f=f->down) {
		if(!f->embedded)
			continue;
		if(f->sym == S)
			continue;
		a = adddot1(s, f->type, d, save, ignorecase);
		if(a != 0 && c == 0)
			dotlist[d].field = f;
		c += a;
	}

out:
	t->trecur = 0;
	return c;
}

// in T.field
// find missing fields that
// will give shortest unique addressing.
// modify the tree with missing type names.
Node* adddot(Node *n)
{
	Type *t;
	Sym *s;
	int c, d;

	typecheck(&n->left, Etype|Erv);
	t = n->left->type;
	if(t == T)
		goto ret;
	
	if(n->left->op == OTYPE)
		goto ret;

	if(n->right->op != ONAME)
		goto ret;
	s = n->right->sym;
	if(s == S)
		goto ret;

	for(d=0; d<nelem(dotlist); d++) {
		c = adddot1(s, t, d, nil, 0);
		if(c > 0)
			goto out;
	}
	goto ret;

out:
	if(c > 1) {
		yyerror("ambiguous selector %N", n);
		n->left = N;
		return n;
	}

	// rebuild elided dots
	for(c=d-1; c>=0; c--)
		n->left = nod(ODOT, n->left, newname(dotlist[c].field->sym));
ret:
	return n;
}

// 在一个拥有方法的对象 t 中, 寻找名称为 s 的方法并返回.
// 理论上返回的方法中, 入参是包含 receiver 的.
//
// 函数名中的 dot 点号, 这是因为 golang 的结构体对象是可以组合的, 对于一个成员方法, 
// 可能出现 A.B.C.D.method() 的情况
//
// 	@param t: 一个拥有 method 方法的对象, 不一定是 struct(但此时一定不会是指针类型).
//
// caller:
// 	1. implements() 只有这一处, 用于判断某个 struct 对象是否实现了某接口时被调用.
static Type* ifacelookdot(Sym *s, Type *t, int *followptr, int ignorecase)
{
	int i, c, d;
	Type *m;

	*followptr = 0;

	if(t == T) {
		return T;
	}
	// golang 的结构体对象是可以组合的, 对于一个成员方法, 可能出现 A.B.C.D.method() 的情况,
	// 不过有最大嵌套层数的限制, 即 dotlist.
	for(d=0; d<nelem(dotlist); d++) {
		c = adddot1(s, t, d, &m, ignorecase);
		if(c > 1) {
			yyerror("%T.%S is ambiguous", t, s);
			return T;
		}
		if(c == 1) {
			for(i=0; i<d; i++) {
				if(isptr[dotlist[i].field->type->etype]) {
					*followptr = 1;
					break;
				}
			}
			if(m->type->etype != TFUNC || m->type->thistuple == 0) {
				yyerror("%T.%S is a field, not a method", t, s);
				return T;
			}
			return m;
		}
	}
	return T;
}

// implements 类型断言, 如 "var inter 接口类型 = struct{} 结构体对象"
//
// 	@param t: struct{} 结构体对象
// 	@param iface: 某个接口类型
// 	@param m: 如果没有完全实现, 则要告知缺失了哪些方法, 都放在了该参数中.
//
// 	@return: 1为已实现, 0为未实现
//
// caller:
// 	1. assignop()
// 	2. src/cmd/gc/typecheck.c -> typecheck1()
// 	3. src/cmd/gc/swt.c -> typecheckswitch()
int implements(Type *t, Type *iface, Type **m, Type **samename, int *ptr)
{
	Type *t0;
	// 被赋值的 interface 类型
	Type *im;
	// 待赋值的 struct 类型(或是别名类型, 也有可能是其他的 interface 类型)
	Type *tm;
	// tm 中上挂载的方法中, receiver 的类型(有可能是 tm 本身, 但也有可能是 *tm)
	Type *rcvr;
	Type *imtype;
	int followptr;

	t0 = t;
	if(t == T) {
		return 0;
	}

	// if this is too slow, could sort these first and then do one loop.

	// 如果 t 也是接口类型, 那就要判断 t 是否为 iface 的父接口了,
	// 因为 t 要实现 iface 的所有方法才行.
	if(t->etype == TINTER) {
		// 双层遍历, 确保 iface 中的第一个方法都能在 t 中找到
		for(im=iface->type; im; im=im->down) {
			for(tm=t->type; tm; tm=tm->down) {
				if(tm->sym == im->sym) {
					if(eqtype(tm->type, im->type)) {
						goto found;
					}
					// 没找到, 则返回.
					*m = im;
					*samename = tm;
					*ptr = 0;
					return 0;
				}
			}
			// 没找到, 则返回.
			*m = im;
			*samename = nil;
			*ptr = 0;
			return 0;
		found:;
		}
		return 1;
	}

	// 运行到这里, 说明 t 并不是接口类型, 那一般就是类型断言了.

	t = methtype(t, 0);
	if(t != T) {
		expandmeth(t);
	}
	for(im=iface->type; im; im=im->down) {
		imtype = methodfunc(im->type, 0);
		tm = ifacelookdot(im->sym, t, &followptr, 0);
		if(tm == T || tm->nointerface || !eqtype(methodfunc(tm->type, 0), imtype)) {
			if(tm == T) {
				tm = ifacelookdot(im->sym, t, &followptr, 1);
			}
			*m = im;
			*samename = tm;
			*ptr = 0;
			return 0;
		}
		// 运行到这里, 说明 im->type 和 tm->type 所表示的方法是相同的, 但还有一个特殊情况.

		// 如果 tm 方法的 receiver 是指针类型, 而 tm 本身不是指针, 那表示并不匹配.
		//
		// if pointer receiver in method, the method does not exist for value types.
		rcvr = getthisx(tm->type)->type->type;
		if(isptr[rcvr->etype] && !isptr[t0->etype] && !followptr && !isifacemethod(tm->type)) {
			if(0 && debug['r']) {
				yyerror("interface pointer mismatch");
			}

			*m = im;
			*samename = nil;
			*ptr = 1;
			return 0;
		}
	}
	return 1;
}
