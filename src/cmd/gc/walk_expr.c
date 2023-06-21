#include	"walk.h"

static	Mpint	mpzero;

void walkexpr(Node **np, NodeList **init)
{
	Node *r, *l, *var, *a;
	NodeList *ll, *lr, *lpost;
	Type *t;
	int et, old_safemode;
	int64 v;
	int32 lno;
	Node *n, *fn, *n1, *n2;
	Sym *sym;
	char buf[100], *p;

	n = *np;

	if(n == N) {
		return;
	}

	if(init == &n->ninit) {
		// not okay to use n->ninit when walking n,
		// because we might replace n with some other node
		// and would lose the init list.
		fatal("walkexpr init == &n->ninit");
	}

	if(n->ninit != nil) {
		walkstmtlist(n->ninit);
		*init = concat(*init, n->ninit);
		n->ninit = nil;
	}

	// annoying case - not typechecked
	if(n->op == OKEY) {
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		return;
	}

	lno = setlineno(n);

	if(debug['w'] > 1)
		dump("walk-before", n);

	if(n->typecheck != 1)
		fatal("missed typecheck: %+N\n", n);

	switch(n->op) {
	default:
		dump("walk", n);
		fatal("walkexpr: switch 1 unknown op %+hN", n);
		break;

	case OTYPE:
	case ONONAME:
	case OINDREG:
	case OEMPTY:
		goto ret;

	case ONOT:
	case OMINUS:
	case OPLUS:
	case OCOM:
	case OREAL:
	case OIMAG:
	case ODOTMETH:
	case ODOTINTER:
		walkexpr(&n->left, init);
		goto ret;

	case OIND:
		walkexpr(&n->left, init);
		goto ret;

	case ODOT:
		usefield(n);
		walkexpr(&n->left, init);
		goto ret;

	case ODOTPTR:
		usefield(n);
		if(n->op == ODOTPTR && n->left->type->type->width == 0) {
			// No actual copy will be generated, so emit an explicit nil check.
			n->left = cheapexpr(n->left, init);
			checknil(n->left, init);
		}
		walkexpr(&n->left, init);
		goto ret;

	case OEFACE:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		goto ret;

	case OSPTR:
	case OITAB:
		walkexpr(&n->left, init);
		goto ret;

	case OLEN:
	case OCAP:
		walkexpr(&n->left, init);

		// replace len(*[10]int) with 10.
		// delayed until now to preserve side effects.
		t = n->left->type;
		if(isptr[t->etype])
			t = t->type;
		if(isfixedarray(t)) {
			safeexpr(n->left, init);
			nodconst(n, n->type, t->bound);
			n->typecheck = 1;
		}
		goto ret;

	case OLSH:
	case ORSH:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
	shiftwalked:
		t = n->left->type;
		n->bounded = bounded(n->right, 8*t->width);
		if(debug['m'] && n->etype && !isconst(n->right, CTINT))
			warn("shift bounds check elided");
		goto ret;

	case OAND:
	case OSUB:
	case OHMUL:
	case OLT:
	case OLE:
	case OGE:
	case OGT:
	case OADD:
	case OCOMPLEX:
	case OLROT:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		goto ret;

	case OOR:
	case OXOR:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		walkrotate(&n);
		goto ret;

	case OEQ:
	case ONE:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		// Disable safemode while compiling this code: the code we
		// generate internally can refer to unsafe.Pointer.
		// In this case it can happen if we need to generate an ==
		// for a struct containing a reflect.Value, which itself has
		// an unexported field of type unsafe.Pointer.
		old_safemode = safemode;
		safemode = 0;
		walkcompare(&n, init);
		safemode = old_safemode;
		goto ret;

	case OANDAND:
	case OOROR:
		walkexpr(&n->left, init);
		// cannot put side effects from n->right on init,
		// because they cannot run before n->left is checked.
		// save elsewhere and store on the eventual n->right.
		ll = nil;
		walkexpr(&n->right, &ll);
		addinit(&n->right, ll);
		goto ret;

	case OPRINT:
	case OPRINTN:
		walkexprlist(n->list, init);
		n = walkprint(n, init, 0);
		goto ret;

	case OPANIC:
		n = mkcall("panic", T, init, n->left);
		goto ret;

	case ORECOVER:
		n = mkcall("recover", n->type, init, nod(OADDR, nodfp, N));
		goto ret;

	case OLITERAL:
		n->addable = 1;
		goto ret;

	case OCLOSUREVAR:
	case OCFUNC:
		n->addable = 1;
		goto ret;

	case ONAME:
		if(!(n->class & PHEAP) && n->class != PPARAMREF)
			n->addable = 1;
		goto ret;

	case OCALLINTER:
		t = n->left->type;
		if(n->list && n->list->n->op == OAS)
			goto ret;
		walkexpr(&n->left, init);
		walkexprlist(n->list, init);
		ll = ascompatte(n->op, n, n->isddd, getinarg(t), n->list, 0, init);
		n->list = reorder1(ll);
		goto ret;

	case OCALLFUNC:
		t = n->left->type;
		if(n->list && n->list->n->op == OAS)
			goto ret;

		walkexpr(&n->left, init);
		walkexprlist(n->list, init);

		ll = ascompatte(n->op, n, n->isddd, getinarg(t), n->list, 0, init);
		n->list = reorder1(ll);
		goto ret;

	case OCALLMETH:
		t = n->left->type;
		if(n->list && n->list->n->op == OAS)
			goto ret;
		walkexpr(&n->left, init);
		walkexprlist(n->list, init);
		ll = ascompatte(n->op, n, 0, getthis(t), list1(n->left->left), 0, init);
		lr = ascompatte(n->op, n, n->isddd, getinarg(t), n->list, 0, init);
		ll = concat(ll, lr);
		n->left->left = N;
		ullmancalc(n->left);
		n->list = reorder1(ll);
		goto ret;

	case OAS:
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		walkexpr(&n->left, init);
		n->left = safeexpr(n->left, init);

		if(oaslit(n, init))
			goto ret;

		walkexpr(&n->right, init);
		if(n->left != N && n->right != N) {
			r = convas(nod(OAS, n->left, n->right), init);
			r->dodata = n->dodata;
			n = r;
		}

		goto ret;

	case OAS2:
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		walkexprlistsafe(n->list, init);
		walkexprlistsafe(n->rlist, init);
		ll = ascompatee(OAS, n->list, n->rlist, init);
		ll = reorder3(ll);
		n = liststmt(ll);
		goto ret;

	case OAS2FUNC:
	as2func:
		// a,b,... = fn()
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		r = n->rlist->n;
		walkexprlistsafe(n->list, init);
		walkexpr(&r, init);
		l = n->list->n;

		// all the really hard stuff - explicit function calls and so on -
		// is gone, but map assignments remain.
		// if there are map assignments here, assign via
		// temporaries, because ascompatet assumes
		// the targets can be addressed without function calls
		// and map index has an implicit one.
		lpost = nil;
		if(l->op == OINDEXMAP) {
			var = temp(l->type);
			n->list->n = var;
			a = nod(OAS, l, var);
			typecheck(&a, Etop);
			lpost = list(lpost, a);
		}
		l = n->list->next->n;
		if(l->op == OINDEXMAP) {
			var = temp(l->type);
			n->list->next->n = var;
			a = nod(OAS, l, var);
			typecheck(&a, Etop);
			lpost = list(lpost, a);
		}
		ll = ascompatet(n->op, n->list, &r->type, 0, init);
		walkexprlist(lpost, init);
		n = liststmt(concat(concat(list1(r), ll), lpost));
		goto ret;

	case OAS2RECV:
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		r = n->rlist->n;
		walkexprlistsafe(n->list, init);
		walkexpr(&r->left, init);
		fn = chanfn("chanrecv2", 2, r->left->type);
		r = mkcall1(fn, getoutargx(fn->type), init, typename(r->left->type), r->left);
		n->rlist->n = r;
		n->op = OAS2FUNC;
		goto as2func;

	case OAS2MAPR:
		// a,b = m[i];
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		r = n->rlist->n;
		walkexprlistsafe(n->list, init);
		walkexpr(&r->left, init);
		t = r->left->type;
		p = nil;
		// Check ../../pkg/runtime/hashmap.c:MAXVALUESIZE before changing.
		if(t->type->width <= 128) { 
			switch(simsimtype(t->down)) {
			case TINT32:
			case TUINT32:
				p = "mapaccess2_fast32";
				break;
			case TINT64:
			case TUINT64:
				p = "mapaccess2_fast64";
				break;
			case TSTRING:
				p = "mapaccess2_faststr";
				break;
			}
		}
		if(p != nil) {
			// from:
			//   a,b = m[i]
			// to:
			//   var,b = mapaccess2_fast*(t, m, i)
			//   a = *var
			a = n->list->n;
			var = temp(ptrto(t->type));
			var->typecheck = 1;

			fn = mapfn(p, t);
			r = mkcall1(fn, getoutargx(fn->type), init, typename(t), r->left, r->right);
			n->rlist = list1(r);
			n->op = OAS2FUNC;
			n->list->n = var;
			walkexpr(&n, init);
			*init = list(*init, n);

			n = nod(OAS, a, nod(OIND, var, N));
			typecheck(&n, Etop);
			walkexpr(&n, init);
			goto ret;
		}
		fn = mapfn("mapaccess2", t);
		r = mkcall1(fn, getoutargx(fn->type), init, typename(t), r->left, r->right);
		n->rlist = list1(r);
		n->op = OAS2FUNC;
		goto as2func;

	case ODELETE:
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		l = n->list->n;
		r = n->list->next->n;
		t = l->type;
		n = mkcall1(mapfndel("mapdelete", t), t->down, init, typename(t), l, r);
		goto ret;

	case OAS2DOTTYPE:
		// a,b = i.(T)
		*init = concat(*init, n->ninit);
		n->ninit = nil;
		r = n->rlist->n;
		walkexprlistsafe(n->list, init);
		if(isblank(n->list->n) && !isinter(r->type)) {
			strcpy(buf, "assert");
			p = buf+strlen(buf);
			if(isnilinter(r->left->type))
				*p++ = 'E';
			else
				*p++ = 'I';
			*p++ = '2';
			*p++ = 'T';
			*p++ = 'O';
			*p++ = 'K';
			*p = '\0';
			
			fn = syslook(buf, 1);
			ll = list1(typename(r->type));
			ll = list(ll, r->left);
			argtype(fn, r->left->type);
			n1 = nod(OCALL, fn, N);
			n1->list = ll;
			n = nod(OAS, n->list->next->n, n1);
			typecheck(&n, Etop);
			walkexpr(&n, init);
			goto ret;
		}

		r->op = ODOTTYPE2;
		walkexpr(&r, init);
		ll = ascompatet(n->op, n->list, &r->type, 0, init);
		n = liststmt(concat(list1(r), ll));
		goto ret;

	case ODOTTYPE:
	case ODOTTYPE2:
		// Build name of function: assertI2E2 etc.
		strcpy(buf, "assert");
		p = buf+strlen(buf);
		if(isnilinter(n->left->type))
			*p++ = 'E';
		else
			*p++ = 'I';
		*p++ = '2';
		if(isnilinter(n->type))
			*p++ = 'E';
		else if(isinter(n->type))
			*p++ = 'I';
		else
			*p++ = 'T';
		if(n->op == ODOTTYPE2)
			*p++ = '2';
		*p = '\0';

		fn = syslook(buf, 1);
		ll = list1(typename(n->type));
		ll = list(ll, n->left);
		argtype(fn, n->left->type);
		argtype(fn, n->type);
		n = nod(OCALL, fn, N);
		n->list = ll;
		typecheck(&n, Erv | Efnstruct);
		walkexpr(&n, init);
		goto ret;

	case OCONVIFACE:
		walkexpr(&n->left, init);

		// Optimize convT2E as a two-word copy when T is uintptr-shaped.
		if(!isinter(n->left->type) && isnilinter(n->type) &&
		   (n->left->type->width == widthptr) &&
		   isint[simsimtype(n->left->type)]) {
			l = nod(OEFACE, typename(n->left->type), n->left);
			l->type = n->type;
			l->typecheck = n->typecheck;
			n = l;
			goto ret;
		}

		// Build name of function: convI2E etc.
		// Not all names are possible
		// (e.g., we'll never generate convE2E or convE2I).
		strcpy(buf, "conv");
		p = buf+strlen(buf);
		if(isnilinter(n->left->type))
			*p++ = 'E';
		else if(isinter(n->left->type))
			*p++ = 'I';
		else
			*p++ = 'T';
		*p++ = '2';
		if(isnilinter(n->type))
			*p++ = 'E';
		else
			*p++ = 'I';
		*p = '\0';

		fn = syslook(buf, 1);
		ll = nil;
		if(!isinter(n->left->type))
			ll = list(ll, typename(n->left->type));
		if(!isnilinter(n->type))
			ll = list(ll, typename(n->type));
		if(!isinter(n->left->type) && !isnilinter(n->type)){
			sym = pkglookup(smprint("%-T.%-T", n->left->type, n->type), itabpkg);
			if(sym->def == N) {
				l = nod(ONAME, N, N);
				l->sym = sym;
				l->type = ptrto(types[TUINT8]);
				l->addable = 1;
				l->class = PEXTERN;
				l->xoffset = 0;
				sym->def = l;
				ggloblsym(sym, widthptr, 1, 0);
			}
			l = nod(OADDR, sym->def, N);
			l->addable = 1;
			ll = list(ll, l);

			if(n->left->type->width == widthptr &&
		   	   isint[simsimtype(n->left->type)]) {
				/* For pointer types, we can make a special form of optimization
				 *
				 * These statements are put onto the expression init list:
				 * 	Itab *tab = atomicloadtype(&cache);
				 * 	if(tab == nil)
				 * 		tab = typ2Itab(type, itype, &cache);
				 *
				 * The CONVIFACE expression is replaced with this:
				 * 	OEFACE{tab, ptr};
				 */
				l = temp(ptrto(types[TUINT8]));

				n1 = nod(OAS, l, sym->def);
				typecheck(&n1, Etop);
				*init = list(*init, n1);

				fn = syslook("typ2Itab", 1);
				n1 = nod(OCALL, fn, N);
				n1->list = ll;
				typecheck(&n1, Erv);
				walkexpr(&n1, init);

				n2 = nod(OIF, N, N);
				n2->ntest = nod(OEQ, l, nodnil());
				n2->nbody = list1(nod(OAS, l, n1));
				n2->likely = -1;
				typecheck(&n2, Etop);
				*init = list(*init, n2);

				l = nod(OEFACE, l, n->left);
				l->typecheck = n->typecheck; 
				l->type = n->type;
				n = l;
				goto ret;
			}
		}
		ll = list(ll, n->left);
		argtype(fn, n->left->type);
		argtype(fn, n->type);
		dowidth(fn->type);
		n = nod(OCALL, fn, N);
		n->list = ll;
		typecheck(&n, Erv);
		walkexpr(&n, init);
		goto ret;

	case OCONV:
	case OCONVNOP:
		if(thechar == '5') {
			if(isfloat[n->left->type->etype]) {
				if(n->type->etype == TINT64) {
					n = mkcall("float64toint64", n->type, init, conv(n->left, types[TFLOAT64]));
					goto ret;
				}
				if(n->type->etype == TUINT64) {
					n = mkcall("float64touint64", n->type, init, conv(n->left, types[TFLOAT64]));
					goto ret;
				}
			}
			if(isfloat[n->type->etype]) {
				if(n->left->type->etype == TINT64) {
					n = mkcall("int64tofloat64", n->type, init, conv(n->left, types[TINT64]));
					goto ret;
				}
				if(n->left->type->etype == TUINT64) {
					n = mkcall("uint64tofloat64", n->type, init, conv(n->left, types[TUINT64]));
					goto ret;
				}
			}
		}
		walkexpr(&n->left, init);
		goto ret;

	case OASOP:
		if(n->etype == OANDNOT) {
			n->etype = OAND;
			n->right = nod(OCOM, n->right, N);
			typecheck(&n->right, Erv);
		}
		n->left = safeexpr(n->left, init);
		walkexpr(&n->left, init);
		l = n->left;
		walkexpr(&n->right, init);

		/*
		 * on 32-bit arch, rewrite 64-bit ops into l = l op r.
		 * on 386, rewrite float ops into l = l op r.
		 * everywhere, rewrite map ops into l = l op r.
		 * everywhere, rewrite string += into l = l op r.
		 * everywhere, rewrite integer/complex /= into l = l op r.
		 * TODO(rsc): Maybe this rewrite should be done always?
		 */
		et = n->left->type->etype;
		if((widthptr == 4 && (et == TUINT64 || et == TINT64)) ||
		   (thechar == '8' && isfloat[et]) ||
		   l->op == OINDEXMAP ||
		   et == TSTRING ||
		   (!isfloat[et] && n->etype == ODIV) ||
		   n->etype == OMOD) {
			l = safeexpr(n->left, init);
			a = l;
			if(a->op == OINDEXMAP) {
				// map index has "lhs" bit set in a->etype.
				// make a copy so we can clear it on the rhs.
				a = nod(OXXX, N, N);
				*a = *l;
				a->etype = 0;
			}
			r = nod(OAS, l, nod(n->etype, a, n->right));
			typecheck(&r, Etop);
			walkexpr(&r, init);
			n = r;
			goto ret;
		}
		if(n->etype == OLSH || n->etype == ORSH)
			goto shiftwalked;
		goto ret;

	case OANDNOT:
		walkexpr(&n->left, init);
		n->op = OAND;
		n->right = nod(OCOM, n->right, N);
		typecheck(&n->right, Erv);
		walkexpr(&n->right, init);
		goto ret;

	case OMUL:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		walkmul(&n, init);
		goto ret;

	case ODIV:
	case OMOD:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		/*
		 * rewrite complex div into function call.
		 */
		et = n->left->type->etype;
		if(iscomplex[et] && n->op == ODIV) {
			t = n->type;
			n = mkcall("complex128div", types[TCOMPLEX128], init,
				conv(n->left, types[TCOMPLEX128]),
				conv(n->right, types[TCOMPLEX128]));
			n = conv(n, t);
			goto ret;
		}
		// Nothing to do for float divisions.
		if(isfloat[et])
			goto ret;

		// Try rewriting as shifts or magic multiplies.
		walkdiv(&n, init);

		/*
		 * rewrite 64-bit div and mod into function calls
		 * on 32-bit architectures.
		 */
		switch(n->op) {
		case OMOD:
		case ODIV:
			if(widthptr > 4 || (et != TUINT64 && et != TINT64))
				goto ret;
			if(et == TINT64)
				strcpy(namebuf, "int64");
			else
				strcpy(namebuf, "uint64");
			if(n->op == ODIV)
				strcat(namebuf, "div");
			else
				strcat(namebuf, "mod");
			n = mkcall(namebuf, n->type, init,
				conv(n->left, types[et]), conv(n->right, types[et]));
			break;
		default:
			break;
		}
		goto ret;

	case OINDEX:
		walkexpr(&n->left, init);
		// save the original node for bounds checking elision.
		// If it was a ODIV/OMOD walk might rewrite it.
		r = n->right;
		walkexpr(&n->right, init);

		// if range of type cannot exceed static array bound,
		// disable bounds check.
		if(n->bounded)
			goto ret;
		t = n->left->type;
		if(t != T && isptr[t->etype])
			t = t->type;
		if(isfixedarray(t)) {
			n->bounded = bounded(r, t->bound);
			if(debug['m'] && n->bounded && !isconst(n->right, CTINT))
				warn("index bounds check elided");
			if(smallintconst(n->right) && !n->bounded)
				yyerror("index out of bounds");
		} else if(isconst(n->left, CTSTR)) {
			n->bounded = bounded(r, n->left->val.u.sval->len);
			if(debug['m'] && n->bounded && !isconst(n->right, CTINT))
				warn("index bounds check elided");
			if(smallintconst(n->right)) {
				if(!n->bounded)
					yyerror("index out of bounds");
				else {
					// replace "abc"[1] with 'b'.
					// delayed until now because "abc"[1] is not
					// an ideal constant.
					v = mpgetfix(n->right->val.u.xval);
					nodconst(n, n->type, n->left->val.u.sval->s[v]);
					n->typecheck = 1;
				}
			}
		}

		if(isconst(n->right, CTINT))
		if(mpcmpfixfix(n->right->val.u.xval, &mpzero) < 0 ||
		   mpcmpfixfix(n->right->val.u.xval, maxintval[TINT]) > 0)
			yyerror("index out of bounds");
		goto ret;

	case OINDEXMAP:
		if(n->etype == 1)
			goto ret;

		t = n->left->type;
		p = nil;
		if(t->type->width <= 128) {  // Check ../../pkg/runtime/hashmap.c:MAXVALUESIZE before changing.
			switch(simsimtype(t->down)) {
			case TINT32:
			case TUINT32:
				p = "mapaccess1_fast32";
				break;
			case TINT64:
			case TUINT64:
				p = "mapaccess1_fast64";
				break;
			case TSTRING:
				p = "mapaccess1_faststr";
				break;
			}
		}
		if(p != nil) {
			// use fast version.  The fast versions return a pointer to the value - we need
			// to dereference it to get the result.
			n = mkcall1(mapfn(p, t), ptrto(t->type), init, typename(t), n->left, n->right);
			n = nod(OIND, n, N);
			n->type = t->type;
			n->typecheck = 1;
		} else {
			// no fast version for this key
			n = mkcall1(mapfn("mapaccess1", t), t->type, init, typename(t), n->left, n->right);
		}
		goto ret;

	case ORECV:
		walkexpr(&n->left, init);
		walkexpr(&n->right, init);
		n = mkcall1(chanfn("chanrecv1", 2, n->left->type), n->type, init, typename(n->left->type), n->left);
		goto ret;

	case OSLICE:
		if(n->right != N && n->right->left == N && n->right->right == N) { // noop
			walkexpr(&n->left, init);
			n = n->left;
			goto ret;
		}
		// fallthrough
	case OSLICEARR:
	case OSLICESTR:
		if(n->right == N) // already processed
			goto ret;

		walkexpr(&n->left, init);
		// cgen_slice can't handle string literals as source
		// TODO the OINDEX case is a bug elsewhere that needs to be traced. 
		// it causes a crash on ([2][]int{ ... })[1][lo:hi]
		if((n->op == OSLICESTR && n->left->op == OLITERAL) || (n->left->op == OINDEX))
			n->left = copyexpr(n->left, n->left->type, init);
		else
			n->left = safeexpr(n->left, init);
		walkexpr(&n->right->left, init);
		n->right->left = safeexpr(n->right->left, init);
		walkexpr(&n->right->right, init);
		n->right->right = safeexpr(n->right->right, init);
		n = sliceany(n, init);  // chops n->right, sets n->list
		goto ret;
	
	case OSLICE3:
	case OSLICE3ARR:
		if(n->right == N) // already processed
			goto ret;

		walkexpr(&n->left, init);
		// TODO the OINDEX case is a bug elsewhere that needs to be traced. 
		// it causes a crash on ([2][]int{ ... })[1][lo:hi]
		// TODO the comment on the previous line was copied from case OSLICE.
		// it might not even be true.
		if(n->left->op == OINDEX)
			n->left = copyexpr(n->left, n->left->type, init);
		else
			n->left = safeexpr(n->left, init);
		walkexpr(&n->right->left, init);
		n->right->left = safeexpr(n->right->left, init);
		walkexpr(&n->right->right->left, init);
		n->right->right->left = safeexpr(n->right->right->left, init);
		walkexpr(&n->right->right->right, init);
		n->right->right->right = safeexpr(n->right->right->right, init);
		n = sliceany(n, init);  // chops n->right, sets n->list
		goto ret;

	case OADDR:
		walkexpr(&n->left, init);
		goto ret;

	case ONEW:
		if(n->esc == EscNone && n->type->type->width < (1<<16)) {
			r = temp(n->type->type);
			r = nod(OAS, r, N);  // zero temp
			typecheck(&r, Etop);
			*init = list(*init, r);
			r = nod(OADDR, r->left, N);
			typecheck(&r, Erv);
			n = r;
		} else {
			n = callnew(n->type->type);
		}
		goto ret;

	case OCMPSTR:
		// If one argument to the comparison is an empty string,
		// comparing the lengths instead will yield the same result
		// without the function call.
		if((isconst(n->left, CTSTR) && n->left->val.u.sval->len == 0) ||
		   (isconst(n->right, CTSTR) && n->right->val.u.sval->len == 0)) {
			r = nod(n->etype, nod(OLEN, n->left, N), nod(OLEN, n->right, N));
			typecheck(&r, Erv);
			walkexpr(&r, init);
			r->type = n->type;
			n = r;
			goto ret;
		}

		// s + "badgerbadgerbadger" == "badgerbadgerbadger"
		if((n->etype == OEQ || n->etype == ONE) &&
		   isconst(n->right, CTSTR) &&
		   n->left->op == OADDSTR && isconst(n->left->right, CTSTR) &&
		   cmpslit(n->right, n->left->right) == 0) {
			r = nod(n->etype, nod(OLEN, n->left->left, N), nodintconst(0));
			typecheck(&r, Erv);
			walkexpr(&r, init);
			r->type = n->type;
			n = r;
			goto ret;
		}

		if(n->etype == OEQ || n->etype == ONE) {
			// prepare for rewrite below
			n->left = cheapexpr(n->left, init);
			n->right = cheapexpr(n->right, init);

			r = mkcall("eqstring", types[TBOOL], init,
				conv(n->left, types[TSTRING]),
				conv(n->right, types[TSTRING]));

			// quick check of len before full compare for == or !=
			if(n->etype == OEQ) {
				// len(left) == len(right) && eqstring(left, right)
				r = nod(OANDAND, nod(OEQ, nod(OLEN, n->left, N), nod(OLEN, n->right, N)), r);
			} else {
				// len(left) != len(right) || !eqstring(left, right)
				r = nod(ONOT, r, N);
				r = nod(OOROR, nod(ONE, nod(OLEN, n->left, N), nod(OLEN, n->right, N)), r);
			}
			typecheck(&r, Erv);
			walkexpr(&r, nil);
		} else {
			// sys_cmpstring(s1, s2) :: 0
			r = mkcall("cmpstring", types[TINT], init,
				conv(n->left, types[TSTRING]),
				conv(n->right, types[TSTRING]));
			r = nod(n->etype, r, nodintconst(0));
		}

		typecheck(&r, Erv);
		if(n->type->etype != TBOOL) fatal("cmp %T", n->type);
		r->type = n->type;
		n = r;
		goto ret;

	case OADDSTR:
		n = addstr(n, init);
		goto ret;
	
	case OAPPEND:
		if(n->isddd) {
			// also works for append(slice, string).
			n = appendslice(n, init); 
		}
		else {
			n = append(n, init);
		}
		goto ret;

	case OCOPY:
		if(flag_race) {
			if(n->right->type->etype == TSTRING) {
				fn = syslook("slicestringcopy", 1);
			}
			else {
				fn = syslook("copy", 1);
			}
			argtype(fn, n->left->type);
			argtype(fn, n->right->type);
			n = mkcall1(
				fn, n->type, init, n->left, n->right,
				nodintconst(n->left->type->type->width)
			);
			goto ret;
		}
		n = copyany(n, init);
		goto ret;

	case OCLOSE:
		// cannot use chanfn - closechan takes any, not chan any
		fn = syslook("closechan", 1);
		argtype(fn, n->left->type);
		n = mkcall1(fn, T, init, n->left);
		goto ret;

	case OMAKECHAN:
		n = mkcall1(chanfn("makechan", 1, n->type), n->type, init,
			typename(n->type),
			conv(n->left, types[TINT64]));
		goto ret;

	case OMAKEMAP:
		t = n->type;

		fn = syslook("makemap", 1);
		argtype(fn, t->down);	// any-1
		argtype(fn, t->type);	// any-2

		n = mkcall1(fn, n->type, init,
			typename(n->type),
			conv(n->left, types[TINT64]));
		goto ret;

	case OMAKESLICE:
		l = n->left;
		r = n->right;
		if(r == nil)
			l = r = safeexpr(l, init);
		t = n->type;
		if(n->esc == EscNone
			&& smallintconst(l) && smallintconst(r)
			&& (t->type->width == 0 || mpgetfix(r->val.u.xval) < (1ULL<<16) / t->type->width)) {
			// var arr [r]T
			// n = arr[:l]
			t = aindex(r, t->type); // [r]T
			var = temp(t);
			a = nod(OAS, var, N); // zero temp
			typecheck(&a, Etop);
			*init = list(*init, a);
			r = nod(OSLICE, var, nod(OKEY, N, l)); // arr[:l]
			r = conv(r, n->type); // in case n->type is named.
			typecheck(&r, Erv);
			walkexpr(&r, init);
			n = r;
		} else {
			// makeslice(t *Type, nel int64, max int64) (ary []any)
			fn = syslook("makeslice", 1);
			argtype(fn, t->type);			// any-1
			n = mkcall1(fn, n->type, init,
				typename(n->type),
				conv(l, types[TINT64]),
				conv(r, types[TINT64]));
		}
		goto ret;

	case ORUNESTR:
		// sys_intstring(v)
		n = mkcall("intstring", n->type, init,
			conv(n->left, types[TINT64]));
		goto ret;

	case OARRAYBYTESTR:
		// slicebytetostring([]byte) string;
		n = mkcall("slicebytetostring", n->type, init, n->left);
		goto ret;

	case OARRAYRUNESTR:
		// slicerunetostring([]rune) string;
		n = mkcall("slicerunetostring", n->type, init, n->left);
		goto ret;

	case OSTRARRAYBYTE:
		// stringtoslicebyte(string) []byte;
		n = mkcall("stringtoslicebyte", n->type, init, conv(n->left, types[TSTRING]));
		goto ret;

	case OSTRARRAYRUNE:
		// stringtoslicerune(string) []rune
		n = mkcall("stringtoslicerune", n->type, init, n->left);
		goto ret;

	case OCMPIFACE:
		// ifaceeq(i1 any-1, i2 any-2) (ret bool);
		if(!eqtype(n->left->type, n->right->type))
			fatal("ifaceeq %O %T %T", n->op, n->left->type, n->right->type);
		if(isnilinter(n->left->type))
			fn = syslook("efaceeq", 1);
		else
			fn = syslook("ifaceeq", 1);

		n->right = cheapexpr(n->right, init);
		n->left = cheapexpr(n->left, init);
		argtype(fn, n->right->type);
		argtype(fn, n->left->type);
		r = mkcall1(fn, n->type, init, n->left, n->right);
		if(n->etype == ONE)
			r = nod(ONOT, r, N);
		
		// check itable/type before full compare.
		if(n->etype == OEQ)
			r = nod(OANDAND, nod(OEQ, nod(OITAB, n->left, N), nod(OITAB, n->right, N)), r);
		else
			r = nod(OOROR, nod(ONE, nod(OITAB, n->left, N), nod(OITAB, n->right, N)), r);
		typecheck(&r, Erv);
		walkexpr(&r, init);
		r->type = n->type;
		n = r;
		goto ret;

	case OARRAYLIT:
	case OMAPLIT:
	case OSTRUCTLIT:
	case OPTRLIT:
		var = temp(n->type);
		anylit(0, n, var, init);
		n = var;
		goto ret;

	case OSEND:
		n = mkcall1(chanfn("chansend1", 2, n->left->type), T, init, typename(n->left->type), n->left, n->right);
		goto ret;

	case OCLOSURE:
		n = walkclosure(n, init);
		goto ret;
	
	case OCALLPART:
		n = walkpartialcall(n, init);
		goto ret;
	}
	fatal("missing switch %O", n->op);

ret:
	// Expressions that are constant at run time but not
	// considered const by the language spec are not turned into
	// constants until walk. For example, if n is y%1 == 0, the
	// walk of y%1 may have replaced it by 0.
	// Check whether n with its updated args is itself now a constant.
	t = n->type;
	evconst(n);
	n->type = t;
	if(n->op == OLITERAL)
		typecheck(&n, Erv);

	ullmancalc(n);

	if(debug['w'] && n != N)
		dump("walk", n);

	lineno = lno;
	*np = n;
}
