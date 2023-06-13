#include "typecheck.h"

// caller:
// 	1. typecheck()
void typecheck1(Node **np, int top)
{
	int et, aop, op, ptr;
	Node *n, *l, *r, *lo, *mid, *hi;
	NodeList *args;
	int ok, ntop;
	Type *t, *tp, *missing, *have, *badtype;
	Val v;
	char *why;
	
	n = *np;

	if(n->sym) {
		if(n->op == ONAME && n->etype != 0 && !(top & Ecall)) {
			yyerror("use of builtin %S not in function call", n->sym);
			goto error;
		}

		typecheckdef(n);
		if(n->op == ONONAME)
			goto error;
	}
	*np = n;

reswitch:
	ok = 0;
	switch(n->op) {
	default:
		// until typecheck is complete, do nothing.
		dump("typecheck", n);
		fatal("typecheck %O", n->op);

	/*
	 * names
	 */
	case OLITERAL:
		ok |= Erv;
		if(n->type == T && n->val.ctype == CTSTR)
			n->type = idealstring;
		goto ret;

	case ONONAME:
		ok |= Erv;
		goto ret;

	case ONAME:
		if(n->etype != 0) {
			ok |= Ecall;
			goto ret;
		}
		if(!(top & Easgn)) {
			// not a write to the variable
			if(isblank(n)) {
				yyerror("cannot use _ as value");
				goto error;
			}
			n->used = 1;
		}
		if(!(top &Ecall) && isunsafebuiltin(n)) {
			yyerror("%N is not an expression, must be called", n);
			goto error;
		}
		ok |= Erv;
		goto ret;

	case OPACK:
		yyerror("use of package %S not in selector", n->sym);
		goto error;

	case ODDD:
		break;

	/*
	 * types (OIND is with exprs)
	 */
	case OTYPE:
		ok |= Etype;
		if(n->type == T)
			goto error;
		break;

	case OTPAREN:
		ok |= Etype;
		l = typecheck(&n->left, Etype);
		if(l->type == T)
			goto error;
		n->op = OTYPE;
		n->type = l->type;
		n->left = N;
		break;
	
	case OTARRAY:
		ok |= Etype;
		t = typ(TARRAY);
		l = n->left;
		r = n->right;
		if(l == nil) {
			t->bound = -1;	// slice
		} else if(l->op == ODDD) {
			t->bound = -100;	// to be filled in
			if(!(top&Ecomplit) && !n->diag) {
				t->broke = 1;
				n->diag = 1;
				yyerror("use of [...] array outside of array literal");
			}
		} else {
			l = typecheck(&n->left, Erv);
			switch(consttype(l)) {
			case CTINT:
			case CTRUNE:
				v = l->val;
				break;
			case CTFLT:
				v = toint(l->val);
				break;
			default:
				yyerror("invalid array bound %N", l);
				goto error;
			}
			t->bound = mpgetfix(v.u.xval);
			if(doesoverflow(v, types[TINT])) {
				yyerror("array bound is too large"); 
				goto error;
			} else if(t->bound < 0) {
				yyerror("array bound must be non-negative");
				goto error;
			}
		}
		typecheck(&r, Etype);
		if(r->type == T) {
			goto error;
		}
		t->type = r->type;
		n->op = OTYPE;
		n->type = t;
		n->left = N;
		n->right = N;
		if(t->bound != -100)
			checkwidth(t);
		break;

	case OTMAP:
		ok |= Etype;
		l = typecheck(&n->left, Etype);
		r = typecheck(&n->right, Etype);
		if(l->type == T || r->type == T) {
			goto error;
		}
		n->op = OTYPE;
		n->type = maptype(l->type, r->type);
		n->left = N;
		n->right = N;
		break;

	case OTCHAN:
		ok |= Etype;
		l = typecheck(&n->left, Etype);
		if(l->type == T)
			goto error;
		t = typ(TCHAN);
		t->type = l->type;
		t->chan = n->etype;
		n->op = OTYPE;
		n->type = t;
		n->left = N;
		n->etype = 0;
		break;

	case OTSTRUCT:
		ok |= Etype;
		n->op = OTYPE;
		n->type = tostruct(n->list);
		if(n->type == T || n->type->broke)
			goto error;
		n->list = nil;
		break;

	case OTINTER:
		ok |= Etype;
		n->op = OTYPE;
		n->type = tointerface(n->list);
		if(n->type == T)
			goto error;
		break;

	case OTFUNC:
		ok |= Etype;
		n->op = OTYPE;
		n->type = functype(n->left, n->list, n->rlist);
		if(n->type == T)
			goto error;
		break;

	/*
	 * type or expr
	 */
	case OIND:
		ntop = Erv | Etype;
		if(!(top & Eaddr))  		// The *x in &*x is not an indirect.
			ntop |= Eindir;
		ntop |= top & Ecomplit;
		l = typecheck(&n->left, ntop);
		if((t = l->type) == T)
			goto error;
		if(l->op == OTYPE) {
			ok |= Etype;
			n->op = OTYPE;
			n->type = ptrto(l->type);
			n->left = N;
			goto ret;
		}
		if(!isptr[t->etype]) {
			if(top & (Erv | Etop)) {
				yyerror("invalid indirect of %lN", n->left);
				goto error;
			}
			goto ret;
		}
		ok |= Erv;
		n->type = t->type;
		goto ret;

	/*
	 * arithmetic exprs
	 */
	case OASOP:
		ok |= Etop;
		l = typecheck(&n->left, Erv);
		checkassign(n->left);
		r = typecheck(&n->right, Erv);
		if(l->type == T || r->type == T)
			goto error;
		op = n->etype;
		goto arith;

	case OADD:
	case OAND:
	case OANDAND:
	case OANDNOT:
	case ODIV:
	case OEQ:
	case OGE:
	case OGT:
	case OLE:
	case OLT:
	case OLSH:
	case ORSH:
	case OMOD:
	case OMUL:
	case ONE:
	case OOR:
	case OOROR:
	case OSUB:
	case OXOR:
		ok |= Erv;
		l = typecheck(&n->left, Erv | (top & Eiota));
		r = typecheck(&n->right, Erv | (top & Eiota));
		if(l->type == T || r->type == T)
			goto error;
		op = n->op;
	arith:
		if(op == OLSH || op == ORSH)
			goto shift;
		// ideal mixed with non-ideal
		defaultlit2(&l, &r, 0);
		n->left = l;
		n->right = r;
		if(l->type == T || r->type == T)
			goto error;
		t = l->type;
		if(t->etype == TIDEAL)
			t = r->type;
		et = t->etype;
		if(et == TIDEAL)
			et = TINT;
		if(iscmp[n->op] && t->etype != TIDEAL && !eqtype(l->type, r->type)) {
			// comparison is okay as long as one side is
			// assignable to the other.  convert so they have
			// the same type.
			//
			// the only conversion that isn't a no-op is concrete == interface.
			// in that case, check comparability of the concrete type.
			if(r->type->etype != TBLANK && (aop = assignop(l->type, r->type, nil)) != 0) {
				if(isinter(r->type) && !isinter(l->type) && algtype1(l->type, nil) == ANOEQ) {
					yyerror("invalid operation: %N (operator %O not defined on %s)", n, op, typekind(l->type));
					goto error;
				}
				l = nod(aop, l, N);
				l->type = r->type;
				l->typecheck = 1;
				n->left = l;
				t = l->type;
			} else if(l->type->etype != TBLANK && (aop = assignop(r->type, l->type, nil)) != 0) {
				if(isinter(l->type) && !isinter(r->type) && algtype1(r->type, nil) == ANOEQ) {
					yyerror("invalid operation: %N (operator %O not defined on %s)", n, op, typekind(r->type));
					goto error;
				}
				r = nod(aop, r, N);
				r->type = l->type;
				r->typecheck = 1;
				n->right = r;
				t = r->type;
			}
			et = t->etype;
		}
		if(t->etype != TIDEAL && !eqtype(l->type, r->type)) {
			defaultlit2(&l, &r, 1);
			yyerror("invalid operation: %N (mismatched types %T and %T)", n, l->type, r->type);
			goto error;
		}
		if(!okfor[op][et]) {
			yyerror("invalid operation: %N (operator %O not defined on %s)", n, op, typekind(t));
			goto error;
		}
		// okfor allows any array == array, map == map, func == func.
		// restrict to slice/map/func == nil and nil == slice/map/func.
		if(isfixedarray(l->type) && algtype1(l->type, nil) == ANOEQ) {
			yyerror("invalid operation: %N (%T cannot be compared)", n, l->type);
			goto error;
		}
		if(isslice(l->type) && !isnil(l) && !isnil(r)) {
			yyerror("invalid operation: %N (slice can only be compared to nil)", n);
			goto error;
		}
		if(l->type->etype == TMAP && !isnil(l) && !isnil(r)) {
			yyerror("invalid operation: %N (map can only be compared to nil)", n);
			goto error;
		}
		if(l->type->etype == TFUNC && !isnil(l) && !isnil(r)) {
			yyerror("invalid operation: %N (func can only be compared to nil)", n);
			goto error;
		}
		if(l->type->etype == TSTRUCT && algtype1(l->type, &badtype) == ANOEQ) {
			yyerror("invalid operation: %N (struct containing %T cannot be compared)", n, badtype);
			goto error;
		}
		
		t = l->type;
		if(iscmp[n->op]) {
			evconst(n);
			t = idealbool;
			if(n->op != OLITERAL) {
				defaultlit2(&l, &r, 1);
				n->left = l;
				n->right = r;
			}
		// non-comparison operators on ideal bools should make them lose their ideal-ness
		} else if(t == idealbool)
			t = types[TBOOL];

		if(et == TSTRING) {
			if(iscmp[n->op]) {
				n->etype = n->op;
				n->op = OCMPSTR;
			} else if(n->op == OADD)
				n->op = OADDSTR;
		}
		if(et == TINTER) {
			if(l->op == OLITERAL && l->val.ctype == CTNIL) {
				// swap for back end
				n->left = r;
				n->right = l;
			} else if(r->op == OLITERAL && r->val.ctype == CTNIL) {
				// leave alone for back end
			} else {
				n->etype = n->op;
				n->op = OCMPIFACE;
			}
		}

		if((op == ODIV || op == OMOD) && isconst(r, CTINT))
		if(mpcmpfixc(r->val.u.xval, 0) == 0) {
			yyerror("division by zero");
			goto error;
		} 

		n->type = t;
		goto ret;

	shift:
		defaultlit(&r, types[TUINT]);
		n->right = r;
		t = r->type;
		if(!isint[t->etype] || issigned[t->etype]) {
			yyerror("invalid operation: %N (shift count type %T, must be unsigned integer)", n, r->type);
			goto error;
		}
		t = l->type;
		if(t != T && t->etype != TIDEAL && !isint[t->etype]) {
			yyerror("invalid operation: %N (shift of type %T)", n, t);
			goto error;
		}
		// no defaultlit for left
		// the outer context gives the type
		n->type = l->type;
		goto ret;

	case OCOM:
	case OMINUS:
	case ONOT:
	case OPLUS:
		ok |= Erv;
		l = typecheck(&n->left, Erv | (top & Eiota));
		if((t = l->type) == T)
			goto error;
		if(!okfor[n->op][t->etype]) {
			yyerror("invalid operation: %O %T", n->op, t);
			goto error;
		}
		n->type = t;
		goto ret;

	/*
	 * exprs
	 */
	case OADDR:
		ok |= Erv;
		typecheck(&n->left, Erv | Eaddr);
		if(n->left->type == T)
			goto error;
		checklvalue(n->left, "take the address of");
		for(l=n->left; l->op == ODOT; l=l->left)
			l->addrtaken = 1;
		l->addrtaken = 1;
		defaultlit(&n->left, T);
		l = n->left;
		if((t = l->type) == T)
			goto error;
		// top&Eindir means this is &x in *&x.  (or the arg to built-in print)
		// n->etype means code generator flagged it as non-escaping.
		if(debug['N'] && !(top & Eindir) && !n->etype)
			addrescapes(n->left);
		n->type = ptrto(t);
		goto ret;

	case OCOMPLIT:
		ok |= Erv;
		typecheckcomplit(&n);
		if(n->type == T)
			goto error;
		goto ret;

	case OXDOT:
		n = adddot(n);
		n->op = ODOT;
		if(n->left == N)
			goto error;
		// fall through
	case ODOT:
		typecheck(&n->left, Erv|Etype);
		defaultlit(&n->left, T);
		if((t = n->left->type) == T)
			goto error;
		if(n->right->op != ONAME) {
			yyerror("rhs of . must be a name");	// impossible
			goto error;
		}
		r = n->right;

		if(n->left->op == OTYPE) {
			if(!looktypedot(n, t, 0)) {
				if(looktypedot(n, t, 1))
					yyerror("%N undefined (cannot refer to unexported method %S)", n, n->right->sym);
				else
					yyerror("%N undefined (type %T has no method %S)", n, t, n->right->sym);
				goto error;
			}
			if(n->type->etype != TFUNC || n->type->thistuple != 1) {
				yyerror("type %T has no method %hS", n->left->type, n->right->sym);
				n->type = T;
				goto error;
			}
			n->op = ONAME;
			n->sym = n->right->sym;
			n->type = methodfunc(n->type, n->left->type);
			n->xoffset = 0;
			n->class = PFUNC;
			ok = Erv;
			goto ret;
		}
		if(isptr[t->etype] && t->type->etype != TINTER) {
			t = t->type;
			if(t == T)
				goto error;
			n->op = ODOTPTR;
			checkwidth(t);
		}
		if(isblank(n->right)) {
			yyerror("cannot refer to blank field or method");
			goto error;
		}
		if(!lookdot(n, t, 0)) {
			if(lookdot(n, t, 1))
				yyerror("%N undefined (cannot refer to unexported field or method %S)", n, n->right->sym);
			else
				yyerror("%N undefined (type %T has no field or method %S)", n, n->left->type, n->right->sym);
			goto error;
		}
		switch(n->op) {
		case ODOTINTER:
		case ODOTMETH:
			if(top&Ecall)
				ok |= Ecall;
			else {
				typecheckpartialcall(n, r);
				ok |= Erv;
			}
			break;
		default:
			ok |= Erv;
			break;
		}
		goto ret;

	case ODOTTYPE:
		ok |= Erv;
		typecheck(&n->left, Erv);
		defaultlit(&n->left, T);
		l = n->left;
		if((t = l->type) == T)
			goto error;
		if(!isinter(t)) {
			yyerror("invalid type assertion: %N (non-interface type %T on left)", n, t);
			goto error;
		}
		if(n->right != N) {
			typecheck(&n->right, Etype);
			n->type = n->right->type;
			n->right = N;
			if(n->type == T)
				goto error;
		}
		if(n->type != T && n->type->etype != TINTER)
		if(!implements(n->type, t, &missing, &have, &ptr)) {
			if(have && have->sym == missing->sym)
				yyerror("impossible type assertion:\n\t%T does not implement %T (wrong type for %S method)\n"
					"\t\thave %S%hhT\n\t\twant %S%hhT", n->type, t, missing->sym,
					have->sym, have->type, missing->sym, missing->type);
			else if(ptr)
				yyerror("impossible type assertion:\n\t%T does not implement %T (%S method has pointer receiver)",
					n->type, t, missing->sym);
			else if(have)
				yyerror("impossible type assertion:\n\t%T does not implement %T (missing %S method)\n"
					"\t\thave %S%hhT\n\t\twant %S%hhT", n->type, t, missing->sym,
					have->sym, have->type, missing->sym, missing->type);
			else
				yyerror("impossible type assertion:\n\t%T does not implement %T (missing %S method)",
					n->type, t, missing->sym);
			goto error;
		}
		goto ret;

	case OINDEX:
		ok |= Erv;
		typecheck(&n->left, Erv);
		defaultlit(&n->left, T);
		implicitstar(&n->left);
		l = n->left;
		typecheck(&n->right, Erv);
		r = n->right;
		if((t = l->type) == T || r->type == T)
			goto error;
		switch(t->etype) {
		default:
			yyerror("invalid operation: %N (index of type %T)", n, t);
			goto error;


		case TSTRING:
		case TARRAY:
			indexlit(&n->right);
			if(t->etype == TSTRING) {
				n->type = types[TUINT8];
			}
			else {
				n->type = t->type;
			}
			why = "string";
			if(t->etype == TARRAY) {
				if(isfixedarray(t)) {
					why = "array";
				}
				else {
					why = "slice";
				}
			}
			// 数组/切片/字符串的索引, 必须是整型数值, 否则报错
			if(n->right->type != T && !isint[n->right->type->etype]) {
				yyerror("non-integer %s index %N", why, n->right);
				break;
			}
			// 如果索引值是整型数值, 那么在编译期就可以确认其合法性
			// (如果是变量就不行了, 比如 array[i]).
			if(isconst(n->right, CTINT)) {
				if(mpgetfix(n->right->val.u.xval) < 0) {
					// 数组/切片/字符串的索引, 必须大于等于0(不可为负值), 否则报错
					yyerror("invalid %s index %N (index must be non-negative)", why, n->right);
				}
				else if(isfixedarray(t) && t->bound > 0 && mpgetfix(n->right->val.u.xval) >= t->bound) {
					// 数组/切片/字符串的索引, 访问越界.
					yyerror(
						"invalid array index %N (out of bounds for %d-element array)", 
						n->right, t->bound
					);
				}
				else if(isconst(n->left, CTSTR) && mpgetfix(n->right->val.u.xval) >= n->left->val.u.sval->len) {
					yyerror(
						"invalid string index %N (out of bounds for %d-byte string)", 
						n->right, n->left->val.u.sval->len
					);
				}
				else if(mpcmpfixfix(n->right->val.u.xval, maxintval[TINT]) > 0) {
					yyerror("invalid %s index %N (index too large)", why, n->right);
				}
			}
			break;

		case TMAP:
			n->etype = 0;
			defaultlit(&n->right, t->down);
			if(n->right->type != T) {
				n->right = assignconv(n->right, t->down, "map index");
			}
			n->type = t->type;
			n->op = OINDEXMAP;
			break;
		}
		goto ret;

	case ORECV:
		ok |= Etop | Erv;
		typecheck(&n->left, Erv);
		defaultlit(&n->left, T);
		l = n->left;
		if((t = l->type) == T)
			goto error;
		if(t->etype != TCHAN) {
			yyerror("invalid operation: %N (receive from non-chan type %T)", n, t);
			goto error;
		}
		if(!(t->chan & Crecv)) {
			yyerror("invalid operation: %N (receive from send-only type %T)", n, t);
			goto error;
		}
		n->type = t->type;
		goto ret;

	case OSEND:
		ok |= Etop;
		l = typecheck(&n->left, Erv);
		typecheck(&n->right, Erv);
		defaultlit(&n->left, T);
		l = n->left;
		if((t = l->type) == T)
			goto error;
		if(t->etype != TCHAN) {
			yyerror("invalid operation: %N (send to non-chan type %T)", n, t);
			goto error;
		}
		if(!(t->chan & Csend)) {
			yyerror("invalid operation: %N (send to receive-only type %T)", n, t);
			goto error;
		}
		defaultlit(&n->right, t->type);
		r = n->right;
		if(r->type == T)
			goto error;
		r = assignconv(r, l->type->type, "send");
		// TODO: more aggressive
		n->etype = 0;
		n->type = T;
		goto ret;

	case OSLICE:
		ok |= Erv;
		typecheck(&n->left, top);
		typecheck(&n->right->left, Erv);
		typecheck(&n->right->right, Erv);
		defaultlit(&n->left, T);
		indexlit(&n->right->left);
		indexlit(&n->right->right);
		l = n->left;
		if(isfixedarray(l->type)) {
			if(!islvalue(n->left)) {
				yyerror("invalid operation %N (slice of unaddressable value)", n);
				goto error;
			}
			n->left = nod(OADDR, n->left, N);
			n->left->implicit = 1;
			typecheck(&n->left, Erv);
			l = n->left;
		}
		if((t = l->type) == T)
			goto error;
		tp = nil;
		if(istype(t, TSTRING)) {
			n->type = t;
			n->op = OSLICESTR;
		} else if(isptr[t->etype] && isfixedarray(t->type)) {
			tp = t->type;
			n->type = typ(TARRAY);
			n->type->type = tp->type;
			n->type->bound = -1;
			dowidth(n->type);
			n->op = OSLICEARR;
		} else if(isslice(t)) {
			n->type = t;
		} else {
			yyerror("cannot slice %N (type %T)", l, t);
			goto error;
		}
		if((lo = n->right->left) != N && checksliceindex(lo, tp) < 0)
			goto error;
		if((hi = n->right->right) != N && checksliceindex(hi, tp) < 0)
			goto error;
		if(checksliceconst(lo, hi) < 0)
			goto error;
		goto ret;

	case OSLICE3:
		ok |= Erv;
		typecheck(&n->left, top);
		typecheck(&n->right->left, Erv);
		typecheck(&n->right->right->left, Erv);
		typecheck(&n->right->right->right, Erv);
		defaultlit(&n->left, T);
		indexlit(&n->right->left);
		indexlit(&n->right->right->left);
		indexlit(&n->right->right->right);
		l = n->left;
		if(isfixedarray(l->type)) {
			if(!islvalue(n->left)) {
				yyerror("invalid operation %N (slice of unaddressable value)", n);
				goto error;
			}
			n->left = nod(OADDR, n->left, N);
			n->left->implicit = 1;
			typecheck(&n->left, Erv);
			l = n->left;
		}
		if((t = l->type) == T)
			goto error;
		tp = nil;
		if(istype(t, TSTRING)) {
			yyerror("invalid operation %N (3-index slice of string)", n);
			goto error;
		}
		if(isptr[t->etype] && isfixedarray(t->type)) {
			tp = t->type;
			n->type = typ(TARRAY);
			n->type->type = tp->type;
			n->type->bound = -1;
			dowidth(n->type);
			n->op = OSLICE3ARR;
		} else if(isslice(t)) {
			n->type = t;
		} else {
			yyerror("cannot slice %N (type %T)", l, t);
			goto error;
		}
		if((lo = n->right->left) != N && checksliceindex(lo, tp) < 0)
			goto error;
		if((mid = n->right->right->left) != N && checksliceindex(mid, tp) < 0)
			goto error;
		if((hi = n->right->right->right) != N && checksliceindex(hi, tp) < 0)
			goto error;
		if(checksliceconst(lo, hi) < 0 || checksliceconst(lo, mid) < 0 || checksliceconst(mid, hi) < 0)
			goto error;
		goto ret;

	/*
	 * call and call like
	 */
	case OCALL:
		l = n->left;
		// 首先判断是否为 unsafe 包中的 Sizeof, Alignof, Offsetof 函数.
		if(l->op == ONAME && (r = unsafenmagic(n)) != N) {
			if(n->isddd) {
				yyerror("invalid use of ... with builtin %N", l);
			}
			n = r;
			goto reswitch;
		}
		typecheck(&n->left, Erv | Etype | Ecall |(top&Eproc));
		l = n->left;
		if(l->op == ONAME && l->etype != 0) {
			if(n->isddd && l->etype != OAPPEND)
				yyerror("invalid use of ... with builtin %N", l);
			// builtin: OLEN, OCAP, etc.
			n->op = l->etype;
			n->left = n->right;
			n->right = N;
			goto reswitch;
		}
		defaultlit(&n->left, T);
		l = n->left;
		if(l->op == OTYPE) {
			if(n->isddd || l->type->bound == -100) {
				if(!l->type->broke)
					yyerror("invalid use of ... in type conversion", l);
				n->diag = 1;
			}
			// pick off before type-checking arguments
			ok |= Erv;
			// turn CALL(type, arg) into CONV(arg) w/ type
			n->left = N;
			n->op = OCONV;
			n->type = l->type;
			if(onearg(n, "conversion to %T", l->type) < 0)
				goto error;
			goto doconv;
		}

		if(count(n->list) == 1 && !n->isddd)
			typecheck(&n->list->n, Erv | Efnstruct);
		else
			typechecklist(n->list, Erv);
		if((t = l->type) == T)
			goto error;
		checkwidth(t);

		switch(l->op) {
		case ODOTINTER:
			n->op = OCALLINTER;
			break;

		case ODOTMETH:
			n->op = OCALLMETH;
			// typecheckaste was used here but there wasn't enough
			// information further down the call chain to know if we
			// were testing a method receiver for unexported fields.
			// It isn't necessary, so just do a sanity check.
			tp = getthisx(t)->type->type;
			if(l->left == N || !eqtype(l->left->type, tp))
				fatal("method receiver");
			break;

		default:
			n->op = OCALLFUNC;
			if(t->etype != TFUNC) {
				yyerror("cannot call non-function %N (type %T)", l, t);
				goto error;
			}
			break;
		}
		typecheckaste(OCALL, n->left, n->isddd, getinargx(t), n->list, "function argument");
		ok |= Etop;
		if(t->outtuple == 0)
			goto ret;
		ok |= Erv;
		if(t->outtuple == 1) {
			t = getoutargx(l->type)->type;
			if(t == T)
				goto error;
			if(t->etype == TFIELD)
				t = t->type;
			n->type = t;
			goto ret;
		}
		// multiple return
		if(!(top & (Efnstruct | Etop))) {
			yyerror("multiple-value %N() in single-value context", l);
			goto ret;
		}
		n->type = getoutargx(l->type);
		goto ret;

	case OCAP:
	case OLEN:
	case OREAL:
	case OIMAG:
		ok |= Erv;
		if(onearg(n, "%O", n->op) < 0)
			goto error;
		typecheck(&n->left, Erv);
		defaultlit(&n->left, T);
		implicitstar(&n->left);
		l = n->left;
		t = l->type;
		if(t == T)
			goto error;
		switch(n->op) {
		case OCAP:
			if(!okforcap[t->etype])
				goto badcall1;
			break;
		case OLEN:
			if(!okforlen[t->etype])
				goto badcall1;
			break;
		case OREAL:
		case OIMAG:
			if(!iscomplex[t->etype])
				goto badcall1;
			if(isconst(l, CTCPLX)){
				r = n;
				if(n->op == OREAL)
					n = nodfltconst(&l->val.u.cval->real);
				else
					n = nodfltconst(&l->val.u.cval->imag);
				n->orig = r;
			}
			n->type = types[cplxsubtype(t->etype)];
			goto ret;
		}
		// might be constant
		switch(t->etype) {
		case TSTRING:
			if(isconst(l, CTSTR)) {
				r = nod(OXXX, N, N);
				nodconst(r, types[TINT], l->val.u.sval->len);
				r->orig = n;
				n = r;
			}
			break;
		case TARRAY:
			if(t->bound < 0) // slice
				break;
			if(callrecv(l)) // has call or receive
				break;
			r = nod(OXXX, N, N);
			nodconst(r, types[TINT], t->bound);
			r->orig = n;
			n = r;
			break;
		}
		n->type = types[TINT];
		goto ret;

	case OCOMPLEX:
		ok |= Erv;
		if(twoarg(n) < 0)
			goto error;
		l = typecheck(&n->left, Erv | (top & Eiota));
		r = typecheck(&n->right, Erv | (top & Eiota));
		if(l->type == T || r->type == T)
			goto error;
		defaultlit2(&l, &r, 0);
		if(l->type == T || r->type == T)
			goto error;
		n->left = l;
		n->right = r;
		if(!eqtype(l->type, r->type)) {
			yyerror("invalid operation: %N (mismatched types %T and %T)", n, l->type, r->type);
			goto error;
		}
		switch(l->type->etype) {
		default:
			yyerror("invalid operation: %N (arguments have type %T, expected floating-point)", n, l->type, r->type);
			goto error;
		case TIDEAL:
			t = types[TIDEAL];
			break;
		case TFLOAT32:
			t = types[TCOMPLEX64];
			break;
		case TFLOAT64:
			t = types[TCOMPLEX128];
			break;
		}
		if(l->op == OLITERAL && r->op == OLITERAL) {
			// make it a complex literal
			r = nodcplxlit(l->val, r->val);
			r->orig = n;
			n = r;
		}
		n->type = t;
		goto ret;

	case OCLOSE:
		if(onearg(n, "%O", n->op) < 0)
			goto error;
		typecheck(&n->left, Erv);
		defaultlit(&n->left, T);
		l = n->left;
		if((t = l->type) == T)
			goto error;
		if(t->etype != TCHAN) {
			yyerror("invalid operation: %N (non-chan type %T)", n, t);
			goto error;
		}
		if(!(t->chan & Csend)) {
			yyerror("invalid operation: %N (cannot close receive-only channel)", n);
			goto error;
		}
		ok |= Etop;
		goto ret;

	case ODELETE:
		args = n->list;
		if(args == nil) {
			yyerror("missing arguments to delete");
			goto error;
		}
		if(args->next == nil) {
			yyerror("missing second (key) argument to delete");
			goto error;
		}
		if(args->next->next != nil) {
			yyerror("too many arguments to delete");
			goto error;
		}
		ok |= Etop;
		typechecklist(args, Erv);
		l = args->n;
		r = args->next->n;
		if(l->type != T && l->type->etype != TMAP) {
			yyerror("first argument to delete must be map; have %lT", l->type);
			goto error;
		}
		args->next->n = assignconv(r, l->type->down, "delete");
		goto ret;

	case OAPPEND:
		ok |= Erv;
		args = n->list;
		if(args == nil) {
			yyerror("missing arguments to append");
			goto error;
		}
		typechecklist(args, Erv);
		if((t = args->n->type) == T)
			goto error;
		n->type = t;
		if(!isslice(t)) {
			if(isconst(args->n, CTNIL)) {
				yyerror("first argument to append must be typed slice; have untyped nil", t);
				goto error;
			}
			yyerror("first argument to append must be slice; have %lT", t);
			goto error;
		}

		if(n->isddd) {
			if(args->next == nil) {
				yyerror("cannot use ... on first argument to append");
				goto error;
			}
			if(args->next->next != nil) {
				yyerror("too many arguments to append");
				goto error;
			}
			if(istype(t->type, TUINT8) && istype(args->next->n->type, TSTRING)) {
				defaultlit(&args->next->n, types[TSTRING]);
				goto ret;
			}
			args->next->n = assignconv(args->next->n, t->orig, "append");
			goto ret;
		}
		for(args=args->next; args != nil; args=args->next) {
			if(args->n->type == T)
				continue;
			args->n = assignconv(args->n, t->type, "append");
		}
		goto ret;

	case OCOPY:
		ok |= Etop|Erv;
		args = n->list;
		if(args == nil || args->next == nil) {
			yyerror("missing arguments to copy");
			goto error;
		}
		if(args->next->next != nil) {
			yyerror("too many arguments to copy");
			goto error;
		}
		n->left = args->n;
		n->right = args->next->n;
		n->list = nil;
		n->type = types[TINT];
		typecheck(&n->left, Erv);
		typecheck(&n->right, Erv);
		if(n->left->type == T || n->right->type == T)
			goto error;
		defaultlit(&n->left, T);
		defaultlit(&n->right, T);

		// copy([]byte, string)
		if(isslice(n->left->type) && n->right->type->etype == TSTRING) {
			if(eqtype(n->left->type->type, bytetype))
				goto ret;
			yyerror("arguments to copy have different element types: %lT and string", n->left->type);
			goto error;
		}

		if(!isslice(n->left->type) || !isslice(n->right->type)) {
			if(!isslice(n->left->type) && !isslice(n->right->type))
				yyerror("arguments to copy must be slices; have %lT, %lT", n->left->type, n->right->type);
			else if(!isslice(n->left->type))
				yyerror("first argument to copy should be slice; have %lT", n->left->type);
			else
				yyerror("second argument to copy should be slice or string; have %lT", n->right->type);
			goto error;
		}
		if(!eqtype(n->left->type->type, n->right->type->type)) {
			yyerror("arguments to copy have different element types: %lT and %lT", n->left->type, n->right->type);
			goto error;
		}
		goto ret;

	case OCONV:
	doconv:
		ok |= Erv;
		saveorignode(n);
		typecheck(&n->left, Erv | (top & (Eindir | Eiota)));
		convlit1(&n->left, n->type, 1);
		if((t = n->left->type) == T || n->type == T)
			goto error;
		if((n->op = convertop(t, n->type, &why)) == 0) {
			if(!n->diag && !n->type->broke) {
				yyerror("cannot convert %lN to type %T%s", n->left, n->type, why);
				n->diag = 1;
			}
			n->op = OCONV;
		}
		switch(n->op) {
		case OCONVNOP:
			if(n->left->op == OLITERAL) {
				r = nod(OXXX, N, N);
				n->op = OCONV;
				n->orig = r;
				*r = *n;
				n->op = OLITERAL;
				n->val = n->left->val;
			}
			break;
		case OSTRARRAYBYTE:
		case OSTRARRAYRUNE:
			if(n->left->op == OLITERAL)
				stringtoarraylit(&n);
			break;
		}
		goto ret;
	// make() 语句, 可以是 make slice, map 和 channel, 3种类型.
	// 在做类型检查的时候, 需要将其继续细分为 OMAKESLICE, OMAKEMAP, OMAKECHAN.
	case OMAKE:
		ok |= Erv;
		args = n->list;
		if(args == nil) {
			yyerror("missing argument to make");
			goto error;
		}
		n->list = nil;
		// make()的第1个参数`t`中, 包含目标类型信息, slice, map 或是 channel.
		l = args->n;
		args = args->next;
		typecheck(&l, Etype);
		if((t = l->type) == T) {
			goto error;
		}

		// 根据 t->etype, 判断要 make 的究竟是 slice, map 还是 channel 类型.
		switch(t->etype) {
		default:
		badmake:
			yyerror("cannot make type %T", t);
			goto error;

		case TARRAY:
			if(!isslice(t)) {
				goto badmake;
			}
			// make([]int), 没有 len 和 cap 参数.
			if(args == nil) {
				yyerror("missing len argument to make(%T)", t);
				goto error;
			}
			// len 参数
			l = args->n;
			args = args->next;
			typecheck(&l, Erv);
			r = N;
			// 如果还有 cap 参数
			if(args != nil) {
				r = args->n;
				args = args->next;
				typecheck(&r, Erv);
			}
			if(l->type == T || (r && r->type == T)) {
				goto error;
			}
			et = checkmake(t, "len", l) < 0;
			et |= r && checkmake(t, "cap", r) < 0;
			if(et) {
				goto error;
			}
			// 1. len 与 cap 需要是常量
			// 2. cap 需要大于等于 len
			if(isconst(l, CTINT) && r && isconst(r, CTINT) && mpcmpfixfix(l->val.u.xval, r->val.u.xval) > 0) {
				yyerror("len larger than cap in make(%T)", t);
				goto error;
			}
			n->left = l;
			n->right = r;
			n->op = OMAKESLICE;
			break;

		case TMAP:
			// make(map[xxx]xxx) 可以不写参数
			if(args != nil) {
				l = args->n;
				args = args->next;
				typecheck(&l, Erv);
				defaultlit(&l, types[TINT]);
				if(l->type == T)
					goto error;
				if(checkmake(t, "size", l) < 0)
					goto error;
				n->left = l;
			} else {
				n->left = nodintconst(0);
			}
			n->op = OMAKEMAP;
			break;

		case TCHAN:
			l = N;
			if(args != nil) {
				l = args->n;
				args = args->next;
				typecheck(&l, Erv);
				defaultlit(&l, types[TINT]);
				if(l->type == T)
					goto error;
				if(checkmake(t, "buffer", l) < 0)
					goto error;
				n->left = l;
			} else {
				n->left = nodintconst(0);
			}
			n->op = OMAKECHAN;
			break;
		} // switch{} 结束
		if(args != nil) {
			yyerror("too many arguments to make(%T)", t);
			n->op = OMAKE;
			goto error;
		}
		n->type = t;
		goto ret;

	case ONEW:
		ok |= Erv;
		args = n->list;
		if(args == nil) {
			yyerror("missing argument to new");
			goto error;
		}
		l = args->n;
		typecheck(&l, Etype);
		if((t = l->type) == T)
			goto error;
		if(args->next != nil) {
			yyerror("too many arguments to new(%T)", t);
			goto error;
		}
		n->left = l;
		n->type = ptrto(t);
		goto ret;

	case OPRINT:
	case OPRINTN:
		ok |= Etop;
		typechecklist(n->list, Erv | Eindir);  // Eindir: address does not escape
		for(args=n->list; args; args=args->next) {
			// Special case for print: int constant is int64, not int.
			if(isconst(args->n, CTINT))
				defaultlit(&args->n, types[TINT64]);
			else
				defaultlit(&args->n, T);
		}
		goto ret;

	case OPANIC:
		ok |= Etop;
		if(onearg(n, "panic") < 0)
			goto error;
		typecheck(&n->left, Erv);
		defaultlit(&n->left, types[TINTER]);
		if(n->left->type == T)
			goto error;
		goto ret;
	
	case ORECOVER:
		ok |= Erv|Etop;
		if(n->list != nil) {
			yyerror("too many arguments to recover");
			goto error;
		}
		n->type = types[TINTER];
		goto ret;

	case OCLOSURE:
		ok |= Erv;
		typecheckclosure(n, top);
		if(n->type == T)
			goto error;
		goto ret;
	
	case OITAB:
		ok |= Erv;
		typecheck(&n->left, Erv);
		if((t = n->left->type) == T)
			goto error;
		if(t->etype != TINTER)
			fatal("OITAB of %T", t);
		n->type = ptrto(types[TUINTPTR]);
		goto ret;

	case OSPTR:
		ok |= Erv;
		typecheck(&n->left, Erv);
		if((t = n->left->type) == T)
			goto error;
		if(!isslice(t) && t->etype != TSTRING)
			fatal("OSPTR of %T", t);
		if(t->etype == TSTRING)
			n->type = ptrto(types[TUINT8]);
		else
			n->type = ptrto(t->type);
		goto ret;

	case OCLOSUREVAR:
		ok |= Erv;
		goto ret;
	
	case OCFUNC:
		ok |= Erv;
		typecheck(&n->left, Erv);
		n->type = types[TUINTPTR];
		goto ret;

	case OCONVNOP:
		ok |= Erv;
		typecheck(&n->left, Erv);
		goto ret;

	/*
	 * statements
	 */
	case OAS:
		ok |= Etop;
		typecheckas(n);
		goto ret;

	case OAS2:
		ok |= Etop;
		typecheckas2(n);
		goto ret;

	case OBREAK:
	case OCONTINUE:
	case ODCL:
	case OEMPTY:
	case OGOTO:
	case OLABEL:
	case OXFALL:
		ok |= Etop;
		goto ret;

	case ODEFER:
		ok |= Etop;
		typecheck(&n->left, Etop|Erv);
		if(!n->left->diag)
			checkdefergo(n);
		goto ret;

	case OPROC:
		ok |= Etop;
		typecheck(&n->left, Etop|Eproc|Erv);
		checkdefergo(n);
		goto ret;

	case OFOR:
		ok |= Etop;
		typechecklist(n->ninit, Etop);
		typecheck(&n->ntest, Erv);
		if(n->ntest != N && (t = n->ntest->type) != T && t->etype != TBOOL)
			yyerror("non-bool %lN used as for condition", n->ntest);
		typecheck(&n->nincr, Etop);
		typechecklist(n->nbody, Etop);
		goto ret;

	case OIF:
		ok |= Etop;
		typechecklist(n->ninit, Etop);
		typecheck(&n->ntest, Erv);
		if(n->ntest != N && (t = n->ntest->type) != T && t->etype != TBOOL)
			yyerror("non-bool %lN used as if condition", n->ntest);
		typechecklist(n->nbody, Etop);
		typechecklist(n->nelse, Etop);
		goto ret;

	case ORETURN:
		ok |= Etop;
		if(count(n->list) == 1)
			typechecklist(n->list, Erv | Efnstruct);
		else
			typechecklist(n->list, Erv);
		if(curfn == N) {
			yyerror("return outside function");
			goto error;
		}
		if(curfn->type->outnamed && n->list == nil)
			goto ret;
		typecheckaste(ORETURN, nil, 0, getoutargx(curfn->type), n->list, "return argument");
		goto ret;
	
	case ORETJMP:
		ok |= Etop;
		goto ret;

	case OSELECT:
		ok |= Etop;
		typecheckselect(n);
		goto ret;

	case OSWITCH:
		ok |= Etop;
		typecheckswitch(n);
		goto ret;

	case ORANGE:
		ok |= Etop;
		typecheckrange(n);
		goto ret;

	case OTYPESW:
		yyerror("use of .(type) outside type switch");
		goto error;

	case OXCASE:
		ok |= Etop;
		typechecklist(n->list, Erv);
		typechecklist(n->nbody, Etop);
		goto ret;

	case ODCLFUNC:
		ok |= Etop;
		typecheckfunc(n);
		goto ret;

	case ODCLCONST:
		ok |= Etop;
		typecheck(&n->left, Erv);
		goto ret;

	case ODCLTYPE:
		ok |= Etop;
		typecheck(&n->left, Etype);
		if(!incannedimport)
			checkwidth(n->left->type);
		goto ret;
	}

ret:
	t = n->type;
	if(t && !t->funarg && n->op != OTYPE) {
		switch(t->etype) {
		case TFUNC:	// might have TANY; wait until its called
		case TANY:
		case TFORW:
		case TIDEAL:
		case TNIL:
		case TBLANK:
			break;
		default:
			checkwidth(t);
		}
	}

	if(safemode && !incannedimport && !importpkg && !compiling_wrappers && t && t->etype == TUNSAFEPTR)
		yyerror("cannot use unsafe.Pointer");

	evconst(n);
	if(n->op == OTYPE && !(top & Etype)) {
		yyerror("type %T is not an expression", n->type);
		goto error;
	}
	if((top & (Erv|Etype)) == Etype && n->op != OTYPE) {
		yyerror("%N is not a type", n);
		goto error;
	}
	// TODO(rsc): simplify
	if((top & (Ecall|Erv|Etype)) && !(top & Etop) && !(ok & (Erv|Etype|Ecall))) {
		yyerror("%N used as value", n);
		goto error;
	}
	if((top & Etop) && !(top & (Ecall|Erv|Etype)) && !(ok & Etop)) {
		if(n->diag == 0) {
			yyerror("%N evaluated but not used", n);
			n->diag = 1;
		}
		goto error;
	}

	/* TODO
	if(n->type == T)
		fatal("typecheck nil type");
	*/
	goto out;

badcall1:
	yyerror("invalid argument %lN for %O", n->left, n->op);
	goto error;

error:
	n->type = T;

out:
	*np = n;
} // typecheck1() 结束
