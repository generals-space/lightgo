#include	<u.h>

#include	<libc.h>

#include	"go.h"
#include	"lex.h"
#include	"y.tab.h"

// caller:
// 	1. _yylex() 只有这一处
int isfrog(int c)
{
	// complain about possibly invisible control characters
	if(c < ' ') {
		return !yy_isspace(c);	// exclude good white space
	}
	if(0x7f <= c && c <= 0xa0)	// DEL, unicode block including unbreakable space.
		return 1;
	return 0;
}

// caller:
// 	1. _yylex() 只有这一处
//
/*
 * read and interpret syntax that looks like
 * //line parse.y:15
 * as a discontinuity in sequential line numbers.
 * the next line of input comes from parse.y:15
 */
int getlinepragma(void)
{
	int i, c, n;
	char *cp, *ep, *linep;
	Hist *h;

	c = getr();
	if(c == 'g')
		goto go;
	if(c != 'l')	
		goto out;
	for(i=1; i<5; i++) {
		c = getr();
		if(c != "line "[i])
			goto out;
	}

	cp = lexbuf;
	ep = lexbuf+sizeof(lexbuf)-5;
	linep = nil;
	for(;;) {
		c = getr();
		if(c == EOF)
			goto out;
		if(c == '\n')
			break;
		if(c == ' ')
			continue;
		if(c == ':')
			linep = cp;
		if(cp < ep)
			*cp++ = c;
	}
	*cp = 0;

	if(linep == nil || linep >= ep)
		goto out;
	*linep++ = '\0';
	n = 0;
	for(cp=linep; *cp; cp++) {
		if(*cp < '0' || *cp > '9')
			goto out;
		n = n*10 + *cp - '0';
		if(n > 1e8) {
			yyerror("line number out of range");
			errorexit();
		}
	}
	if(n <= 0)
		goto out;

	// try to avoid allocating file name over and over
	for(h=hist; h!=H; h=h->link) {
		if(h->name != nil && strcmp(h->name, lexbuf) == 0) {
			linehist(h->name, n, 0);
			goto out;
		}
	}
	linehist(strdup(lexbuf), n, 0);
	goto out;

go:
	cp = lexbuf;
	ep = lexbuf+sizeof(lexbuf)-5;
	*cp++ = 'g'; // already read
	for(;;) {
		c = getr();
		if(c == EOF || c >= Runeself)
			goto out;
		if(c == '\n')
			break;
		if(cp < ep)
			*cp++ = c;
	}
	*cp = 0;
	ep = strchr(lexbuf, ' ');
	if(ep != nil)
		*ep = 0;

	if(strcmp(lexbuf, "go:nointerface") == 0 && fieldtrack_enabled) {
		nointerface = 1;
		goto out;
	}
	if(strcmp(lexbuf, "go:noescape") == 0) {
		noescape = 1;
		goto out;
	}
	
out:
	return c;
}

int getc(void)
{
	int c, c1, c2;

	c = curio.peekc;
	if(c != 0) {
		curio.peekc = curio.peekc1;
		curio.peekc1 = 0;
		goto check;
	}
	
	if(curio.bin == nil) {
		c = *curio.cp & 0xff;
		if(c != 0)
			curio.cp++;
	} else {
	loop:
		c = BGETC(curio.bin);
		if(c == 0xef) {
			c1 = BGETC(curio.bin);
			c2 = BGETC(curio.bin);
			if(c1 == 0xbb && c2 == 0xbf) {
				yyerrorl(lexlineno, "Unicode (UTF-8) BOM in middle of file");
				goto loop;
			}
			Bungetc(curio.bin);
			Bungetc(curio.bin);
		}
	}

check:
	switch(c) {
	case 0:
		if(curio.bin != nil) {
			yyerror("illegal NUL byte");
			break;
		}
	case EOF:
		// insert \n at EOF
		if(curio.eofnl || curio.last == '\n')
			return EOF;
		curio.eofnl = 1;
		c = '\n';
	case '\n':
		if(pushedio.bin == nil)
			lexlineno++;
		break;
	}
	curio.last = c;
	return c;
}

void ungetc(int c)
{
	curio.peekc1 = curio.peekc;
	curio.peekc = c;
	if(c == '\n' && pushedio.bin == nil) {
		lexlineno--;
	}
}

// caller:
// 	1. _yylex()
// 	2. getlinepragma()
// 	3. escchar()
int32 getr(void)
{
	int c, i;
	char str[UTFmax+1];
	Rune rune;

	c = getc();
	if(c < Runeself)
		return c;
	i = 0;
	str[i++] = c;

loop:
	c = getc();
	str[i++] = c;
	if(!fullrune(str, i))
		goto loop;
	c = chartorune(&rune, str);
	if(rune == Runeerror && c == 1) {
		lineno = lexlineno;
		yyerror("illegal UTF-8 sequence");
		flusherrors();
		print("\t");
		for(c=0; c<i; c++)
			print("%s%.2x", c > 0 ? " " : "", *(uchar*)(str+c));
		print("\n");
	}
	return rune;
}

// caller:
// 	1. _yylex() 只有这一处
int escchar(int e, int *escflg, vlong *val)
{
	int i, u, c;
	vlong l;

	*escflg = 0;

	c = getr();
	switch(c) {
	case EOF:
		yyerror("eof in string");
		return 1;
	case '\n':
		yyerror("newline in string");
		return 1;
	case '\\':
		break;
	default:
		if(c == e)
			return 1;
		*val = c;
		return 0;
	}

	u = 0;
	c = getr();
	switch(c) {
	case 'x':
		*escflg = 1;	// it's a byte
		i = 2;
		goto hex;

	case 'u':
		i = 4;
		u = 1;
		goto hex;

	case 'U':
		i = 8;
		u = 1;
		goto hex;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		*escflg = 1;	// it's a byte
		goto oct;

	case 'a': c = '\a'; break;
	case 'b': c = '\b'; break;
	case 'f': c = '\f'; break;
	case 'n': c = '\n'; break;
	case 'r': c = '\r'; break;
	case 't': c = '\t'; break;
	case 'v': c = '\v'; break;
	case '\\': c = '\\'; break;

	default:
		if(c != e) {
			yyerror("unknown escape sequence: %c", c);
		}
	}
	*val = c;
	return 0;

hex:
{
	l = 0;
	for(; i>0; i--) {
		c = getc();
		if(c >= '0' && c <= '9') {
			l = l*16 + c-'0';
			continue;
		}
		if(c >= 'a' && c <= 'f') {
			l = l*16 + c-'a' + 10;
			continue;
		}
		if(c >= 'A' && c <= 'F') {
			l = l*16 + c-'A' + 10;
			continue;
		}
		yyerror("non-hex character in escape sequence: %c", c);
		ungetc(c);
		break;
	}
	if(u && (l > Runemax || (0xd800 <= l && l < 0xe000))) {
		yyerror("invalid Unicode code point in escape sequence: %#llx", l);
		l = Runeerror;
	}
	*val = l;
	return 0;
}

oct:
{
	l = c - '0';
	for(i=2; i>0; i--) {
		c = getc();
		if(c >= '0' && c <= '7') {
			l = l*8 + c-'0';
			continue;
		}
		yyerror("non-octal character in escape sequence: %c", c);
		ungetc(c);
	}
	if(l > 255) {
		yyerror("octal escape value > 255: %d", l);
	}

	*val = l;
	return 0;
}
}

struct
{
	int	lex;
	char*	name;
} lexn[] =
{
	LANDAND,	"ANDAND",
	LASOP,		"ASOP",
	LBREAK,		"BREAK",
	LCASE,		"CASE",
	LCHAN,		"CHAN",
	LCOLAS,		"COLAS",
	LCONST,		"CONST",
	LCONTINUE,	"CONTINUE",
	LDEC,		"DEC",
	LDEFER,		"DEFER",
	LELSE,		"ELSE",
	LEQ,		"EQ",
	LFALL,		"FALL",
	LFOR,		"FOR",
	LFUNC,		"FUNC",
	LGE,		"GE",
	LGO,		"GO",
	LGOTO,		"GOTO",
	LGT,		"GT",
	LIF,		"IF",
	LIMPORT,	"IMPORT",
	LINC,		"INC",
	LINTERFACE,	"INTERFACE",
	LLE,		"LE",
	LLITERAL,	"LITERAL",
	LLSH,		"LSH",
	LLT,		"LT",
	LMAP,		"MAP",
	LNAME,		"NAME",
	LNE,		"NE",
	LOROR,		"OROR",
	LPACKAGE,	"PACKAGE",
	LRANGE,		"RANGE",
	LRETURN,	"RETURN",
	LRSH,		"RSH",
	LSTRUCT,	"STRUCT",
	LSWITCH,	"SWITCH",
	LTYPE,		"TYPE",
	LVAR,		"VAR",
};

// caller:
// 	1. _yylex() 只有这一处
char* lexname(int lex)
{
	int i;
	static char buf[100];

	for(i=0; i<nelem(lexn); i++) {
		if(lexn[i].lex == lex) {
			return lexn[i].name;
		}
	}
	snprint(buf, sizeof(buf), "LEX-%d", lex);
	return buf;
}
