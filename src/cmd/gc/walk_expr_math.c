#include	"walk.h"

// caller:
//  1. walkexpr() 只有这一处
//
// walkmul rewrites integer multiplication by powers of two as shifts.
// 
void walkmul(Node **np, NodeList **init)
{
	Node *n, *nl, *nr;
	int pow, neg, w;
	
	n = *np;
	if(!isint[n->type->etype]) {
		return;
	}

	if(n->right->op == OLITERAL) {
		nl = n->left;
		nr = n->right;
	} else if(n->left->op == OLITERAL) {
		nl = n->right;
		nr = n->left;
	} else {
		return;
	}

	neg = 0;

	// x*0 is 0 (and side effects of x).
	if(mpgetfix(nr->val.u.xval) == 0) {
		cheapexpr(nl, init);
		nodconst(n, n->type, 0);
		goto ret;
	}

	// nr is a constant.
	pow = powtwo(nr);
	if(pow < 0) {
		return;
	}
	if(pow >= 1000) {
		// negative power of 2, like -16
		neg = 1;
		pow -= 1000;
	}

	w = nl->type->width*8;
	if(pow+1 >= w)// too big, shouldn't happen
		return;

	nl = cheapexpr(nl, init);

	if(pow == 0) {
		// x*1 is x
		n = nl;
		goto ret;
	}
	
	n = nod(OLSH, nl, nodintconst(pow));

ret:
	if(neg)
		n = nod(OMINUS, n, N);

	typecheck(&n, Erv);
	walkexpr(&n, init);
	*np = n;
}

// caller:
//  1. walkexpr() 只有这一处
//
// walkdiv rewrites division by a constant as less expensive operations.
// 
void walkdiv(Node **np, NodeList **init)
{
	Node *n, *nl, *nr, *nc;
	Node *n1, *n2, *n3, *n4;
	int pow; // if >= 0, nr is 1<<pow
	int s; // 1 if nr is negative.
	int w;
	Type *twide;
	Magic m;

	n = *np;
	if(n->right->op != OLITERAL)
		return;
	// nr is a constant.
	nl = cheapexpr(n->left, init);
	nr = n->right;

	// special cases of mod/div
	// by a constant
	w = nl->type->width*8;
	s = 0;
	pow = powtwo(nr);
	if(pow >= 1000) {
		// negative power of 2
		s = 1;
		pow -= 1000;
	}

	if(pow+1 >= w) {
		// divisor too large.
		return;
	}
	if(pow < 0) {
		goto divbymul;
	}

	switch(pow) {
	case 0:
		if(n->op == OMOD) {
			// nl % 1 is zero.
			nodconst(n, n->type, 0);
		} else if(s) {
			// divide by -1
			n->op = OMINUS;
			n->right = N;
		} else {
			// divide by 1
			n = nl;
		}
		break;
	default:
		if(issigned[n->type->etype]) {
			if(n->op == OMOD) {
				// signed modulo 2^pow is like ANDing
				// with the last pow bits, but if nl < 0,
				// nl & (2^pow-1) is (nl+1)%2^pow - 1.
				nc = nod(OXXX, N, N);
				nodconst(nc, types[simtype[TUINT]], w-1);
				n1 = nod(ORSH, nl, nc); // n1 = -1 iff nl < 0.
				if(pow == 1) {
					typecheck(&n1, Erv);
					n1 = cheapexpr(n1, init);
					// n = (nl+ε)&1 -ε where ε=1 iff nl<0.
					n2 = nod(OSUB, nl, n1);
					nc = nod(OXXX, N, N);
					nodconst(nc, nl->type, 1);
					n3 = nod(OAND, n2, nc);
					n = nod(OADD, n3, n1);
				} else {
					// n = (nl+ε)&(nr-1) - ε where ε=2^pow-1 iff nl<0.
					nc = nod(OXXX, N, N);
					nodconst(nc, nl->type, (1LL<<pow)-1);
					n2 = nod(OAND, n1, nc); // n2 = 2^pow-1 iff nl<0.
					typecheck(&n2, Erv);
					n2 = cheapexpr(n2, init);

					n3 = nod(OADD, nl, n2);
					n4 = nod(OAND, n3, nc);
					n = nod(OSUB, n4, n2);
				}
				break;
			} else {
				// arithmetic right shift does not give the correct rounding.
				// if nl >= 0, nl >> n == nl / nr
				// if nl < 0, we want to add 2^n-1 first.
				nc = nod(OXXX, N, N);
				nodconst(nc, types[simtype[TUINT]], w-1);
				n1 = nod(ORSH, nl, nc); // n1 = -1 iff nl < 0.
				if(pow == 1) {
					// nl+1 is nl-(-1)
					n->left = nod(OSUB, nl, n1);
				} else {
					// Do a logical right right on -1 to keep pow bits.
					nc = nod(OXXX, N, N);
					nodconst(nc, types[simtype[TUINT]], w-pow);
					n2 = nod(ORSH, conv(n1, tounsigned(nl->type)), nc);
					n->left = nod(OADD, nl, conv(n2, nl->type));
				}
				// n = (nl + 2^pow-1) >> pow
				n->op = ORSH;
				nc = nod(OXXX, N, N);
				nodconst(nc, types[simtype[TUINT]], pow);
				n->right = nc;
				n->typecheck = 0;
			}
			if(s)
				n = nod(OMINUS, n, N);
			break;
		}
		nc = nod(OXXX, N, N);
		if(n->op == OMOD) {
			// n = nl & (nr-1)
			n->op = OAND;
			nodconst(nc, nl->type, mpgetfix(nr->val.u.xval)-1);
		} else {
			// n = nl >> pow
			n->op = ORSH;
			nodconst(nc, types[simtype[TUINT]], pow);
		}
		n->typecheck = 0;
		n->right = nc;
		break;
	}
	goto ret;

divbymul:
	// try to do division by multiply by (2^w)/d
	// see hacker's delight chapter 10
	// TODO: support 64-bit magic multiply here.
	m.w = w;
	if(issigned[nl->type->etype]) {
		m.sd = mpgetfix(nr->val.u.xval);
		smagic(&m);
	} else {
		m.ud = mpgetfix(nr->val.u.xval);
		umagic(&m);
	}
	if(m.bad)
		return;

	// We have a quick division method so use it
	// for modulo too.
	if(n->op == OMOD)
		goto longmod;

	switch(simtype[nl->type->etype]) {
	default:
		return;

	case TUINT8:
	case TUINT16:
	case TUINT32:
		// n1 = nl * magic >> w (HMUL)
		nc = nod(OXXX, N, N);
		nodconst(nc, nl->type, m.um);
		n1 = nod(OMUL, nl, nc);
		typecheck(&n1, Erv);
		n1->op = OHMUL;
		if(m.ua) {
			// Select a Go type with (at least) twice the width.
			switch(simtype[nl->type->etype]) {
			default:
				return;
			case TUINT8:
			case TUINT16:
				twide = types[TUINT32];
				break;
			case TUINT32:
				twide = types[TUINT64];
				break;
			case TINT8:
			case TINT16:
				twide = types[TINT32];
				break;
			case TINT32:
				twide = types[TINT64];
				break;
			}

			// add numerator (might overflow).
			// n2 = (n1 + nl)
			n2 = nod(OADD, conv(n1, twide), conv(nl, twide));

			// shift by m.s
			nc = nod(OXXX, N, N);
			nodconst(nc, types[TUINT], m.s);
			n = conv(nod(ORSH, n2, nc), nl->type);
		} else {
			// n = n1 >> m.s
			nc = nod(OXXX, N, N);
			nodconst(nc, types[TUINT], m.s);
			n = nod(ORSH, n1, nc);
		}
		break;

	case TINT8:
	case TINT16:
	case TINT32:
		// n1 = nl * magic >> w
		nc = nod(OXXX, N, N);
		nodconst(nc, nl->type, m.sm);
		n1 = nod(OMUL, nl, nc);
		typecheck(&n1, Erv);
		n1->op = OHMUL;
		if(m.sm < 0) {
			// add the numerator.
			n1 = nod(OADD, n1, nl);
		}
		// shift by m.s
		nc = nod(OXXX, N, N);
		nodconst(nc, types[TUINT], m.s);
		n2 = conv(nod(ORSH, n1, nc), nl->type);
		// add 1 iff n1 is negative.
		nc = nod(OXXX, N, N);
		nodconst(nc, types[TUINT], w-1);
		n3 = nod(ORSH, nl, nc); // n4 = -1 iff n1 is negative.
		n = nod(OSUB, n2, n3);
		// apply sign.
		if(m.sd < 0)
			n = nod(OMINUS, n, N);
		break;
	}
	goto ret;

longmod:
	// rewrite as A%B = A - (A/B*B).
	n1 = nod(ODIV, nl, nr);
	n2 = nod(OMUL, n1, nr);
	n = nod(OSUB, nl, n2);
	goto ret;

ret:
	typecheck(&n, Erv);
	walkexpr(&n, init);
	*np = n;
}
