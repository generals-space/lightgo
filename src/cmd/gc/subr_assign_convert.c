
#include	<u.h>
#include	<libc.h>
#include	"go.h"

// assignop 判断 src 对象是否可以赋值给 dst 类型.
// 一般出现在将 strcut{} 结构体对象(src) 赋值给某个 interface 接口(dst)类型对象.
//
// 	@param src: struct{} 结构体对象
// 	@param dst: 接口对象
//
// caller:
// 	1. convertop()
// 	2. assignconv()
//
// Is type src assignment compatible to type dst?
// If so, return op code to use in conversion.
// If not, return 0.
int assignop(Type *src, Type *dst, char **why)
{
	Type *missing, *have;
	int ptr;

	if(why != nil)
		*why = "";

	// TODO(rsc,lvd): This behaves poorly in the presence of inlining.
	// https://code.google.com/p/go/issues/detail?id=2795
	if(safemode && importpkg == nil && src != T && src->etype == TUNSAFEPTR) {
		yyerror("cannot use unsafe.Pointer");
		errorexit();
	}

	if(src == dst) {
		return OCONVNOP;
	}
	if(src == T || dst == T || src->etype == TFORW || dst->etype == TFORW || src->orig == T || dst->orig == T) {
		return 0;
	}

	// 1. 两者本来就是同一类型, 无需做转换. OCONVNOP 的注解已经给出了具体场景.
	//
	// 1. src type is identical to dst.
	if(eqtype(src, dst)) {
		return OCONVNOP;
	}
	
	// 2. 底层类型相同, 是啥情况?
	//
	// 2. src and dst have identical underlying types
	// and either src or dst is not a named type or
	// both are interface types.
	if(eqtype(src->orig, dst->orig) && (src->sym == S || dst->sym == S || src->etype == TINTER)) {
		return OCONVNOP;
	}

	// 3. 类型断言, 如 "var inter 接口类型 = struct{} 结构体对象".
	//
	// 3. dst is an interface type and src implements dst.
	if(dst->etype == TINTER && src->etype != TNIL) {
		if(implements(src, dst, &missing, &have, &ptr)) {
			return OCONVIFACE;
		}

		// we'll have complained about this method anyway, supress spurious messages.
		if(have && have->sym == missing->sym && (have->type->broke || missing->type->broke)) {
			return OCONVIFACE;
		}

		if(why != nil) {
			if(isptrto(src, TINTER)) {
				*why = smprint(":\n\t%T is pointer to interface, not interface", src);
			}
			else if(have && have->sym == missing->sym) {
				*why = smprint(
					":\n\t%T does not implement %T (wrong type for %S method)\n"
					"\t\thave %S%hhT\n\t\twant %S%hhT", src, dst, missing->sym,
					have->sym, have->type, missing->sym, missing->type
				);
			}
			else if(ptr) {
				*why = smprint(
					":\n\t%T does not implement %T (%S method has pointer receiver)",
					src, dst, missing->sym
				);
			}
			else if(have) {
				*why = smprint(
					":\n\t%T does not implement %T (missing %S method)\n"
					"\t\thave %S%hhT\n\t\twant %S%hhT", 
					src, dst, missing->sym,
					have->sym, have->type, missing->sym, missing->type
				);
			}
			else {
				// [anchor] 运行到该 if 块内的示例, 请见 014.interface 中的 func02() 函数.
				//
				// 这是个编译阶段的报错.
				*why = smprint(
					":\n\t%T does not implement %T (missing %S method)",
					src, dst, missing->sym
				);
			}
		}
		return 0;
	}
	if(isptrto(dst, TINTER)) {
		if(why != nil)
			*why = smprint(":\n\t%T is pointer to interface, not interface", dst);
		return 0;
	}
	if(src->etype == TINTER && dst->etype != TBLANK) {
		if(why != nil && implements(dst, src, &missing, &have, &ptr))
			*why = ": need type assertion";
		return 0;
	}

	// 4. src is a bidirectional channel value, dst is a channel type,
	// src and dst have identical element types, and
	// either src or dst is not a named type.
	if(src->etype == TCHAN && src->chan == Cboth && dst->etype == TCHAN)
	if(eqtype(src->type, dst->type) && (src->sym == S || dst->sym == S))
		return OCONVNOP;

	// 5. src is the predeclared identifier nil and dst is a nillable type.
	if(src->etype == TNIL) {
		switch(dst->etype) {
		case TARRAY:
			if(dst->bound != -100)	// not slice
				break;
		case TPTR32:
		case TPTR64:
		case TFUNC:
		case TMAP:
		case TCHAN:
		case TINTER:
			return OCONVNOP;
		}
	}

	// 6. rule about untyped constants - already converted by defaultlit.
	
	// 7. Any typed value can be assigned to the blank identifier.
	if(dst->etype == TBLANK)
		return OCONVNOP;

	return 0;
}

// Can we convert a value of type src to a value of type dst?
// If so, return op code to use in conversion (maybe OCONVNOP).
// If not, return 0.
int convertop(Type *src, Type *dst, char **why)
{
	int op;
	
	if(why != nil)
		*why = "";

	if(src == dst)
		return OCONVNOP;
	if(src == T || dst == T)
		return 0;
	
	// 1. src can be assigned to dst.
	if((op = assignop(src, dst, why)) != 0)
		return op;

	// The rules for interfaces are no different in conversions
	// than assignments.  If interfaces are involved, stop now
	// with the good message from assignop.
	// Otherwise clear the error.
	if(src->etype == TINTER || dst->etype == TINTER)
		return 0;
	if(why != nil)
		*why = "";

	// 2. src and dst have identical underlying types.
	if(eqtype(src->orig, dst->orig))
		return OCONVNOP;
	
	// 3. src and dst are unnamed pointer types 
	// and their base types have identical underlying types.
	if(isptr[src->etype] && isptr[dst->etype] && src->sym == S && dst->sym == S)
	if(eqtype(src->type->orig, dst->type->orig))
		return OCONVNOP;

	// 4. src and dst are both integer or floating point types.
	if((isint[src->etype] || isfloat[src->etype]) && (isint[dst->etype] || isfloat[dst->etype])) {
		if(simtype[src->etype] == simtype[dst->etype])
			return OCONVNOP;
		return OCONV;
	}

	// 5. src and dst are both complex types.
	if(iscomplex[src->etype] && iscomplex[dst->etype]) {
		if(simtype[src->etype] == simtype[dst->etype])
			return OCONVNOP;
		return OCONV;
	}

	// 6. src is an integer or has type []byte or []rune
	// and dst is a string type.
	if(isint[src->etype] && dst->etype == TSTRING)
		return ORUNESTR;

	if(isslice(src) && dst->etype == TSTRING) {
		if(src->type->etype == bytetype->etype)
			return OARRAYBYTESTR;
		if(src->type->etype == runetype->etype)
			return OARRAYRUNESTR;
	}
	
	// 7. src is a string and dst is []byte or []rune.
	// String to slice.
	if(src->etype == TSTRING && isslice(dst)) {
		if(dst->type->etype == bytetype->etype)
			return OSTRARRAYBYTE;
		if(dst->type->etype == runetype->etype)
			return OSTRARRAYRUNE;
	}
	
	// 8. src is a pointer or uintptr and dst is unsafe.Pointer.
	if((isptr[src->etype] || src->etype == TUINTPTR) && dst->etype == TUNSAFEPTR)
		return OCONVNOP;

	// 9. src is unsafe.Pointer and dst is a pointer or uintptr.
	if(src->etype == TUNSAFEPTR && (isptr[dst->etype] || dst->etype == TUINTPTR))
		return OCONVNOP;

	return 0;
}

// caller:
// 	1. src/cmd/gc/typecheck_assign.c -> typecheckas()
//
// Convert node n for assignment to type t.
Node* assignconv(Node *n, Type *t, char *context)
{
	int op;
	Node *r, *old;
	char *why;
	
	if(n == N || n->type == T || n->type->broke) {
		return n;
	}

	if(t->etype == TBLANK && n->type->etype == TNIL) {
		yyerror("use of untyped nil");
	}

	old = n;
	old->diag++;  // silence errors about n; we'll issue one below
	defaultlit(&n, t);
	old->diag--;
	if(t->etype == TBLANK) {
		return n;
	}

	// Convert ideal bool from comparison to plain bool
	// if the next step is non-bool (like interface{}).
	if(n->type == idealbool && t->etype != TBOOL) {
		if(n->op == ONAME || n->op == OLITERAL) {
			r = nod(OCONVNOP, n, N);
			r->type = types[TBOOL];
			r->typecheck = 1;
			r->implicit = 1;
			n = r;
		}
	}

	if(eqtype(n->type, t)) {
		return n;
	}

	op = assignop(n->type, t, &why);
	if(op == 0) {
		yyerror("cannot use %lN as type %T in %s%s", n, t, context, why);
		op = OCONV;
	}

	r = nod(op, n, N);
	r->type = t;
	r->typecheck = 1;
	r->implicit = 1;
	r->orig = n->orig;
	return r;
}

////////////////////////////////////////////////////////////////////////////////

// caller: none 好像没有地方用到.
//
// Is a conversion between t1 and t2 a no-op?
int noconv(Type *t1, Type *t2)
{
	int e1, e2;

	e1 = simtype[t1->etype];
	e2 = simtype[t2->etype];

	switch(e1) {
	case TINT8:
	case TUINT8:
		return e2 == TINT8 || e2 == TUINT8;

	case TINT16:
	case TUINT16:
		return e2 == TINT16 || e2 == TUINT16;

	case TINT32:
	case TUINT32:
	case TPTR32:
		return e2 == TINT32 || e2 == TUINT32 || e2 == TPTR32;

	case TINT64:
	case TUINT64:
	case TPTR64:
		return e2 == TINT64 || e2 == TUINT64 || e2 == TPTR64;

	case TFLOAT32:
		return e2 == TFLOAT32;

	case TFLOAT64:
		return e2 == TFLOAT64;
	}
	return 0;
}
