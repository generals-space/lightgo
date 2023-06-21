#include	<u.h>
#include	<libc.h>
#include	"go.h"

int algtype(Type *t)
{
	int a;
	
	a = algtype1(t, nil);
	if(a == AMEM || a == ANOEQ) {
		if(isslice(t))
			return ASLICE;
		switch(t->width) {
		case 0:
			return a + AMEM0 - AMEM;
		case 1:
			return a + AMEM8 - AMEM;
		case 2:
			return a + AMEM16 - AMEM;
		case 4:
			return a + AMEM32 - AMEM;
		case 8:
			return a + AMEM64 - AMEM;
		case 16:
			return a + AMEM128 - AMEM;
		}
	}
	return a;
}

// t 是某个拥有 method 方法的 Type 对象(不是只有 struct 才有方法), 有可能是个指针,
// 即 t == T 或 t == T*.
// 该函数会返回 T, 即, 如果 t 是指针类型, 则返回其所指向的实际类型.
//
// given receiver of type t (t == r or t == *r)
// return type to hang methods off (r).
// hang off: 悬挂
Type* methtype(Type *t, int mustname)
{
	if(t == T) {
		return T;
	}
	// 如果 t 是指针类型, 则 t->type 表示该指针指向的变量所属的真正类型.
	// (TPTR64 和 TPTR32 也是"复合"类型)
	//
	// strip away(揭开) pointer if it's there
	if(isptr[t->etype]) {
		if(t->sym != S) {
			return T;
		}
		t = t->type;
		if(t == T) {
			return T;
		}
	}

	// need a type name
	if(t->sym == S && (mustname || t->etype != TSTRUCT)) {
		return T;
	}

	// check types
	if(!issimple[t->etype]) {
		switch(t->etype) {
		default:
			return T;
		case TSTRUCT:
		case TARRAY:
		case TMAP:
		case TCHAN:
		case TSTRING:
		case TFUNC:
			break;
		}
	}

	return t;
}

static int subtype(Type **stp, Type *t, int d)
{
	Type *st;

loop:
	st = *stp;
	if(st == T)
		return 0;

	d++;
	if(d >= 10)
		return 0;

	switch(st->etype) {
	default:
		return 0;

	case TPTR32:
	case TPTR64:
	case TCHAN:
	case TARRAY:
		stp = &st->type;
		goto loop;

	case TANY:
		if(!st->copyany)
			return 0;
		*stp = t;
		break;

	case TMAP:
		if(subtype(&st->down, t, d))
			break;
		stp = &st->type;
		goto loop;

	case TFUNC:
		for(;;) {
			if(subtype(&st->type, t, d))
				break;
			if(subtype(&st->type->down->down, t, d))
				break;
			if(subtype(&st->type->down, t, d))
				break;
			return 0;
		}
		break;

	case TSTRUCT:
		for(st=st->type; st!=T; st=st->down)
			if(subtype(&st->type, t, d))
				return 1;
		return 0;
	}
	return 1;
}

void argtype(Node *on, Type *t)
{
	dowidth(t);
	if(!subtype(&on->type, t, 0)) {
		fatal("argtype: failed %N %T\n", on, t);
	}
}

int cplxsubtype(int et)
{
	switch(et) {
	case TCOMPLEX64:
		return TFLOAT32;
	case TCOMPLEX128:
		return TFLOAT64;
	}
	fatal("cplxsubtype: %E\n", et);
	return 0;
}

void badtype(int o, Type *tl, Type *tr)
{
	Fmt fmt;
	char *s;
	
	fmtstrinit(&fmt);
	if(tl != T)
		fmtprint(&fmt, "\n	%T", tl);
	if(tr != T)
		fmtprint(&fmt, "\n	%T", tr);

	// common mistake: *struct and *interface.
	if(tl && tr && isptr[tl->etype] && isptr[tr->etype]) {
		if(tl->type->etype == TSTRUCT && tr->type->etype == TINTER)
			fmtprint(&fmt, "\n	(*struct vs *interface)");
		else if(tl->type->etype == TINTER && tr->type->etype == TSTRUCT)
			fmtprint(&fmt, "\n	(*interface vs *struct)");
	}
	s = fmtstrflush(&fmt);
	yyerror("illegal types for operand: %O%s", o, s);
}

/*
 * even simpler simtype; get rid of ptr, bool.
 * assuming that the front end has rejected
 * all the invalid conversions (like ptr -> bool)
 */
int simsimtype(Type *t)
{
	int et;

	if(t == 0)
		return 0;

	et = simtype[t->etype];
	switch(et) {
	case TPTR32:
		et = TUINT32;
		break;
	case TPTR64:
		et = TUINT64;
		break;
	case TBOOL:
		et = TUINT8;
		break;
	}
	return et;
}
