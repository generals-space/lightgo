#include "typecheck.h"

/*
 * type check composite
 */

// caller:
//  1. typecheckcomplit() 只有这一处
static int nokeys(NodeList *l)
{
	for(; l; l=l->next)
		if(l->n->op == OKEY)
			return 0;
	return 1;
}

// caller:
//  1. typecheckcomplit() 只有这一处
static void fielddup(Node *n, Node *hash[], ulong nhash)
{
	uint h;
	char *s;
	Node *a;

	if(n->op != ONAME)
		fatal("fielddup: not ONAME");
	s = n->sym->name;
	h = stringhash(s)%nhash;
	for(a=hash[h]; a!=N; a=a->ntest) {
		if(strcmp(a->sym->name, s) == 0) {
			yyerror("duplicate field name in struct literal: %s", s);
			return;
		}
	}
	n->ntest = hash[h];
	hash[h] = n;
}

// caller:
//  1. typecheckcomplit() 只有这一处
static void keydup(Node *n, Node *hash[], ulong nhash)
{
	uint h;
	ulong b;
	double d;
	int i;
	Node *a;
	Node cmp;
	char *s;

	evconst(n);
	if(n->op != OLITERAL)
		return;	// we dont check variables

	switch(n->val.ctype) {
	default:	// unknown, bool, nil
		b = 23;
		break;
	case CTINT:
	case CTRUNE:
		b = mpgetfix(n->val.u.xval);
		break;
	case CTFLT:
		d = mpgetflt(n->val.u.fval);
		s = (char*)&d;
		b = 0;
		for(i=sizeof(d); i>0; i--)
			b = b*PRIME1 + *s++;
		break;
	case CTSTR:
		b = 0;
		s = n->val.u.sval->s;
		for(i=n->val.u.sval->len; i>0; i--)
			b = b*PRIME1 + *s++;
		break;
	}

	h = b%nhash;
	memset(&cmp, 0, sizeof(cmp));
	for(a=hash[h]; a!=N; a=a->ntest) {
		cmp.op = OEQ;
		cmp.left = n;
		cmp.right = a;
		evconst(&cmp);
		b = cmp.val.u.bval;
		if(b) {
			// too lazy to print the literal
			yyerror("duplicate key %N in map literal", n);
			return;
		}
	}
	n->ntest = hash[h];
	hash[h] = n;
}

// caller:
//  1. typecheckcomplit() 只有这一处
static void indexdup(Node *n, Node *hash[], ulong nhash)
{
	uint h;
	Node *a;
	ulong b, c;

	if(n->op != OLITERAL)
		fatal("indexdup: not OLITERAL");

	b = mpgetfix(n->val.u.xval);
	h = b%nhash;
	for(a=hash[h]; a!=N; a=a->ntest) {
		c = mpgetfix(a->val.u.xval);
		if(b == c) {
			yyerror("duplicate index in array literal: %ld", b);
			return;
		}
	}
	n->ntest = hash[h];
	hash[h] = n;
}

// caller:
//  1. inithash() 只有这一处
static int prime(ulong h, ulong sr)
{
	ulong n;

	for(n=3; n<=sr; n+=2)
		if(h%n == 0)
			return 0;
	return 1;
}

// caller:
//  1. typecheckcomplit() 只有这一处
static ulong inithash(Node *n, Node ***hash, Node **autohash, ulong nautohash)
{
	ulong h, sr;
	NodeList *ll;
	int i;

	// count the number of entries
	h = 0;
	for(ll=n->list; ll; ll=ll->next)
		h++;

	// if the auto hash table is
	// large enough use it.
	if(h <= nautohash) {
		*hash = autohash;
		memset(*hash, 0, nautohash * sizeof(**hash));
		return nautohash;
	}

	// make hash size odd and 12% larger than entries
	h += h/8;
	h |= 1;

	// calculate sqrt of h
	sr = h/2;
	for(i=0; i<5; i++)
		sr = (sr + h/sr)/2;

	// check for primeality
	while(!prime(h, sr))
		h += 2;

	// build and return a throw-away hash table
	*hash = mal(h * sizeof(**hash));
	memset(*hash, 0, h * sizeof(**hash));
	return h;
}

// caller:
//  1. pushtype() 
//  2. typecheckcomplit() 
static int iscomptype(Type *t)
{
	switch(t->etype) {
	case TARRAY:
	case TSTRUCT:
	case TMAP:
		return 1;
	case TPTR32:
	case TPTR64:
		switch(t->type->etype) {
		case TARRAY:
		case TSTRUCT:
		case TMAP:
			return 1;
		}
		break;
	}
	return 0;
}

// caller:
//  1. typecheckcomplit() 只有这一处
static void pushtype(Node *n, Type *t)
{
	if(n == N || n->op != OCOMPLIT || !iscomptype(t))
		return;
	
	if(n->right == N) {
		n->right = typenod(t);
		n->implicit = 1;  // don't print
		n->right->implicit = 1;  // * is okay
	}
	else if(debug['s']) {
		typecheck(&n->right, Etype);
		if(n->right->type != T && eqtype(n->right->type, t))
			print("%lL: redundant type: %T\n", n->lineno, t);
	}
}

// caller:
//  1. typecheck1() 只有这一处
void typecheckcomplit(Node **np)
{
	int bad, i, nerr;
	int64 len;
	Node *l, *n, *norig, *r, **hash;
	NodeList *ll;
	Type *t, *f;
	Sym *s, *s1;
	int32 lno;
	ulong nhash;
	Node *autohash[101];

	n = *np;
	lno = lineno;

	if(n->right == N) {
		if(n->list != nil)
			setlineno(n->list->n);
		yyerror("missing type in composite literal");
		goto error;
	}

	// Save original node (including n->right)
	norig = nod(n->op, N, N);
	*norig = *n;

	setlineno(n->right);
	l = typecheck(&n->right /* sic */, Etype|Ecomplit);
	if((t = l->type) == T)
		goto error;
	nerr = nerrors;
	n->type = t;

	if(isptr[t->etype]) {
		// For better or worse, we don't allow pointers as the composite literal type,
		// except when using the &T syntax, which sets implicit on the OIND.
		if(!n->right->implicit) {
			yyerror("invalid pointer type %T for composite literal (use &%T instead)", t, t->type);
			goto error;
		}
		// Also, the underlying type must be a struct, map, slice, or array.
		if(!iscomptype(t)) {
			yyerror("invalid pointer type %T for composite literal", t);
			goto error;
		}
		t = t->type;
	}

	switch(t->etype) {
	default:
		yyerror("invalid type for composite literal: %T", t);
		n->type = T;
		break;

	case TARRAY:
		nhash = inithash(n, &hash, autohash, nelem(autohash));

		len = 0;
		i = 0;
		for(ll=n->list; ll; ll=ll->next) {
			l = ll->n;
			setlineno(l);
			if(l->op != OKEY) {
				l = nod(OKEY, nodintconst(i), l);
				l->left->type = types[TINT];
				l->left->typecheck = 1;
				ll->n = l;
			}

			typecheck(&l->left, Erv);
			evconst(l->left);
			i = nonnegconst(l->left);
			if(i < 0) {
				yyerror("array index must be non-negative integer constant");
				i = -(1<<30);	// stay negative for a while
			}
			if(i >= 0)
				indexdup(l->left, hash, nhash);
			i++;
			if(i > len) {
				len = i;
				if(t->bound >= 0 && len > t->bound) {
					setlineno(l);
					yyerror("array index %d out of bounds [0:%d]", len, t->bound);
					t->bound = -1;	// no more errors
				}
			}

			r = l->right;
			pushtype(r, t->type);
			typecheck(&r, Erv);
			defaultlit(&r, t->type);
			l->right = assignconv(r, t->type, "array element");
		}
		if(t->bound == -100)
			t->bound = len;
		if(t->bound < 0)
			n->right = nodintconst(len);
		n->op = OARRAYLIT;
		break;

	case TMAP:
		nhash = inithash(n, &hash, autohash, nelem(autohash));

		for(ll=n->list; ll; ll=ll->next) {
			l = ll->n;
			setlineno(l);
			if(l->op != OKEY) {
				typecheck(&ll->n, Erv);
				yyerror("missing key in map literal");
				continue;
			}

			typecheck(&l->left, Erv);
			defaultlit(&l->left, t->down);
			l->left = assignconv(l->left, t->down, "map key");
			if (l->left->op != OCONV)
				keydup(l->left, hash, nhash);

			r = l->right;
			pushtype(r, t->type);
			typecheck(&r, Erv);
			defaultlit(&r, t->type);
			l->right = assignconv(r, t->type, "map value");
		}
		n->op = OMAPLIT;
		break;

	case TSTRUCT:
		bad = 0;
		if(n->list != nil && nokeys(n->list)) {
			// simple list of variables
			f = t->type;
			for(ll=n->list; ll; ll=ll->next) {
				setlineno(ll->n);
				typecheck(&ll->n, Erv);
				if(f == nil) {
					if(!bad++)
						yyerror("too many values in struct initializer");
					continue;
				}
				s = f->sym;
				if(s != nil && !exportname(s->name) && s->pkg != localpkg)
					yyerror("implicit assignment of unexported field '%s' in %T literal", s->name, t);
				// No pushtype allowed here.  Must name fields for that.
				ll->n = assignconv(ll->n, f->type, "field value");
				ll->n = nod(OKEY, newname(f->sym), ll->n);
				ll->n->left->type = f;
				ll->n->left->typecheck = 1;
				f = f->down;
			}
			if(f != nil)
				yyerror("too few values in struct initializer");
		} else {
			nhash = inithash(n, &hash, autohash, nelem(autohash));

			// keyed list
			for(ll=n->list; ll; ll=ll->next) {
				l = ll->n;
				setlineno(l);
				if(l->op != OKEY) {
					if(!bad++)
						yyerror("mixture of field:value and value initializers");
					typecheck(&ll->n, Erv);
					continue;
				}
				s = l->left->sym;
				if(s == S) {
					yyerror("invalid field name %N in struct initializer", l->left);
					typecheck(&l->right, Erv);
					continue;
				}

				// Sym might have resolved to name in other top-level
				// package, because of import dot.  Redirect to correct sym
				// before we do the lookup.
				if(s->pkg != localpkg && exportname(s->name)) {
					s1 = lookup(s->name);
					if(s1->origpkg == s->pkg)
						s = s1;
				}
				f = lookdot1(nil, s, t, t->type, 0);
				if(f == nil) {
					yyerror("unknown %T field '%S' in struct literal", t, s);
					continue;
				}
				l->left = newname(s);
				l->left->typecheck = 1;
				l->left->type = f;
				s = f->sym;
				fielddup(newname(s), hash, nhash);
				r = l->right;
				// No pushtype allowed here.  Tried and rejected.
				typecheck(&r, Erv);
				l->right = assignconv(r, f->type, "field value");
			}
		}
		n->op = OSTRUCTLIT;
		break;
	}
	if(nerr != nerrors)
		goto error;
	
	n->orig = norig;
	if(isptr[n->type->etype]) {
		n = nod(OPTRLIT, n, N);
		n->typecheck = 1;
		n->type = n->left->type;
		n->left->type = t;
		n->left->typecheck = 1;
	}

	n->orig = norig;
	*np = n;
	lineno = lno;
	return;

error:
	n->type = T;
	*np = n;
	lineno = lno;
}
