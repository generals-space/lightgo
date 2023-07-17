#include	<u.h>
#include	<libc.h>

#include	"go.h"

struct
{
	char *have;
	char *want;
} yytfix[] =
{
	"$end",	"EOF",
	"LLITERAL",	"literal",
	"LASOP",	"op=",
	"LBREAK",	"break",
	"LCASE",	"case",
	"LCOLAS",	":=",
	"LCONST",	"const",
	"LCONTINUE",	"continue",
	"LDDD",	"...",
	"LDEFAULT",	"default",
	"LDEFER",	"defer",
	"LELSE",	"else",
	"LFALL",	"fallthrough",
	"LFOR",	"for",
	"LFUNC",	"func",
	"LGO",	"go",
	"LGOTO",	"goto",
	"LIF",	"if",
	"LIMPORT",	"import",
	"LINTERFACE",	"interface",
	"LMAP",	"map",
	"LNAME",	"name",
	"LPACKAGE",	"package",
	"LRANGE",	"range",
	"LRETURN",	"return",
	"LSELECT",	"select",
	"LSTRUCT",	"struct",
	"LSWITCH",	"switch",
	"LTYPE",	"type",
	"LVAR",	"var",
	"LANDAND",	"&&",
	"LANDNOT",	"&^",
	"LBODY",	"{",
	"LCOMM",	"<-",
	"LDEC",	"--",
	"LINC",	"++",
	"LEQ",	"==",
	"LGE",	">=",
	"LGT",	">",
	"LLE",	"<=",
	"LLT",	"<",
	"LLSH",	"<<",
	"LRSH",	">>",
	"LOROR",	"||",
	"LNE",	"!=",
	
	// spell out to avoid confusion with punctuation in error messages
	"';'",	"semicolon or newline",
	"','",	"comma",
};

// caller:
// 	1. main() 只有这一处
void yytinit(void)
{
	int i, j;
	extern char *yytname[];
	char *s, *t;

	for(i=0; yytname[i] != nil; i++) {
		s = yytname[i];
		
		if(strcmp(s, "LLITERAL") == 0) {
			strcpy(litbuf, "literal");
			yytname[i] = litbuf;
			goto loop;
		}
		
		// apply yytfix if possible
		for(j=0; j<nelem(yytfix); j++) {
			if(strcmp(s, yytfix[j].have) == 0) {
				yytname[i] = yytfix[j].want;
				goto loop;
			}
		}

		// turn 'x' into x.
		if(s[0] == '\'') {
			t = strdup(s+1);
			t[strlen(t)-1] = '\0';
			yytname[i] = t;
		}
	loop:;
	}
}
