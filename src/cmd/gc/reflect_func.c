#include "reflect.h"

int sigcmp(Sig *a, Sig *b)
{
	int i;

	i = strcmp(a->name, b->name);
	if(i != 0)
		return i;
	if(a->pkg == b->pkg)
		return 0;
	if(a->pkg == nil)
		return -1;
	if(b->pkg == nil)
		return +1;
	return strcmp(a->pkg->path->s, b->pkg->path->s);
}

int haspointers(Type *t)
{
	Type *t1;
	int ret;

	if(t->haspointers != 0)
		return t->haspointers - 1;

	switch(t->etype) {
	case TINT:
	case TUINT:
	case TINT8:
	case TUINT8:
	case TINT16:
	case TUINT16:
	case TINT32:
	case TUINT32:
	case TINT64:
	case TUINT64:
	case TUINTPTR:
	case TFLOAT32:
	case TFLOAT64:
	case TCOMPLEX64:
	case TCOMPLEX128:
	case TBOOL:
		ret = 0;
		break;
	case TARRAY:
		if(t->bound < 0) {	// slice
			ret = 1;
			break;
		}
		ret = haspointers(t->type);
		break;
	case TSTRUCT:
		ret = 0;
		for(t1=t->type; t1!=T; t1=t1->down) {
			if(haspointers(t1->type)) {
				ret = 1;
				break;
			}
		}
		break;
	case TSTRING:
	case TPTR32:
	case TPTR64:
	case TUNSAFEPTR:
	case TINTER:
	case TCHAN:
	case TMAP:
	case TFUNC:
	default:
		ret = 1;
		break;
	}
	
	t->haspointers = 1+ret;
	return ret;
}

// 将一个 method 方法, 转换成一个常规 function 函数.
// (将 method 的 receiver 作为第0个入参).
//
// 	@param f: 一个函数类型对象
// 	@param receiver: 如果 f 是某个对象的成员方法, 那 receiver 则为其所属对象, T 或 T*.
// 	如果 f 是某个接口的成员方法, receiver 可以是 0(即 nil)
//
// caller:
// 	1. implements()
//
// f is method type, with receiver.
// return function type, receiver as first argument (or not).
// 
Type* methodfunc(Type *f, Type *receiver)
{
	NodeList *in, *out;
	Node *d;
	Type *t;

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
	in = nil;
	if(receiver) {
		d = nod(ODCLFIELD, N, N);
		d->type = receiver;
		in = list(in, d);
	}
	for(t=getinargx(f)->type; t; t=t->down) {
		d = nod(ODCLFIELD, N, N);
		d->type = t->type;
		d->isddd = t->isddd;
		in = list(in, d);
	}

	out = nil;
	for(t=getoutargx(f)->type; t; t=t->down) {
		d = nod(ODCLFIELD, N, N);
		d->type = t->type;
		out = list(out, d);
	}

	t = functype(N, in, out);
	if(f->nname) {
		// Link to name of original method function.
		t->nname = f->nname;
	}
	return t;
}

// caller:
//  1. dgcsym1() 只有这一处
int gcinline(Type *t)
{
	switch(t->etype) {
	case TARRAY:
		if(t->bound == 1)
			return 1;
		if(t->width <= 4*widthptr)
			return 1;
		break;
	}
	return 0;
}
