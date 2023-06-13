#include	<u.h>
#include	<libc.h>
#include	"go.h"

typedef struct TypePairList TypePairList;
struct TypePairList
{
	Type *t1;
	Type *t2;
	TypePairList *next;
};

static int eqtype1(Type*, Type*, TypePairList*);
static int onlist(TypePairList *l, Type *t1, Type *t2);
static int eqnote(Strlit *a, Strlit *b);

// caller:
// 	1. implements()
//
// Return 1 if t1 and t2 are identical(相同的), following the spec rules.
//
// Any cyclic(循环的) type must go through a named type, and if one is
// named, it is only identical to the other if they are the same
// pointer (t1 == t2), so there's no chance of chasing cycles
// ad infinitum, so no need for a depth counter.
int eqtype(Type *t1, Type *t2)
{
	return eqtype1(t1, t2, nil);
}

static int eqtype1(Type *t1, Type *t2, TypePairList *assumed_equal)
{
	TypePairList l;

	if(t1 == t2)
		return 1;
	if(t1 == T || t2 == T || t1->etype != t2->etype)
		return 0;
	if(t1->sym || t2->sym) {
		// Special case: we keep byte and uint8 separate
		// for error messages.  Treat them as equal.
		switch(t1->etype) {
		case TUINT8:
			if((t1 == types[TUINT8] || t1 == bytetype) && (t2 == types[TUINT8] || t2 == bytetype))
				return 1;
			break;
		case TINT:
		case TINT32:
			if((t1 == types[runetype->etype] || t1 == runetype) && (t2 == types[runetype->etype] || t2 == runetype))
				return 1;
			break;
		}
		return 0;
	}

	if(onlist(assumed_equal, t1, t2))
		return 1;
	l.next = assumed_equal;
	l.t1 = t1;
	l.t2 = t2;

	switch(t1->etype) {
	case TINTER:
	case TSTRUCT:
		for(t1=t1->type, t2=t2->type; t1 && t2; t1=t1->down, t2=t2->down) {
			if(t1->etype != TFIELD || t2->etype != TFIELD)
				fatal("struct/interface missing field: %T %T", t1, t2);
			if(t1->sym != t2->sym || t1->embedded != t2->embedded || !eqtype1(t1->type, t2->type, &l) || !eqnote(t1->note, t2->note))
				goto no;
		}
		if(t1 == T && t2 == T)
			goto yes;
		goto no;

	case TFUNC:
		// Loop over structs: receiver, in, out.
		for(t1=t1->type, t2=t2->type; t1 && t2; t1=t1->down, t2=t2->down) {
			Type *ta, *tb;

			if(t1->etype != TSTRUCT || t2->etype != TSTRUCT)
				fatal("func missing struct: %T %T", t1, t2);

			// Loop over fields in structs, ignoring argument names.
			for(ta=t1->type, tb=t2->type; ta && tb; ta=ta->down, tb=tb->down) {
				if(ta->etype != TFIELD || tb->etype != TFIELD)
					fatal("func struct missing field: %T %T", ta, tb);
				if(ta->isddd != tb->isddd || !eqtype1(ta->type, tb->type, &l))
					goto no;
			}
			if(ta != T || tb != T)
				goto no;
		}
		if(t1 == T && t2 == T)
			goto yes;
		goto no;
	
	case TARRAY:
		if(t1->bound != t2->bound)
			goto no;
		break;
	
	case TCHAN:
		if(t1->chan != t2->chan)
			goto no;
		break;
	}

	if(eqtype1(t1->down, t2->down, &l) && eqtype1(t1->type, t2->type, &l))
		goto yes;
	goto no;

yes:
	return 1;

no:
	return 0;
}

// Are t1 and t2 equal struct types when field names are ignored?
// For deciding whether the result struct from g can be copied
// directly when compiling f(g()).
int eqtypenoname(Type *t1, Type *t2)
{
	if(t1 == T || t2 == T || t1->etype != TSTRUCT || t2->etype != TSTRUCT)
		return 0;

	t1 = t1->type;
	t2 = t2->type;
	for(;;) {
		if(!eqtype(t1, t2))
			return 0;
		if(t1 == T)
			return 1;
		t1 = t1->down;
		t2 = t2->down;
	}
}

// caller:
//  1. eqtype1() 只有这一处
static int eqnote(Strlit *a, Strlit *b)
{
	if(a == b)
		return 1;
	if(a == nil || b == nil)
		return 0;
	if(a->len != b->len)
		return 0;
	return memcmp(a->s, b->s, a->len) == 0;
}

static int onlist(TypePairList *l, Type *t1, Type *t2) 
{
	for(; l; l=l->next)
		if((l->t1 == t1 && l->t2 == t2) || (l->t1 == t2 && l->t2 == t1))
			return 1;
	return 0;
}
