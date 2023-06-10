#include	<u.h>
#include	<libc.h>
#include	"go.h"

/*
 * iterator to this and inargs in a function
 */
Type* funcfirst(Iter *s, Type *t)
{
	Type *fp;

	if(t == T)
		goto bad;

	if(t->etype != TFUNC)
		goto bad;

	s->tfunc = t;
	s->done = 0;
	fp = structfirst(s, getthis(t));
	if(fp == T) {
		s->done = 1;
		fp = structfirst(s, getinarg(t));
	}
	return fp;

bad:
	fatal("funcfirst: not func %T", t);
	return T;
}

Type* funcnext(Iter *s)
{
	Type *fp;

	fp = structnext(s);
	if(fp == T && !s->done) {
		s->done = 1;
		fp = structfirst(s, getinarg(s->tfunc));
	}
	return fp;
}

// Type 类型的 type 成员属性, 一般 Type 类型为 Array, Map, Channel 等,
// 可以称为"复合类型", 而 type 成员则表示ta们的成员的类型.
// 有一些特殊情况, 如
// Interface 类型的 type 可以是 Func 类型
// Func 类型的 type 可以是其参数的类型, 但 Func 有入参和返回值两类, 因此还要更复杂一些.
//
//                             入参类型[0]    入参类型[1]
//                             Type(type) -> Type(down) ...
//               返回值         |
// Type(Func) -> Type(down) -> Type(down)
//               |             入参
//               |
//               Type(type) -> Type(down) ...
//               返回值类型[0]   返回值类型[1]
//

Type** getthis(Type *t)
{
	if(t->etype != TFUNC) {
		fatal("getthis: not a func %T", t);
	}
	return &t->type;
}

Type** getoutarg(Type *t)
{
	if(t->etype != TFUNC) {
		fatal("getoutarg: not a func %T", t);
	}
	return &t->type->down;
}

Type** getinarg(Type *t)
{
	if(t->etype != TFUNC) {
		fatal("getinarg: not a func %T", t);
	}
	return &t->type->down->down;
}

Type* getthisx(Type *t)
{
	return *getthis(t);
}

Type* getoutargx(Type *t)
{
	return *getoutarg(t);
}

Type* getinargx(Type *t)
{
	return *getinarg(t);
}
