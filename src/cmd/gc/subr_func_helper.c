#include	<u.h>
#include	<libc.h>
#include	"go.h"

void errorexit(void)
{
	flusherrors();
	if(outfile)
		remove(outfile);
	exits("error");
}

// strlit 初始化一个 Strlit 字符串字面量对象, 把 s 的内容和长度赋值给该对象中的成员.
Strlit* strlit(char *s)
{
	Strlit *t;

	t = mal(sizeof *t + strlen(s));
	strcpy(t->s, s);
	t->len = strlen(s);
	return t;
}

// 初始化一个类型为 et 的 Type 对象
//
// 	@param et: 取自 src/cmd/gc/go.h -> Txxx 所在的枚举列表中的值, 可以表示 int, bool, func, struct 等.
Type* typ(int et)
{
	Type *t;

	t = mal(sizeof(*t));
	t->etype = et;
	t->width = BADWIDTH;
	t->lineno = lineno;
	t->orig = t;
	return t;
}

Type* ptrto(Type *t)
{
	Type *t1;

	if(tptr == 0) {
		fatal("ptrto: no tptr");
	}
	t1 = typ(tptr);
	t1->type = t;
	t1->width = widthptr;
	t1->align = widthptr;
	return t1;
}

// caller:
//  1. genhash()
//  2. geneq()
//
// ispaddedfield reports whether the given field
// is followed by padding. For the case where t is
// the last field, total gives the size of the enclosing struct.
int ispaddedfield(Type *t, vlong total)
{
	if(t->etype != TFIELD)
		fatal("ispaddedfield called non-field %T", t);
	if(t->down == T)
		return t->width + t->type->width != total;
	return t->width + t->type->width != t->down->width;
}

////////////////////////////////////////////////////////////////////////////////

extern int yychar;
int parserline(void)
{
	// parser has one symbol lookahead
	if(yychar != 0 && yychar != -2) {
		return prevlineno;
	}
	return lineno;
}

// caller:
// 	1. src/cmd/gc/lex.c -> main()
void linehist(char *file, int32 off, int relative)
{
	Hist *h;
	char *cp;

	if(debug['i']) {
		if(file != nil) {
			if(off < 0)
				print("pragma %s", file);
			else
			if(off > 0)
				print("line %s", file);
			else
				print("import %s", file);
		} else
			print("end of import");
		print(" at line %L\n", lexlineno);
	}

	if(off < 0 && file[0] != '/' && !relative) {
		cp = mal(strlen(file) + strlen(pathname) + 2);
		sprint(cp, "%s/%s", pathname, file);
		file = cp;
	}

	h = mal(sizeof(Hist));
	h->name = file;
	h->line = lexlineno;
	h->offset = off;
	h->link = H;
	if(ehist == H) {
		hist = h;
		ehist = h;
		return;
	}
	ehist->link = h;
	ehist = h;
}

int32 setlineno(Node *n)
{
	int32 lno;

	lno = lineno;
	if(n != N)
	switch(n->op) {
	case ONAME:
	case OTYPE:
	case OPACK:
	case OLITERAL:
		break;
	default:
		lineno = n->lineno;
		if(lineno == 0) {
			if(debug['K'])
				warn("setlineno: line 0");
			lineno = lno;
		}
	}
	return lno;
}

////////////////////////////////////////////////////////////////////////////////

uint32 stringhash(char *p)
{
	uint32 h;
	int c;

	h = 0;
	for(;;) {
		c = *p++;
		if(c == 0) {
			break;
		}
		h = h*PRIME1 + c;
	}

	if((int32)h < 0) {
		h = -h;
		if((int32)h < 0) {
			h = 0;
		}
	}
	return h;
}
