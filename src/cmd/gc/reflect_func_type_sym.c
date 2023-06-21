#include "reflect.h"

Sym* typesym(Type *t)
{
	char *p;
	Sym *s;

	p = smprint("%-T", t);
	s = pkglookup(p, typepkg);
	//print("typesym: %s -> %+S\n", p, s);
	free(p);
	return s;
}

Sym* tracksym(Type *t)
{
	char *p;
	Sym *s;

	p = smprint("%-T.%s", t->outer, t->sym->name);
	s = pkglookup(p, trackpkg);
	free(p);
	return s;
}

Sym* typelinksym(Type *t)
{
	char *p;
	Sym *s;

	// %-uT is what the generated Type's string field says.
	// It uses (ambiguous) package names instead of import paths.
	// %-T is the complete, unambiguous type name.
	// We want the types to end up sorted by string field,
	// so use that first in the name, and then add :%-T to
	// disambiguate. The names are a little long but they are
	// discarded by the linker and do not end up in the symbol
	// table of the final binary.
	p = smprint("%-uT/%-T", t, t);
	s = pkglookup(p, typelinkpkg);
	//print("typelinksym: %s -> %+S\n", p, s);
	free(p);
	return s;
}

Sym* typesymprefix(char *prefix, Type *t)
{
	char *p;
	Sym *s;

	p = smprint("%s.%-T", prefix, t);
	s = pkglookup(p, typepkg);
	//print("algsym: %s -> %+S\n", p, s);
	free(p);
	return s;
}

////////////////////////////////////////////////////////////////////////////////

Sym* typenamesym(Type *t)
{
	Sym *s;
	Node *n;

	if(t == T || (isptr[t->etype] && t->type == T) || isideal(t))
		fatal("typename %T", t);
	s = typesym(t);
	if(s->def == N) {
		n = nod(ONAME, N, N);
		n->sym = s;
		n->type = types[TUINT8];
		n->addable = 1;
		n->ullman = 1;
		n->class = PEXTERN;
		n->xoffset = 0;
		n->typecheck = 1;
		s->def = n;

		signatlist = list(signatlist, typenod(t));
	}
	return s->def->sym;
}

Node* typename(Type *t)
{
	Sym *s;
	Node *n;

	s = typenamesym(t);
	n = nod(OADDR, s->def, N);
	n->type = ptrto(s->def->type);
	n->addable = 1;
	n->ullman = 2;
	n->typecheck = 1;
	return n;
}