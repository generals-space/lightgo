#include	"walk.h"

// expand append(l1, l2...) to
//   init {
//     s := l1
//     if n := len(l1) + len(l2) - cap(s); n > 0 {
//       s = growslice(s, n)
//     }
//     s = s[:len(l1)+len(l2)]
//     memmove(&s[len(l1)], &l2[0], len(l2)*sizeof(T))
//   }
//   s
//
// l2 is allowed to be a string.
Node* appendslice(Node *n, NodeList **init)
{
	NodeList *l;
	Node *l1, *l2, *nt, *nif, *fn;
	Node *nptr1, *nptr2, *nwid;
	Node *s;

	walkexprlistsafe(n->list, init);

	// walkexprlistsafe will leave OINDEX (s[n]) alone if both s
	// and n are name or literal, but those may index the slice we're
	// modifying here.  Fix explicitly.
	for(l=n->list; l; l=l->next) {
		l->n = cheapexpr(l->n, init);
	}

	l1 = n->list->n;
	l2 = n->list->next->n;

	s = temp(l1->type); // var s []T
	l = nil;
	l = list(l, nod(OAS, s, l1)); // s = l1

	nt = temp(types[TINT]);
	nif = nod(OIF, N, N);
	// n := len(s) + len(l2) - cap(s)
	nif->ninit = list1(nod(OAS, nt,
		nod(OSUB, nod(OADD, nod(OLEN, s, N), nod(OLEN, l2, N)), nod(OCAP, s, N))));
	nif->ntest = nod(OGT, nt, nodintconst(0));
	// instantiate growslice(Type*, []any, int64) []any
	fn = syslook("growslice", 1);
	argtype(fn, s->type->type);
	argtype(fn, s->type->type);

	// s = growslice(T, s, n)
	nif->nbody = list1(nod(OAS, s, mkcall1(fn, s->type, &nif->ninit,
					       typename(s->type),
					       s,
					       conv(nt, types[TINT64]))));

	l = list(l, nif);

	if(flag_race) {
		// rely on runtime to instrument copy.
		// copy(s[len(l1):len(l1)+len(l2)], l2)
		nptr1 = nod(OSLICE, s, nod(OKEY,
			nod(OLEN, l1, N),
			nod(OADD, nod(OLEN, l1, N), nod(OLEN, l2, N))));
		nptr1->etype = 1;
		nptr2 = l2;
		if(l2->type->etype == TSTRING)
			fn = syslook("slicestringcopy", 1);
		else
			fn = syslook("copy", 1);
		argtype(fn, l1->type);
		argtype(fn, l2->type);
		l = list(l, mkcall1(fn, types[TINT], init,
				nptr1, nptr2,
				nodintconst(s->type->type->width)));
	} else {
		// memmove(&s[len(l1)], &l2[0], len(l2)*sizeof(T))
		nptr1 = nod(OINDEX, s, nod(OLEN, l1, N));
		nptr1->bounded = 1;
		nptr1 = nod(OADDR, nptr1, N);

		nptr2 = nod(OSPTR, l2, N);

		fn = syslook("memmove", 1);
		argtype(fn, s->type->type);	// 1 old []any
		argtype(fn, s->type->type);	// 2 ret []any

		nwid = cheapexpr(conv(nod(OLEN, l2, N), types[TUINTPTR]), &l);
		nwid = nod(OMUL, nwid, nodintconst(s->type->type->width));
		l = list(l, mkcall1(fn, T, init, nptr1, nptr2, nwid));
	}

	// s = s[:len(l1)+len(l2)]
	nt = nod(OADD, nod(OLEN, l1, N), nod(OLEN, l2, N));
	nt = nod(OSLICE, s, nod(OKEY, N, nt));
	nt->etype = 1;
	l = list(l, nod(OAS, s, nt));

	typechecklist(l, Etop);
	walkstmtlist(l);
	*init = concat(*init, l);
	return s;
}

// expand append(src, a [, b]* ) to
//
//   init {
//     s := src
//     const argc = len(args) - 1
//     if cap(s) - len(s) < argc {
//	    s = growslice(s, argc)
//     }
//     n := len(s)
//     s = s[:n+argc]
//     s[n] = a
//     s[n+1] = b
//     ...
//   }
//   s
Node* append(Node *n, NodeList **init)
{
	NodeList *l, *a;
	Node *nsrc, *ns, *nn, *na, *nx, *fn;
	int argc;

	walkexprlistsafe(n->list, init);

	// walkexprlistsafe will leave OINDEX (s[n]) alone if both s
	// and n are name or literal, but those may index the slice we're
	// modifying here.  Fix explicitly.
	for(l=n->list; l; l=l->next) {
		l->n = cheapexpr(l->n, init);
	}

	nsrc = n->list->n;
	argc = count(n->list) - 1;
	if (argc < 1) {
		return nsrc;
	}

	l = nil;

	ns = temp(nsrc->type);
	l = list(l, nod(OAS, ns, nsrc));  // s = src

	na = nodintconst(argc);		// const argc
	nx = nod(OIF, N, N);		// if cap(s) - len(s) < argc
	nx->ntest = nod(OLT, nod(OSUB, nod(OCAP, ns, N), nod(OLEN, ns, N)), na);

	fn = syslook("growslice", 1);	//   growslice(<type>, old []T, n int64) (ret []T)
	argtype(fn, ns->type->type);	// 1 old []any
	argtype(fn, ns->type->type);	// 2 ret []any

	nx->nbody = list1(nod(OAS, ns, mkcall1(fn,  ns->type, &nx->ninit,
					       typename(ns->type),
					       ns,
					       conv(na, types[TINT64]))));
	l = list(l, nx);

	nn = temp(types[TINT]);
	l = list(l, nod(OAS, nn, nod(OLEN, ns, N)));	 // n = len(s)

	nx = nod(OSLICE, ns, nod(OKEY, N, nod(OADD, nn, na)));	 // ...s[:n+argc]
	nx->etype = 1;
	l = list(l, nod(OAS, ns, nx));			// s = s[:n+argc]

	for (a = n->list->next;	 a != nil; a = a->next) {
		nx = nod(OINDEX, ns, nn);		// s[n] ...
		nx->bounded = 1;
		l = list(l, nod(OAS, nx, a->n));	// s[n] = arg
		if (a->next != nil)
			l = list(l, nod(OAS, nn, nod(OADD, nn, nodintconst(1))));  // n = n + 1
	}

	typechecklist(l, Etop);
	walkstmtlist(l);
	*init = concat(*init, l);
	return ns;
}

// Lower copy(a, b) to a memmove call.
//
// init {
//   n := len(a)
//   if n > len(b) { n = len(b) }
//   memmove(a.ptr, b.ptr, n*sizeof(elem(a)))
// }
// n;
//
// Also works if b is a string.
//
Node* copyany(Node *n, NodeList **init)
{
	Node *nl, *nr, *nfrm, *nto, *nif, *nlen, *nwid, *fn;
	NodeList *l;

	walkexpr(&n->left, init);
	walkexpr(&n->right, init);
	nl = temp(n->left->type);
	nr = temp(n->right->type);
	l = nil;
	l = list(l, nod(OAS, nl, n->left));
	l = list(l, nod(OAS, nr, n->right));

	nfrm = nod(OSPTR, nr, N);
	nto = nod(OSPTR, nl, N);

	nlen = temp(types[TINT]);
	// n = len(to)
	l = list(l, nod(OAS, nlen, nod(OLEN, nl, N)));
	// if n > len(frm) { n = len(frm) }
	nif = nod(OIF, N, N);
	nif->ntest = nod(OGT, nlen, nod(OLEN, nr, N));
	nif->nbody = list(nif->nbody,
		nod(OAS, nlen, nod(OLEN, nr, N)));
	l = list(l, nif);

	// Call memmove.
	fn = syslook("memmove", 1);
	argtype(fn, nl->type->type);
	argtype(fn, nl->type->type);
	nwid = temp(types[TUINTPTR]);
	l = list(l, nod(OAS, nwid, conv(nlen, types[TUINTPTR])));
	nwid = nod(OMUL, nwid, nodintconst(nl->type->type->width));
	l = list(l, mkcall1(fn, T, init, nto, nfrm, nwid));

	typechecklist(l, Etop);
	walkstmtlist(l);
	*init = concat(*init, l);
	return nlen;
}
