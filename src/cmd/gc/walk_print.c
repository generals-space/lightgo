#include	"walk.h"

// caller:
// 	1. walkstmt()
// 	2. walkexpr()
//
// generate code for print
Node* walkprint(Node *nn, NodeList **init, int defer)
{
	Node *r;
	Node *n;
	NodeList *l, *all;
	Node *on;
	Type *t;
	int notfirst, et, op;
	NodeList *calls, *intypes, *args;
	Fmt fmt;

	on = nil;
	op = nn->op;
	all = nn->list;
	calls = nil;
	notfirst = 0;
	intypes = nil;
	args = nil;

	memset(&fmt, 0, sizeof fmt);
	if(defer) {
		// defer print turns into defer printf with format string
		fmtstrinit(&fmt);
		intypes = list(intypes, nod(ODCLFIELD, N, typenod(types[TSTRING])));
		args = list1(nod(OXXX, N, N));
	}

	for(l=all; l; l=l->next) {
		if(notfirst) {
			if(defer)
				fmtprint(&fmt, " ");
			else
				calls = list(calls, mkcall("printsp", T, init));
		}
		notfirst = op == OPRINTN;

		n = l->n;
		if(n->op == OLITERAL) {
			switch(n->val.ctype) {
			case CTRUNE:
				defaultlit(&n, runetype);
				break;
			case CTINT:
				defaultlit(&n, types[TINT64]);
				break;
			case CTFLT:
				defaultlit(&n, types[TFLOAT64]);
				break;
			}
		}
		if(n->op != OLITERAL && n->type && n->type->etype == TIDEAL)
			defaultlit(&n, types[TINT64]);
		defaultlit(&n, nil);
		l->n = n;
		if(n->type == T || n->type->etype == TFORW)
			continue;

		t = n->type;
		et = n->type->etype;
		if(isinter(n->type)) {
			if(defer) {
				if(isnilinter(n->type))
					fmtprint(&fmt, "%%e");
				else
					fmtprint(&fmt, "%%i");
			} else {
				if(isnilinter(n->type))
					on = syslook("printeface", 1);
				else
					on = syslook("printiface", 1);
				argtype(on, n->type);		// any-1
			}
		} else if(isptr[et] || et == TCHAN || et == TMAP || et == TFUNC || et == TUNSAFEPTR) {
			if(defer) {
				fmtprint(&fmt, "%%p");
			} else {
				on = syslook("printpointer", 1);
				argtype(on, n->type);	// any-1
			}
		} else if(isslice(n->type)) {
			if(defer) {
				fmtprint(&fmt, "%%a");
			} else {
				on = syslook("printslice", 1);
				argtype(on, n->type);	// any-1
			}
		} else if(isint[et]) {
			if(defer) {
				if(et == TUINT64)
					fmtprint(&fmt, "%%U");
				else {
					fmtprint(&fmt, "%%D");
					t = types[TINT64];
				}
			} else {
				if(et == TUINT64)
					on = syslook("printuint", 0);
				else
					on = syslook("printint", 0);
			}
		} else if(isfloat[et]) {
			if(defer) {
				fmtprint(&fmt, "%%f");
				t = types[TFLOAT64];
			} else
				on = syslook("printfloat", 0);
		} else if(iscomplex[et]) {
			if(defer) {
				fmtprint(&fmt, "%%C");
				t = types[TCOMPLEX128];
			} else
				on = syslook("printcomplex", 0);
		} else if(et == TBOOL) {
			if(defer)
				fmtprint(&fmt, "%%t");
			else
				on = syslook("printbool", 0);
		} else if(et == TSTRING) {
			if(defer)
				fmtprint(&fmt, "%%S");
			else
				on = syslook("printstring", 0);
		} else {
			badtype(OPRINT, n->type, T);
			continue;
		}

		if(!defer) {
			t = *getinarg(on->type);
			if(t != nil)
				t = t->type;
			if(t != nil)
				t = t->type;
		}

		if(!eqtype(t, n->type)) {
			n = nod(OCONV, n, N);
			n->type = t;
		}

		if(defer) {
			intypes = list(intypes, nod(ODCLFIELD, N, typenod(t)));
			args = list(args, n);
		} else {
			r = nod(OCALL, on, N);
			r->list = list1(n);
			calls = list(calls, r);
		}
	}

	if(defer) {
		if(op == OPRINTN)
			fmtprint(&fmt, "\n");
		on = syslook("goprintf", 1);
		on->type = functype(nil, intypes, nil);
		args->n = nod(OLITERAL, N, N);
		args->n->val.ctype = CTSTR;
		args->n->val.u.sval = strlit(fmtstrflush(&fmt));
		r = nod(OCALL, on, N);
		r->list = args;
		typecheck(&r, Etop);
		walkexpr(&r, init);
	} else {
		if(op == OPRINTN)
			calls = list(calls, mkcall("printnl", T, nil));
		typechecklist(calls, Etop);
		walkexprlist(calls, init);

		r = nod(OEMPTY, N, N);
		typecheck(&r, Etop);
		walkexpr(&r, init);
		r->ninit = calls;
	}
	return r;
}
