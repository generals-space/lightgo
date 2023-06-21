#include	<u.h>
#include	<libc.h>
#include	"go.h"

// caller:
//  1. mal()
//  2. remal()
static void gethunk(void)
{
	char *h;
	int32 nh;

	nh = NHUNK;
	if(thunk >= 10L*NHUNK)
		nh = 10L*NHUNK;
	h = (char*)malloc(nh);
	if(h == nil) {
		flusherrors();
		yyerror("out of memory");
		errorexit();
	}
	hunk = h;
	nhunk = nh;
	thunk += nh;
}

void* mal(int32 n)
{
	void *p;

	if(n >= NHUNK) {
		p = malloc(n);
		if(p == nil) {
			flusherrors();
			yyerror("out of memory");
			errorexit();
		}
		memset(p, 0, n);
		return p;
	}

	while((uintptr)hunk & MAXALIGN) {
		hunk++;
		nhunk--;
	}
	if(nhunk < n)
		gethunk();

	p = hunk;
	nhunk -= n;
	hunk += n;
	memset(p, 0, n);
	return p;
}

void* remal(void *p, int32 on, int32 n)
{
	void *q;

	q = (uchar*)p + on;
	if(q != hunk || nhunk < n) {
		if(on+n >= NHUNK) {
			q = mal(on+n);
			memmove(q, p, on);
			return q;
		}
		if(nhunk < on+n)
			gethunk();
		memmove(hunk, p, on);
		p = hunk;
		hunk += on;
		nhunk -= on;
	}
	hunk += n;
	nhunk -= n;
	return p;
}
