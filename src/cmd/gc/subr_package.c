#include	<u.h>
#include	<libc.h>
#include	"go.h"
#include	"y.tab.h"

Sym* lookup(char *name)
{
	return pkglookup(name, localpkg);
}

Sym* pkglookup(char *name, Pkg *pkg)
{
	Sym *s;
	uint32 h;
	int c;

	h = stringhash(name) % NHASH;
	c = name[0];
	for(s = hash[h]; s != S; s = s->link) {
		if(s->name[0] != c || s->pkg != pkg) {
			continue;
		}
		if(strcmp(s->name, name) == 0) {
			return s;
		}
	}
	// 没找到, 则新建一个再返回
	s = mal(sizeof(*s));
	s->name = mal(strlen(name)+1);
	strcpy(s->name, name);

	s->pkg = pkg;

	s->link = hash[h];
	hash[h] = s;
	s->lexical = LNAME;

	return s;
}

Sym* restrictlookup(char *name, Pkg *pkg)
{
	if(!exportname(name) && pkg != localpkg)
		yyerror("cannot refer to unexported name %s.%s", pkg->name, name);
	return pkglookup(name, pkg);
}

// importdot 遍历目标 opkg 包中所有 sym 对象, 找到其中所有的可导出对象,
// 并将ta们加载到当前 package 中.
//
// find all the exported symbols in package opkg
// and make them available in the current package
void importdot(Pkg *opkg, Node *pack)
{
	Sym *s, *s1;
	uint32 h;
	int n;
	char *pkgerror;

	// n 表示目标 package opkg 中的对象被引用的次数
	n = 0;
	for(h=0; h<NHASH; h++) {
		for(s = hash[h]; s != S; s = s->link) {
			if(s->pkg != opkg) {
				continue;
			}
			if(s->def == N) {
				continue;
			}
			// 0xb7 = center dot
			if(!exportname(s->name) || utfrune(s->name, 0xb7)) {
				continue;
			}
			s1 = lookup(s->name);
			if(s1->def != N) {
				pkgerror = smprint("during import \"%Z\"", opkg->path);
				redeclare(s1, pkgerror);
				continue;
			}
			s1->def = s->def;
			s1->block = s->block;
			s1->def->pack = pack;
			s1->origpkg = opkg;
			n++;
		}
	}
	if(n == 0) {
		// can't possibly be used - there were no symbols
		yyerrorl(pack->lineno, "imported and not used: \"%Z\"", opkg->path);
	}
}

/*
 * Convert raw string to the prefix that will be used in the symbol table. 
 * All control characters, space, '%' and '"', as well as
 * non-7-bit clean bytes turn into %xx.  The period needs escaping
 * only in the last segment of the path, and it makes for happier
 * users if we escape that as little as possible.
 *
 * If you edit this, edit ../ld/lib.c:/^pathtoprefix copy too.
 */
static char* pathtoprefix(char *s)
{
	static char hex[] = "0123456789abcdef";
	char *p, *r, *w, *l;
	int n;

	// find first character past the last slash, if any.
	l = s;
	for(r=s; *r; r++)
		if(*r == '/')
			l = r+1;

	// check for chars that need escaping
	n = 0;
	for(r=s; *r; r++)
		if(*r <= ' ' || (*r == '.' && r >= l) || *r == '%' || *r == '"' || *r >= 0x7f)
			n++;

	// quick exit
	if(n == 0)
		return s;

	// escape
	p = mal((r-s)+1+2*n);
	for(r=s, w=p; *r; r++) {
		if(*r <= ' ' || (*r == '.' && r >= l) || *r == '%' || *r == '"' || *r >= 0x7f) {
			*w++ = '%';
			*w++ = hex[(*r>>4)&0xF];
			*w++ = hex[*r&0xF];
		} else
			*w++ = *r;
	}
	*w = '\0';
	return p;
}

// 将目标 path 所表示的 package 添加到 phash 数组中.
//
// caller:
// 	1. src/cmd/gc/lex.c -> main() 6g 命令编译初期, 加载内置的, 已知的 package
// 	2. src/cmd/gc/lex.c -> importfile() 按 path 路径加载第3方的 package
Pkg* mkpkg(Strlit *path)
{
	Pkg *p;
	int h;

	h = stringhash(path->s) & (nelem(phash)-1);
	for(p=phash[h]; p; p=p->link) {
		if(p->path->len == path->len && memcmp(path->s, p->path->s, path->len) == 0) {
			return p;
		}
	}

	p = mal(sizeof *p);
	p->path = path;
	p->prefix = pathtoprefix(path->s);
	p->link = phash[h];
	phash[h] = p;
	return p;
}

static char* reservedimports[] = {
	"go",
	"type",
};

int isbadimport(Strlit *path)
{
	int i;
	char *s;
	Rune r;

	if(strlen(path->s) != path->len) {
		yyerror("import path contains NUL");
		return 1;
	}
	
	for(i=0; i<nelem(reservedimports); i++) {
		if(strcmp(path->s, reservedimports[i]) == 0) {
			yyerror("import path \"%s\" is reserved and cannot be used", path->s);
			return 1;
		}
	}

	s = path->s;
	while(*s) {
		s += chartorune(&r, s);
		if(r == Runeerror) {
			yyerror("import path contains invalid UTF-8 sequence: \"%Z\"", path);
			return 1;
		}
		if(r < 0x20 || r == 0x7f) {
			yyerror("import path contains control character: \"%Z\"", path);
			return 1;
		}
		if(r == '\\') {
			yyerror("import path contains backslash; use slash: \"%Z\"", path);
			return 1;
		}
		if(isspacerune(r)) {
			yyerror("import path contains space character: \"%Z\"", path);
			return 1;
		}
		if(utfrune("!\"#$%&'()*,:;<=>?[]^`{|}", r)) {
			yyerror("import path contains invalid character '%C': \"%Z\"", r, path);
			return 1;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////


Type* shallow(Type *t)
{
	Type *nt;

	if(t == T)
		return T;
	nt = typ(0);
	*nt = *t;
	if(t->orig == t)
		nt->orig = nt;
	return nt;
}

static Type* deep(Type *t)
{
	Type *nt, *xt;

	if(t == T)
		return T;

	switch(t->etype) {
	default:
		nt = t;	// share from here down
		break;

	case TANY:
		nt = shallow(t);
		nt->copyany = 1;
		break;

	case TPTR32:
	case TPTR64:
	case TCHAN:
	case TARRAY:
		nt = shallow(t);
		nt->type = deep(t->type);
		break;

	case TMAP:
		nt = shallow(t);
		nt->down = deep(t->down);
		nt->type = deep(t->type);
		break;

	case TFUNC:
		nt = shallow(t);
		nt->type = deep(t->type);
		nt->type->down = deep(t->type->down);
		nt->type->down->down = deep(t->type->down->down);
		break;

	case TSTRUCT:
		nt = shallow(t);
		nt->type = shallow(t->type);
		xt = nt->type;

		for(t=t->type; t!=T; t=t->down) {
			xt->type = deep(t->type);
			xt->down = shallow(t->down);
			xt = xt->down;
		}
		break;
	}
	return nt;
}

Node* syslook(char *name, int copy)
{
	Sym *s;
	Node *n;

	s = pkglookup(name, runtimepkg);
	if(s == S || s->def == N)
		fatal("syslook: can't find runtime.%s", name);

	if(!copy)
		return s->def;

	n = nod(0, N, N);
	*n = *s->def;
	n->type = deep(s->def->type);

	return n;
}
