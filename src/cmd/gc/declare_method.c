#include	<u.h>

#include	<libc.h>

#include	"declare.h"

// isifacemethod 判断目标 f 方法是否为某个 interface 的成员方法.
//
// 判断依据是: 普通对象的方法的 receiver 是其所属对象, 而 interface 方法的 receiver,
// 则为匿名的 *struct{}
//
// 	@param f: 一个**方法类型**的 Type(不是单纯的函数, f 包含 receiver)
//
// Is this field a method on an interface?
// Those methods have an anonymous *struct{} as the receiver.
// (See fakethis above.)
// 
int isifacemethod(Type *f)
{
	Type *rcvr;
	Type *t;

	rcvr = getthisx(f)->type;
	if(rcvr->sym != S)
		return 0;
	t = rcvr->type;
	if(!isptr[t->etype])
		return 0;
	t = t->type;
	if(t->sym != S || t->etype != TSTRUCT || t->type != T)
		return 0;
	return 1;
}

// caller:
// 	1. methodname()
Sym* methodsym(Sym *nsym, Type *t0, int iface)
{
	Sym *s;
	char *p;
	Type *t;
	char *suffix;
	Pkg *spkg;
	static Pkg *toppkg;

	t = t0;
	if(t == T)
		goto bad;
	s = t->sym;
	if(s == S && isptr[t->etype]) {
		t = t->type;
		if(t == T)
			goto bad;
		s = t->sym;
	}
	spkg = nil;
	if(s != S)
		spkg = s->pkg;

	// if t0 == *t and t0 has a sym,
	// we want to see *t, not t0, in the method name.
	if(t != t0 && t0->sym)
		t0 = ptrto(t);

	suffix = "";
	if(iface) {
		dowidth(t0);
		if(t0->width < types[tptr]->width)
			suffix = "·i";
	}
	if((spkg == nil || nsym->pkg != spkg) && !exportname(nsym->name)) {
		if(t0->sym == S && isptr[t0->etype])
			p = smprint("(%-hT).%s.%s%s", t0, nsym->pkg->prefix, nsym->name, suffix);
		else
			p = smprint("%-hT.%s.%s%s", t0, nsym->pkg->prefix, nsym->name, suffix);
	} else {
		if(t0->sym == S && isptr[t0->etype])
			p = smprint("(%-hT).%s%s", t0, nsym->name, suffix);
		else
			p = smprint("%-hT.%s%s", t0, nsym->name, suffix);
	}
	if(spkg == nil) {
		if(toppkg == nil)
			toppkg = mkpkg(strlit("go"));
		spkg = toppkg;
	}
	s = pkglookup(p, spkg);
	free(p);
	return s;

bad:
	yyerror("illegal receiver type: %T", t0);
	return S;
}

// caller:
// 	1. src/cmd/gc/typecheck_lookdot.c -> looktypedot()
// 	1. src/cmd/gc/typecheck_lookdot.c -> lookdot()
Node* methodname(Node *n, Type *t)
{
	Sym *s;

	s = methodsym(n->sym, t, 0);
	if(s == S)
		return n;
	return newname(s);
}

// caller:
// 	1. src/cmd/gc/go.y -> fndcl{}
// 	2. src/cmd/gc/go.y -> hidden_fndcl{}
Node* methodname1(Node *n, Node *t)
{
	char *star;
	char *p;

	star = nil;
	if(t->op == OIND) {
		star = "*";
		t = t->left;
	}
	if(t->sym == S || isblank(n))
		return newname(n->sym);

	if(star)
		p = smprint("(%s%S).%S", star, t->sym, n->sym);
	else
		p = smprint("%S.%S", t->sym, n->sym);

	if(exportname(t->sym->name))
		n = newname(lookup(p));
	else
		n = newname(pkglookup(p, t->sym->pkg));
	free(p);
	return n;
}

// 	@param sf: function的方法名称
//
// caller:
// 	1. src/cmd/gc/go.y -> hidden_fndcl{}
// 	2. src/cmd/gc/typecheck1_func.c -> typecheckfunc()
//
// add a method, declared as a function,
// n is fieldname, pa is base type, t is function type
// 
void addmethod(Sym *sf, Type *t, int local, int nointerface)
{
	Type *f, *d, *pa;
	Node *n;

	// get field sym
	if(sf == S)
		fatal("no method symbol");

	// get parent type sym
	pa = getthisx(t)->type;	// ptr to this structure
	if(pa == T) {
		yyerror("missing receiver");
		return;
	}

	pa = pa->type;
	f = methtype(pa, 1);
	if(f == T) {
		t = pa;
		if(t == T) // rely on typecheck having complained before
			return;
		if(t != T) {
			if(isptr[t->etype]) {
				if(t->sym != S) {
					yyerror("invalid receiver type %T (%T is a pointer type)", pa, t);
					return;
				}
				t = t->type;
			}
			if(t->broke) // rely on typecheck having complained before
				return;
			if(t->sym == S) {
				yyerror("invalid receiver type %T (%T is an unnamed type)", pa, t);
				return;
			}
			if(isptr[t->etype]) {
				yyerror("invalid receiver type %T (%T is a pointer type)", pa, t);
				return;
			}
			if(t->etype == TINTER) {
				yyerror("invalid receiver type %T (%T is an interface type)", pa, t);
				return;
			}
		}
		// Should have picked off all the reasons above,
		// but just in case, fall back to generic error.
		yyerror("invalid receiver type %T (%lT / %lT)", pa, pa, t);
		return;
	}

	pa = f;
	if(pa->etype == TSTRUCT) {
		for(f=pa->type; f; f=f->down) {
			if(f->sym == sf) {
				yyerror("type %T has both field and method named %S", pa, sf);
				return;
			}
		}
	}

	if(local && !pa->local) {
		// defining method on non-local type.
		yyerror("cannot define new methods on non-local type %T", pa);
		return;
	}

	n = nod(ODCLFIELD, newname(sf), N);
	n->type = t;

	d = T;	// last found
	for(f=pa->method; f!=T; f=f->down) {
		d = f;
		if(f->etype != TFIELD)
			fatal("addmethod: not TFIELD: %N", f);
		if(strcmp(sf->name, f->sym->name) != 0)
			continue;
		if(!eqtype(t, f->type))
			yyerror("method redeclared: %T.%S\n\t%T\n\t%T", pa, sf, f->type, t);
		return;
	}

	f = structfield(n);
	f->nointerface = nointerface;

	// during import unexported method names should be in the type's package
	if(importpkg && f->sym && !exportname(f->sym->name) && f->sym->pkg != structpkg)
		fatal("imported method name %+S in wrong package %s\n", f->sym, structpkg->name);

	if(d == T)
		pa->method = f;
	else
		d->down = f;
	return;
}
