#include "reflect.h"

void dimportpath(Pkg *p)
{
	static Pkg *gopkg;
	char *nam;
	Node *n;

	if(p->pathsym != S) {
		return;
	}

	if(gopkg == nil) {
		gopkg = mkpkg(strlit("go"));
		gopkg->name = "go";
	}
	nam = smprint("importpath.%s.", p->prefix);

	n = nod(ONAME, N, N);
	n->sym = pkglookup(nam, gopkg);
	free(nam);
	n->class = PEXTERN;
	n->xoffset = 0;
	p->pathsym = n->sym;

	gdatastring(n, p->path);
	ggloblsym(n->sym, types[TSTRING]->width, 1, 1);
}

int dgopkgpath(Sym *s, int ot, Pkg *pkg)
{
	if(pkg == nil)
		return dgostringptr(s, ot, nil);

	// Emit reference to go.importpath.""., which 6l will
	// rewrite using the correct import path.  Every package
	// that imports this one directly defines the symbol.
	if(pkg == localpkg) {
		static Sym *ns;

		if(ns == nil)
			ns = pkglookup("importpath.\"\".", mkpkg(strlit("go")));
		return dsymptr(s, ot, ns, 0);
	}

	dimportpath(pkg);
	return dsymptr(s, ot, pkg->pathsym, 0);
}
