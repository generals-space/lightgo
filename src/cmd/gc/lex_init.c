#include	<u.h>

#include	<libc.h>

#include	"go.h"
#include	"y.tab.h"

static	struct
{
	char*	name;
	int	lexical;
	int	etype;
	int	op;
} syms[] =
{
/*	name		lexical		etype		op
 */
/* basic types */
	"int8",		LNAME,		TINT8,		OXXX,
	"int16",	LNAME,		TINT16,		OXXX,
	"int32",	LNAME,		TINT32,		OXXX,
	"int64",	LNAME,		TINT64,		OXXX,

	"uint8",	LNAME,		TUINT8,		OXXX,
	"uint16",	LNAME,		TUINT16,	OXXX,
	"uint32",	LNAME,		TUINT32,	OXXX,
	"uint64",	LNAME,		TUINT64,	OXXX,

	"float32",	LNAME,		TFLOAT32,	OXXX,
	"float64",	LNAME,		TFLOAT64,	OXXX,

	"complex64",	LNAME,		TCOMPLEX64,	OXXX,
	"complex128",	LNAME,		TCOMPLEX128,	OXXX,

	"bool",		LNAME,		TBOOL,		OXXX,
	"string",	LNAME,		TSTRING,	OXXX,

	"any",		LNAME,		TANY,		OXXX,

	"break",	LBREAK,		Txxx,		OXXX,
	"case",		LCASE,		Txxx,		OXXX,
	"chan",		LCHAN,		Txxx,		OXXX,
	"const",	LCONST,		Txxx,		OXXX,
	"continue",	LCONTINUE,	Txxx,		OXXX,
	"default",	LDEFAULT,	Txxx,		OXXX,
	"else",		LELSE,		Txxx,		OXXX,
	"defer",	LDEFER,		Txxx,		OXXX,
	"fallthrough",	LFALL,		Txxx,		OXXX,
	"for",		LFOR,		Txxx,		OXXX,
	"func",		LFUNC,		Txxx,		OXXX,
	"go",		LGO,		Txxx,		OXXX,
	"goto",		LGOTO,		Txxx,		OXXX,
	"if",		LIF,		Txxx,		OXXX,
	"import",	LIMPORT,	Txxx,		OXXX,
	"interface",	LINTERFACE,	Txxx,		OXXX,
	"map",		LMAP,		Txxx,		OXXX,
	"package",	LPACKAGE,	Txxx,		OXXX,
	"range",	LRANGE,		Txxx,		OXXX,
	"return",	LRETURN,	Txxx,		OXXX,
	"select",	LSELECT,	Txxx,		OXXX,
	"struct",	LSTRUCT,	Txxx,		OXXX,
	"switch",	LSWITCH,	Txxx,		OXXX,
	"type",		LTYPE,		Txxx,		OXXX,
	"var",		LVAR,		Txxx,		OXXX,

	"append",	LNAME,		Txxx,		OAPPEND,
	"cap",		LNAME,		Txxx,		OCAP,
	"close",	LNAME,		Txxx,		OCLOSE,
	"complex",	LNAME,		Txxx,		OCOMPLEX,
	"copy",		LNAME,		Txxx,		OCOPY,
	"delete",	LNAME,		Txxx,		ODELETE,
	"imag",		LNAME,		Txxx,		OIMAG,
	"len",		LNAME,		Txxx,		OLEN,
	"make",		LNAME,		Txxx,		OMAKE,
	"new",		LNAME,		Txxx,		ONEW,
	"panic",	LNAME,		Txxx,		OPANIC,
	"print",	LNAME,		Txxx,		OPRINT,
	"println",	LNAME,		Txxx,		OPRINTN,
	"real",		LNAME,		Txxx,		OREAL,
	"recover",	LNAME,		Txxx,		ORECOVER,

	"notwithstanding",		LIGNORE,	Txxx,		OXXX,
	"thetruthofthematter",		LIGNORE,	Txxx,		OXXX,
	"despiteallobjections",		LIGNORE,	Txxx,		OXXX,
	"whereas",			LIGNORE,	Txxx,		OXXX,
	"insofaras",			LIGNORE,	Txxx,		OXXX,
};

// caller:
// 	1. main()
void lexinit(void)
{
	int i, lex;
	Sym *s, *s1;
	Type *t;
	int etype;
	Val v;

	/*
	 * initialize basic types array
	 * initialize known symbols
	 */
	for(i=0; i<nelem(syms); i++) {
		lex = syms[i].lexical;
		s = lookup(syms[i].name);
		s->lexical = lex;

		etype = syms[i].etype;
		if(etype != Txxx) {
			if(etype < 0 || etype >= nelem(types)) {
				fatal("lexinit: %s bad etype", s->name);
			}
			s1 = pkglookup(syms[i].name, builtinpkg);
			t = types[etype];
			if(t == T) {
				t = typ(etype);
				t->sym = s1;

				if(etype != TANY && etype != TSTRING) {
					dowidth(t);
				}
				types[etype] = t;
			}
			s1->lexical = LNAME;
			s1->def = typenod(t);
			continue;
		}

		etype = syms[i].op;
		if(etype != OXXX) {
			s1 = pkglookup(syms[i].name, builtinpkg);
			s1->lexical = LNAME;
			s1->def = nod(ONAME, N, N);
			s1->def->sym = s1;
			s1->def->etype = etype;
			s1->def->builtin = 1;
		}
	}

	// logically, the type of a string literal.
	// types[TSTRING] is the named type string
	// (the type of x in var x string or var x = "hello").
	// this is the ideal form
	// (the type of x in const x = "hello").
	idealstring = typ(TSTRING);
	idealbool = typ(TBOOL);

	s = pkglookup("true", builtinpkg);
	s->def = nodbool(1);
	s->def->sym = lookup("true");
	s->def->type = idealbool;

	s = pkglookup("false", builtinpkg);
	s->def = nodbool(0);
	s->def->sym = lookup("false");
	s->def->type = idealbool;

	s = lookup("_");
	s->block = -100;
	s->def = nod(ONAME, N, N);
	s->def->sym = s;
	types[TBLANK] = typ(TBLANK);
	s->def->type = types[TBLANK];
	nblank = s->def;

	s = pkglookup("_", builtinpkg);
	s->block = -100;
	s->def = nod(ONAME, N, N);
	s->def->sym = s;
	types[TBLANK] = typ(TBLANK);
	s->def->type = types[TBLANK];

	types[TNIL] = typ(TNIL);
	s = pkglookup("nil", builtinpkg);
	v.ctype = CTNIL;
	s->def = nodlit(v);
	s->def->sym = s;
}

// caller:
// 	1. main() 只有这一处
void lexinit1(void)
{
	Sym *s, *s1;
	Type *t, *f, *rcvr, *in, *out;

	// t = interface { Error() string }
	rcvr = typ(TSTRUCT);
	rcvr->type = typ(TFIELD);
	rcvr->type->type = ptrto(typ(TSTRUCT));
	rcvr->funarg = 1;
	in = typ(TSTRUCT);
	in->funarg = 1;
	out = typ(TSTRUCT);
	out->type = typ(TFIELD);
	out->type->type = types[TSTRING];
	out->funarg = 1;
	f = typ(TFUNC);
	*getthis(f) = rcvr;
	*getoutarg(f) = out;
	*getinarg(f) = in;
	f->thistuple = 1;
	f->intuple = 0;
	f->outnamed = 0;
	f->outtuple = 1;
	t = typ(TINTER);
	t->type = typ(TFIELD);
	t->type->sym = lookup("Error");
	t->type->type = f;

	// error type
	s = lookup("error");
	s->lexical = LNAME;
	s1 = pkglookup("error", builtinpkg);
	errortype = t;
	errortype->sym = s1;
	s1->lexical = LNAME;
	s1->def = typenod(errortype);

	// byte alias
	s = lookup("byte");
	s->lexical = LNAME;
	s1 = pkglookup("byte", builtinpkg);
	bytetype = typ(TUINT8);
	bytetype->sym = s1;
	s1->lexical = LNAME;
	s1->def = typenod(bytetype);

	// rune alias
	s = lookup("rune");
	s->lexical = LNAME;
	s1 = pkglookup("rune", builtinpkg);
	runetype = typ(TINT32);
	runetype->sym = s1;
	s1->lexical = LNAME;
	s1->def = typenod(runetype);
}

// caller:
// 	1. main() 只有这一处
void lexfini(void)
{
	Sym *s;
	int lex, etype, i;
	Val v;

	for(i=0; i<nelem(syms); i++) {
		lex = syms[i].lexical;
		if(lex != LNAME) {
			continue;
		}
		s = lookup(syms[i].name);
		s->lexical = lex;

		etype = syms[i].etype;
		if(etype != Txxx && (etype != TANY || debug['A']) && s->def == N) {
			s->def = typenod(types[etype]);
			s->origpkg = builtinpkg;
		}

		etype = syms[i].op;
		if(etype != OXXX && s->def == N) {
			s->def = nod(ONAME, N, N);
			s->def->sym = s;
			s->def->etype = etype;
			s->def->builtin = 1;
			s->origpkg = builtinpkg;
		}
	}

	// backend-specific builtin types (e.g. int).
	for(i=0; typedefs[i].name; i++) {
		s = lookup(typedefs[i].name);
		if(s->def == N) {
			s->def = typenod(types[typedefs[i].etype]);
			s->origpkg = builtinpkg;
		}
	}

	// there's only so much table-driven we can handle.
	// these are special cases.
	s = lookup("byte");
	if(s->def == N) {
		s->def = typenod(bytetype);
		s->origpkg = builtinpkg;
	}

	s = lookup("error");
	if(s->def == N) {
		s->def = typenod(errortype);
		s->origpkg = builtinpkg;
	}

	s = lookup("rune");
	if(s->def == N) {
		s->def = typenod(runetype);
		s->origpkg = builtinpkg;
	}

	s = lookup("nil");
	if(s->def == N) {
		v.ctype = CTNIL;
		s->def = nodlit(v);
		s->def->sym = s;
		s->origpkg = builtinpkg;
	}

	s = lookup("iota");
	if(s->def == N) {
		s->def = nod(OIOTA, N, N);
		s->def->sym = s;
		s->origpkg = builtinpkg;
	}

	s = lookup("true");
	if(s->def == N) {
		s->def = nodbool(1);
		s->def->sym = s;
		s->origpkg = builtinpkg;
	}

	s = lookup("false");
	if(s->def == N) {
		s->def = nodbool(0);
		s->def->sym = s;
		s->origpkg = builtinpkg;
	}

	nodfp = nod(ONAME, N, N);
	nodfp->type = types[TINT32];
	nodfp->xoffset = 0;
	nodfp->class = PPARAM;
	nodfp->sym = lookup(".fp");
}
