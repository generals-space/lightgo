#include	<u.h>
#include	<libc.h>
#include	"go.h"

/*
 * return power of 2 of the constant
 * operand. -1 if it is not a power of 2.
 * 1000+ if it is a -(power of 2)
 */
int powtwo(Node *n)
{
	uvlong v, b;
	int i;

	if(n == N || n->op != OLITERAL || n->type == T)
		goto no;
	if(!isint[n->type->etype])
		goto no;

	v = mpgetfix(n->val.u.xval);
	b = 1ULL;
	for(i=0; i<64; i++) {
		if(b == v)
			return i;
		b = b<<1;
	}

	if(!issigned[n->type->etype])
		goto no;

	v = -v;
	b = 1ULL;
	for(i=0; i<64; i++) {
		if(b == v)
			return i+1000;
		b = b<<1;
	}

no:
	return -1;
}

/*
 * return the unsigned type for a signed integer type.
 * returns T if input is not a signed integer type.
 */
Type* tounsigned(Type *t)
{

	// this is types[et+1], but not sure
	// that this relation is immutable
	switch(t->etype) {
	default:
		print("tounsigned: unknown type %T\n", t);
		t = T;
		break;
	case TINT:
		t = types[TUINT];
		break;
	case TINT8:
		t = types[TUINT8];
		break;
	case TINT16:
		t = types[TUINT16];
		break;
	case TINT32:
		t = types[TUINT32];
		break;
	case TINT64:
		t = types[TUINT64];
		break;
	}
	return t;
}

/*
 * magic number for signed division see hacker's delight chapter 10
 */
void smagic(Magic *m)
{
	int p;
	uint64 ad, anc, delta, q1, r1, q2, r2, t;
	uint64 mask, two31;

	m->bad = 0;
	switch(m->w) {
	default:
		m->bad = 1;
		return;
	case 8:
		mask = 0xffLL;
		break;
	case 16:
		mask = 0xffffLL;
		break;
	case 32:
		mask = 0xffffffffLL;
		break;
	case 64:
		mask = 0xffffffffffffffffLL;
		break;
	}
	two31 = mask ^ (mask>>1);

	p = m->w-1;
	ad = m->sd;
	if(m->sd < 0)
		ad = -m->sd;

	// bad denominators
	if(ad == 0 || ad == 1 || ad == two31) {
		m->bad = 1;
		return;
	}

	t = two31;
	ad &= mask;

	anc = t - 1 - t%ad;
	anc &= mask;

	q1 = two31/anc;
	r1 = two31 - q1*anc;
	q1 &= mask;
	r1 &= mask;

	q2 = two31/ad;
	r2 = two31 - q2*ad;
	q2 &= mask;
	r2 &= mask;

	for(;;) {
		p++;
		q1 <<= 1;
		r1 <<= 1;
		q1 &= mask;
		r1 &= mask;
		if(r1 >= anc) {
			q1++;
			r1 -= anc;
			q1 &= mask;
			r1 &= mask;
		}

		q2 <<= 1;
		r2 <<= 1;
		q2 &= mask;
		r2 &= mask;
		if(r2 >= ad) {
			q2++;
			r2 -= ad;
			q2 &= mask;
			r2 &= mask;
		}

		delta = ad - r2;
		delta &= mask;
		if(q1 < delta || (q1 == delta && r1 == 0)) {
			continue;
		}
		break;
	}

	m->sm = q2+1;
	if(m->sm & two31)
		m->sm |= ~mask;
	m->s = p-m->w;
}

/*
 * magic number for unsigned division see hacker's delight chapter 10
 */
void umagic(Magic *m)
{
	int p;
	uint64 nc, delta, q1, r1, q2, r2;
	uint64 mask, two31;

	m->bad = 0;
	m->ua = 0;

	switch(m->w) {
	default:
		m->bad = 1;
		return;
	case 8:
		mask = 0xffLL;
		break;
	case 16:
		mask = 0xffffLL;
		break;
	case 32:
		mask = 0xffffffffLL;
		break;
	case 64:
		mask = 0xffffffffffffffffLL;
		break;
	}
	two31 = mask ^ (mask>>1);

	m->ud &= mask;
	if(m->ud == 0 || m->ud == two31) {
		m->bad = 1;
		return;
	}
	nc = mask - (-m->ud&mask)%m->ud;
	p = m->w-1;

	q1 = two31/nc;
	r1 = two31 - q1*nc;
	q1 &= mask;
	r1 &= mask;

	q2 = (two31-1) / m->ud;
	r2 = (two31-1) - q2*m->ud;
	q2 &= mask;
	r2 &= mask;

	for(;;) {
		p++;
		if(r1 >= nc-r1) {
			q1 <<= 1;
			q1++;
			r1 <<= 1;
			r1 -= nc;
		} else {
			q1 <<= 1;
			r1 <<= 1;
		}
		q1 &= mask;
		r1 &= mask;
		if(r2+1 >= m->ud-r2) {
			if(q2 >= two31-1) {
				m->ua = 1;
			}
			q2 <<= 1;
			q2++;
			r2 <<= 1;
			r2++;
			r2 -= m->ud;
		} else {
			if(q2 >= two31) {
				m->ua = 1;
			}
			q2 <<= 1;
			r2 <<= 1;
			r2++;
		}
		q2 &= mask;
		r2 &= mask;

		delta = m->ud - 1 - r2;
		delta &= mask;

		if(p < m->w+m->w)
		if(q1 < delta || (q1 == delta && r1 == 0)) {
			continue;
		}
		break;
	}
	m->um = q2+1;
	m->s = p-m->w;
}
