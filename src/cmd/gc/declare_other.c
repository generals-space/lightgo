#include	<u.h>
#include	<libc.h>

#include	"go.h"

// caller:
// 	1. src/cmd/gc/init.c -> fninit()
// 	2. src/cmd/gc/sinit.c -> staticname()
void addvar(Node *n, Type *t, int ctxt)
{
	if(n==N || n->sym == S || (n->op != ONAME && n->op != ONONAME) || t == T) {
		fatal("addvar: n=%N t=%T nil", n, t);
	}

	n->op = ONAME;
	declare(n, ctxt);
	n->type = t;
}

// 	@param isclosure: 从目前来看, 好像所有主调函数传入的都是 0 啊...
//
// caller:
// 	1. src/cmd/gc/lex.c -> main()
// 	2. src/cmd/gc/init.c -> fninit()
void funccompile(Node *n, int isclosure)
{
	stksize = BADWIDTH;
	maxarg = 0;

	if(n->type == T) {
		if(nerrors == 0) {
			fatal("funccompile missing type");
		}
		return;
	}

	// assign parameter offsets
	checkwidth(n->type);

	// record offset to actual frame pointer.
	// for closure, have to skip over leading pointers and PC slot.
	nodfp->xoffset = 0;
	if(isclosure) {
		NodeList *l;
		for(l=n->nname->ntype->list; l; l=l->next) {
			nodfp->xoffset += widthptr;
			if(l->n->left == N)	// found slot for PC
				break;
		}
	}

	if(curfn) {
		fatal("funccompile %S inside %S", n->nname->sym, curfn->nname->sym);
	}

	stksize = 0;
	dclcontext = PAUTO;
	funcdepth = n->funcdepth + 1;
	compile(n);
	curfn = nil;
	funcdepth = 0;
	dclcontext = PEXTERN;
}

// caller:
// 	1. src/cmd/6g/gsubr.c -> naddr{} 只有这一处
Sym* funcsym(Sym *s)
{
	char *p;
	Sym *s1;
	
	p = smprint("%s·f", s->name);
	s1 = pkglookup(p, s->pkg);
	free(p);
	if(s1->def == N) {
		s1->def = newname(s1);
		s1->def->shortname = newname(s);
		funcsyms = list(funcsyms, s1->def);
	}
	return s1;
}
