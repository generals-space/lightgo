// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include	<u.h>
#include	<libc.h>
#include	"go.h"

/// uses arithmetic

int
mpcmpfixflt(Mpint *a, Mpflt *b)
{
	char buf[500];
	Mpflt c;

	snprint(buf, sizeof(buf), "%B", a);
	mpatoflt(&c, buf);
	return mpcmpfltflt(&c, b);
}

int
mpcmpfltfix(Mpflt *a, Mpint *b)
{
	char buf[500];
	Mpflt c;

	snprint(buf, sizeof(buf), "%B", b);
	mpatoflt(&c, buf);
	return mpcmpfltflt(a, &c);
}

int
mpcmpfixfix(Mpint *a, Mpint *b)
{
	Mpint c;

	mpmovefixfix(&c, a);
	mpsubfixfix(&c, b);
	return mptestfix(&c);
}

int
mpcmpfixc(Mpint *b, vlong c)
{
	Mpint c1;

	mpmovecfix(&c1, c);
	return mpcmpfixfix(b, &c1);
}

int
mpcmpfltflt(Mpflt *a, Mpflt *b)
{
	Mpflt c;

	mpmovefltflt(&c, a);
	mpsubfltflt(&c, b);
	return mptestflt(&c);
}

int
mpcmpfltc(Mpflt *b, double c)
{
	Mpflt a;

	mpmovecflt(&a, c);
	return mpcmpfltflt(&a, b);
}

void
mpsubfixfix(Mpint *a, Mpint *b)
{
	mpnegfix(a);
	mpaddfixfix(a, b, 0);
	mpnegfix(a);
}

void
mpsubfltflt(Mpflt *a, Mpflt *b)
{
	mpnegflt(a);
	mpaddfltflt(a, b);
	mpnegflt(a);
}

void
mpaddcfix(Mpint *a, vlong c)
{
	Mpint b;

	mpmovecfix(&b, c);
	mpaddfixfix(a, &b, 0);
}

void
mpaddcflt(Mpflt *a, double c)
{
	Mpflt b;

	mpmovecflt(&b, c);
	mpaddfltflt(a, &b);
}

void
mpmulcfix(Mpint *a, vlong c)
{
	Mpint b;

	mpmovecfix(&b, c);
	mpmulfixfix(a, &b);
}

void
mpmulcflt(Mpflt *a, double c)
{
	Mpflt b;

	mpmovecflt(&b, c);
	mpmulfltflt(a, &b);
}

void
mpdivfixfix(Mpint *a, Mpint *b)
{
	Mpint q, r;

	mpdivmodfixfix(&q, &r, a, b);
	mpmovefixfix(a, &q);
}

void
mpmodfixfix(Mpint *a, Mpint *b)
{
	Mpint q, r;

	mpdivmodfixfix(&q, &r, a, b);
	mpmovefixfix(a, &r);
}

void
mpcomfix(Mpint *a)
{
	Mpint b;

	mpmovecfix(&b, 1);
	mpnegfix(a);
	mpsubfixfix(a, &b);
}

void
mpmovefixflt(Mpflt *a, Mpint *b)
{
	a->val = *b;
	a->exp = 0;
	mpnorm(a);
}

// convert (truncate) b to a.
// return -1 (but still convert) if b was non-integer.
static int
mpexactfltfix(Mpint *a, Mpflt *b)
{
	Mpflt f;

	*a = b->val;
	mpshiftfix(a, b->exp);
	if(b->exp < 0) {
		f.val = *a;
		f.exp = 0;
		mpnorm(&f);
		if(mpcmpfltflt(b, &f) != 0)
			return -1;
	}
	return 0;
}

int
mpmovefltfix(Mpint *a, Mpflt *b)
{
	Mpflt f;
	int i;

	if(mpexactfltfix(a, b) == 0)
		return 0;

	// try rounding down a little
	f = *b;
	f.val.a[0] = 0;
	if(mpexactfltfix(a, &f) == 0)
		return 0;

	// try rounding up a little
	for(i=1; i<Mpprec; i++) {
		f.val.a[i]++;
		if(f.val.a[i] != Mpbase)
			break;
		f.val.a[i] = 0;
	}
	mpnorm(&f);
	if(mpexactfltfix(a, &f) == 0)
		return 0;

	return -1;
}

void
mpmovefixfix(Mpint *a, Mpint *b)
{
	*a = *b;
}

void
mpmovefltflt(Mpflt *a, Mpflt *b)
{
	*a = *b;
}

static	double	tab[] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7 };
static void
mppow10flt(Mpflt *a, int p)
{
	if(p < 0)
		abort();
	if(p < nelem(tab)) {
		mpmovecflt(a, tab[p]);
		return;
	}
	mppow10flt(a, p>>1);
	mpmulfltflt(a, a);
	if(p & 1)
		mpmulcflt(a, 10);
}

static void
mphextofix(Mpint *a, char *s, int n)
{
	char *hexdigitp, *end, c;
	long d;
	int bit;

	while(*s == '0') {
		s++;
		n--;
	}

	// overflow
	if(4*n > Mpscale*Mpprec) {
		a->ovf = 1;
		return;
	}

	end = s+n-1;
	for(hexdigitp=end; hexdigitp>=s; hexdigitp--) {
		c = *hexdigitp;
		if(c >= '0' && c <= '9')
			d = c-'0';
		else if(c >= 'A' && c <= 'F')
			d = c-'A'+10;
		else
			d = c-'a'+10;

		bit = 4*(end - hexdigitp);
		while(d > 0) {
			if(d & 1)
				a->a[bit/Mpscale] |= (long)1 << (bit%Mpscale);
			bit++;
			d = d >> 1;
		}
	}
}

//
// floating point input
// required syntax is [+-]d*[.]d*[e[+-]d*] or [+-]0xH*[e[+-]d*]
//
void
mpatoflt(Mpflt *a, char *as)
{
	Mpflt b;
	int dp, c, f, ef, ex, eb, base;
	char *s, *start;

	while(*as == ' ' || *as == '\t')
		as++;

	/* determine base */
	s = as;
	base = -1;
	while(base == -1) {
		switch(*s++) {
		case '-':
		case '+':
			break;

		case '0':
			if(*s == 'x')
				base = 16;
			else
				base = 10;
			break;

		default:
			base = 10;
		}
	}

	s = as;
	dp = 0;		/* digits after decimal point */
	f = 0;		/* sign */
	ex = 0;		/* exponent */
	eb = 0;		/* binary point */

	mpmovecflt(a, 0.0);
	if(base == 16) {
		start = nil;
		for(;;) {
			c = *s;
			if(c == '-') {
				f = 1;
				s++;
			}
			else if(c == '+') {
				s++;
			}
			else if(c == '0' && s[1] == 'x') {
				s += 2;
				start = s;
			}
			else if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
				s++;
			}
			else {
				break;
			}
		}
		if(start == nil)
			goto bad;

		mphextofix(&a->val, start, s-start);
		if(a->val.ovf)
			goto bad;
		a->exp = 0;
		mpnorm(a);
	}
	for(;;) {
		switch(c = *s++) {
		default:
			goto bad;

		case '-':
			f = 1;

		case ' ':
		case '\t':
		case '+':
			continue;

		case '.':
			if(base == 16)
				goto bad;
			dp = 1;
			continue;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
			mpmulcflt(a, 10);
			mpaddcflt(a, c-'0');
			if(dp)
				dp++;
			continue;

		case 'P':
		case 'p':
			eb = 1;

		case 'E':
		case 'e':
			ex = 0;
			ef = 0;
			for(;;) {
				c = *s++;
				if(c == '+' || c == ' ' || c == '\t')
					continue;
				if(c == '-') {
					ef = 1;
					continue;
				}
				if(c >= '0' && c <= '9') {
					ex = ex*10 + (c-'0');
					if(ex > 1e8) {
						yyerror("constant exponent out of range: %s", as);
						errorexit();
					}
					continue;
				}
				break;
			}
			if(ef)
				ex = -ex;

		case 0:
			break;
		}
		break;
	}

	if(eb) {
		if(dp)
			goto bad;
		a->exp += ex;
		goto out;
	}

	if(dp)
		dp--;
	if(mpcmpfltc(a, 0.0) != 0) {
		if(ex >= dp) {
			mppow10flt(&b, ex-dp);
			mpmulfltflt(a, &b);
		} else {
			mppow10flt(&b, dp-ex);
			mpdivfltflt(a, &b);
		}
	}

out:
	if(f)
		mpnegflt(a);
	return;

bad:
	yyerror("constant too large: %s", as);
	mpmovecflt(a, 0.0);
}

// caller:
// 	1. src/cmd/gc/lex.c -> _yylex()
//
// fixed point input
// required syntax is [+-][0[x]]d*
//
void mpatofix(Mpint *a, char *as)
{
	int c, f;
	char *s, *s0;

	s = as;
	f = 0;
	mpmovecfix(a, 0);

	c = *s++;
	switch(c) {
	case '-':
		f = 1;

	case '+':
		c = *s++;
		if(c != '0')
			break;

	case '0':
		goto oct;
	}

	while(c) {
		if(c >= '0' && c <= '9') {
			mpmulcfix(a, 10);
			mpaddcfix(a, c-'0');
			c = *s++;
			continue;
		}
		goto bad;
	}
	goto out;

oct: // 八进制
{
	c = *s++;
	if(c == 'x' || c == 'X') {
		goto hex;
	}
	// 对于 0o755 这种八进制格式, 如遇到字符 o/O, 还要再将指针后移一位再处理.
	if(c == 'o' || c == 'O') {
		c = *s++;
	}
	while(c) {
		if(c >= '0' && c <= '7') {
			mpmulcfix(a, 8);
			mpaddcfix(a, c-'0');
			c = *s++;
			continue;
		}
		goto bad;
	}
	goto out;
}

hex: // 十六进制
{
	s0 = s;
	c = *s;
	while(c) {
		if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
			s++;
			c = *s;
			continue;
		}
		goto bad;
	}
	mphextofix(a, s0, s-s0);
	if(a->ovf) {
		goto bad;
	}
}

out:
	if(f)
		mpnegfix(a);
	return;

bad:
	yyerror("constant too large: %s", as);
	mpmovecfix(a, 0);
}

int
Bconv(Fmt *fp)
{
	char buf[500], *p;
	Mpint *xval, q, r, ten, sixteen;
	int f, digit;

	xval = va_arg(fp->args, Mpint*);
	mpmovefixfix(&q, xval);
	f = 0;
	if(mptestfix(&q) < 0) {
		f = 1;
		mpnegfix(&q);
	}

	p = &buf[sizeof(buf)];
	*--p = 0;
	if(fp->flags & FmtSharp) {
		// Hexadecimal
		mpmovecfix(&sixteen, 16);
		for(;;) {
			mpdivmodfixfix(&q, &r, &q, &sixteen);
			digit = mpgetfix(&r);
			if(digit < 10)
				*--p = digit + '0';
			else
				*--p = digit - 10 + 'A';
			if(mptestfix(&q) <= 0)
				break;
		}
		*--p = 'x';
		*--p = '0';
	} else {
		// Decimal
		mpmovecfix(&ten, 10);
		for(;;) {
			mpdivmodfixfix(&q, &r, &q, &ten);
			*--p = mpgetfix(&r) + '0';
			if(mptestfix(&q) <= 0)
				break;
		}
	}
	if(f)
		*--p = '-';
	return fmtstrcpy(fp, p);
}

int
Fconv(Fmt *fp)
{
	char buf[500];
	Mpflt *fvp, fv;
	double d;

	fvp = va_arg(fp->args, Mpflt*);
	if(fp->flags & FmtSharp) {
		// alternate form - decimal for error messages.
		// for well in range, convert to double and use print's %g
		if(-900 < fvp->exp && fvp->exp < 900) {
			d = mpgetflt(fvp);
			if(d >= 0 && (fp->flags & FmtSign))
				fmtprint(fp, "+");
			return fmtprint(fp, "%g", d);
		}
		// TODO(rsc): for well out of range, print
		// an approximation like 1.234e1000
	}

	if(sigfig(fvp) == 0) {
		snprint(buf, sizeof(buf), "0p+0");
		goto out;
	}
	fv = *fvp;

	while(fv.val.a[0] == 0) {
		mpshiftfix(&fv.val, -Mpscale);
		fv.exp += Mpscale;
	}
	while((fv.val.a[0]&1) == 0) {
		mpshiftfix(&fv.val, -1);
		fv.exp += 1;
	}

	if(fv.exp >= 0) {
		snprint(buf, sizeof(buf), "%#Bp+%d", &fv.val, fv.exp);
		goto out;
	}
	snprint(buf, sizeof(buf), "%#Bp-%d", &fv.val, -fv.exp);

out:
	return fmtstrcpy(fp, buf);
}
