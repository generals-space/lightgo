#include	<u.h>
#include	<libc.h>
#include	"go.h"
#include	"lex.h"
#include	"y.tab.h"

#define	DBG	if(!debug['x']){}else print

typedef struct Loophack Loophack;
struct Loophack {
	int v;
	Loophack *next;
};

// caller:
// 	1. yylex() 只有这一处
static int32 _yylex(void)
{
	int c, c1, clen, escflag, ncp;
	vlong v;
	char *cp, *ep;
	Rune rune;
	Sym *s;
	static Loophack *lstk;
	Loophack *h;

	prevlineno = lineno;

l0:
	c = getc();
	if(yy_isspace(c)) {
		if(c == '\n' && curio.nlsemi) {
			ungetc(c);
			DBG("lex: implicit semi\n");
			return ';';
		}
		goto l0;
	}

	lineno = lexlineno;	/* start of token */

	if(c >= Runeself) {
		/* all multibyte runes are alpha */
		cp = lexbuf;
		ep = lexbuf+sizeof lexbuf;
		goto talph;
	}

	if(yy_isalpha(c)) {
		cp = lexbuf;
		ep = lexbuf+sizeof lexbuf;
		goto talph;
	}

	if(yy_isdigit(c)) {
		goto tnum;
	}

	switch(c) {
	case EOF:
		lineno = prevlineno;
		ungetc(EOF);
		return -1;

	case '_':
		cp = lexbuf;
		ep = lexbuf+sizeof lexbuf;
		goto talph;

	case '.':
		c1 = getc();
		if(yy_isdigit(c1)) {
			cp = lexbuf;
			ep = lexbuf+sizeof lexbuf;
			*cp++ = c;
			c = c1;
			goto casedot;
		}
		if(c1 == '.') {
			c1 = getc();
			if(c1 == '.') {
				c = LDDD;
				goto lx;
			}
			ungetc(c1);
			c1 = '.';
		}
		break;

	case '"':
		/* "..." */
		strcpy(lexbuf, "\"<string>\"");
		cp = mal(8);
		clen = sizeof(int32);
		ncp = 8;

		for(;;) {
			if(clen+UTFmax > ncp) {
				cp = remal(cp, ncp, ncp);
				ncp += ncp;
			}
			if(escchar('"', &escflag, &v))
				break;
			if(v < Runeself || escflag) {
				cp[clen++] = v;
			} else {
				rune = v;
				c = runelen(rune);
				runetochar(cp+clen, &rune);
				clen += c;
			}
		}
		goto strlit;
	
	case '`':
		/* `...` */
		strcpy(lexbuf, "`<string>`");
		cp = mal(8);
		clen = sizeof(int32);
		ncp = 8;

		for(;;) {
			if(clen+UTFmax > ncp) {
				cp = remal(cp, ncp, ncp);
				ncp += ncp;
			}
			c = getr();
			if(c == '\r')
				continue;
			if(c == EOF) {
				yyerror("eof in string");
				break;
			}
			if(c == '`')
				break;
			rune = c;
			clen += runetochar(cp+clen, &rune);
		}

	strlit:
		*(int32*)cp = clen-sizeof(int32);	// length
		do {
			cp[clen++] = 0;
		} while(clen & MAXALIGN);
		yylval.val.u.sval = (Strlit*)cp;
		yylval.val.ctype = CTSTR;
		DBG("lex: string literal\n");
		strcpy(litbuf, "string literal");
		return LLITERAL;

	case '\'':
		/* '.' */
		if(escchar('\'', &escflag, &v)) {
			yyerror("empty character literal or unescaped ' in character literal");
			v = '\'';
		}
		if(!escchar('\'', &escflag, &v)) {
			yyerror("missing '");
			ungetc(v);
		}
		yylval.val.u.xval = mal(sizeof(*yylval.val.u.xval));
		mpmovecfix(yylval.val.u.xval, v);
		yylval.val.ctype = CTRUNE;
		DBG("lex: codepoint literal\n");
		strcpy(litbuf, "string literal");
		return LLITERAL;

	case '/':
		c1 = getc();
		if(c1 == '*') {
			int nl;
			
			nl = 0;
			for(;;) {
				c = getr();
				if(c == '\n')
					nl = 1;
				while(c == '*') {
					c = getr();
					if(c == '/') {
						if(nl)
							ungetc('\n');
						goto l0;
					}
					if(c == '\n')
						nl = 1;
				}
				if(c == EOF) {
					yyerror("eof in comment");
					errorexit();
				}
			}
		}
		if(c1 == '/') {
			c = getlinepragma();
			for(;;) {
				if(c == '\n' || c == EOF) {
					ungetc(c);
					goto l0;
				}
				c = getr();
			}
		}
		if(c1 == '=') {
			c = ODIV;
			goto asop;
		}
		break;

	case ':':
		c1 = getc();
		if(c1 == '=') {
			c = LCOLAS;
			yylval.i = lexlineno;
			goto lx;
		}
		break;

	case '*':
		c1 = getc();
		if(c1 == '=') {
			c = OMUL;
			goto asop;
		}
		break;

	case '%':
		c1 = getc();
		if(c1 == '=') {
			c = OMOD;
			goto asop;
		}
		break;

	case '+':
		c1 = getc();
		if(c1 == '+') {
			c = LINC;
			goto lx;
		}
		if(c1 == '=') {
			c = OADD;
			goto asop;
		}
		break;

	case '-':
		c1 = getc();
		if(c1 == '-') {
			c = LDEC;
			goto lx;
		}
		if(c1 == '=') {
			c = OSUB;
			goto asop;
		}
		break;

	case '>':
		c1 = getc();
		if(c1 == '>') {
			c = LRSH;
			c1 = getc();
			if(c1 == '=') {
				c = ORSH;
				goto asop;
			}
			break;
		}
		if(c1 == '=') {
			c = LGE;
			goto lx;
		}
		c = LGT;
		break;

	case '<':
		c1 = getc();
		if(c1 == '<') {
			c = LLSH;
			c1 = getc();
			if(c1 == '=') {
				c = OLSH;
				goto asop;
			}
			break;
		}
		if(c1 == '=') {
			c = LLE;
			goto lx;
		}
		if(c1 == '-') {
			c = LCOMM;
			goto lx;
		}
		c = LLT;
		break;

	case '=':
		c1 = getc();
		if(c1 == '=') {
			c = LEQ;
			goto lx;
		}
		break;

	case '!':
		c1 = getc();
		if(c1 == '=') {
			c = LNE;
			goto lx;
		}
		break;

	case '&':
		c1 = getc();
		if(c1 == '&') {
			c = LANDAND;
			goto lx;
		}
		if(c1 == '^') {
			c = LANDNOT;
			c1 = getc();
			if(c1 == '=') {
				c = OANDNOT;
				goto asop;
			}
			break;
		}
		if(c1 == '=') {
			c = OAND;
			goto asop;
		}
		break;

	case '|':
		c1 = getc();
		if(c1 == '|') {
			c = LOROR;
			goto lx;
		}
		if(c1 == '=') {
			c = OOR;
			goto asop;
		}
		break;

	case '^':
		c1 = getc();
		if(c1 == '=') {
			c = OXOR;
			goto asop;
		}
		break;

	/*
	 * clumsy dance:
	 * to implement rule that disallows
	 *	if T{1}[0] { ... }
	 * but allows
	 * 	if (T{1}[0]) { ... }
	 * the block bodies for if/for/switch/select
	 * begin with an LBODY token, not '{'.
	 *
	 * when we see the keyword, the next
	 * non-parenthesized '{' becomes an LBODY.
	 * loophack is normally 0.
	 * a keyword makes it go up to 1.
	 * parens push loophack onto a stack and go back to 0.
	 * a '{' with loophack == 1 becomes LBODY and disables loophack.
	 *
	 * i said it was clumsy.
	 */
	case '(':
	case '[':
		if(loophack || lstk != nil) {
			h = malloc(sizeof *h);
			if(h == nil) {
				flusherrors();
				yyerror("out of memory");
				errorexit();
			}
			h->v = loophack;
			h->next = lstk;
			lstk = h;
			loophack = 0;
		}
		goto lx;
	case ')':
	case ']':
		if(lstk != nil) {
			h = lstk;
			loophack = h->v;
			lstk = h->next;
			free(h);
		}
		goto lx;
	case '{':
		if(loophack == 1) {
			DBG("%L lex: LBODY\n", lexlineno);
			loophack = 0;
			return LBODY;
		}
		goto lx;

	default:
		goto lx;
	}
	ungetc(c1);

lx:
	if(c > 0xff)
		DBG("%L lex: TOKEN %s\n", lexlineno, lexname(c));
	else
		DBG("%L lex: TOKEN '%c'\n", lexlineno, c);
	if(isfrog(c)) {
		yyerror("illegal character 0x%ux", c);
		goto l0;
	}
	if(importpkg == nil && (c == '#' || c == '$' || c == '?' || c == '@' || c == '\\')) {
		yyerror("%s: unexpected %c", "syntax error", c);
		goto l0;
	}
	return c;

asop:
	yylval.i = c;	// rathole to hold which asop
	DBG("lex: TOKEN ASOP %c\n", c);
	return LASOP;

talph:
	/*
	 * cp is set to lexbuf and some
	 * prefix has been stored
	 */
	for(;;) {
		if(cp+10 >= ep) {
			yyerror("identifier too long");
			errorexit();
		}
		if(c >= Runeself) {
			ungetc(c);
			rune = getr();
			// 0xb7 · is used for internal names
			if(!isalpharune(rune) && !isdigitrune(rune) && (importpkg == nil || rune != 0xb7))
				yyerror("invalid identifier character U+%04x", rune);
			cp += runetochar(cp, &rune);
		} else if(!yy_isalnum(c) && c != '_')
			break;
		else
			*cp++ = c;
		c = getc();
	}
	*cp = 0;
	ungetc(c);

	s = lookup(lexbuf);
	switch(s->lexical) {
	case LIGNORE:
		goto l0;

	case LFOR:
	case LIF:
	case LSWITCH:
	case LSELECT:
		loophack = 1;	// see comment about loophack above
		break;
	}

	DBG("lex: %S %s\n", s, lexname(s->lexical));
	yylval.sym = s;
	return s->lexical;

tnum:
{
	cp = lexbuf;
	ep = lexbuf+sizeof lexbuf;
	// 如果 c != '0' 一般为常规十进制数字
	if(c != '0') {
		for(;;) {
			if(cp+10 >= ep) {
				yyerror("identifier too long");
				errorexit();
			}
			*cp++ = c;
			c = getc();
			if(yy_isdigit(c)) {
				continue;
			}
			goto dc;
		}
	}
	// 如果 c == '0', 则有可能是八进制(0755, 0o755), 或十六进制数字(0xA1)

	*cp++ = c;
	c = getc();
	// 十六进制
	if(c == 'x' || c == 'X') {
		for(;;) {
			if(cp+10 >= ep) {
				yyerror("identifier too long");
				errorexit();
			}
			*cp++ = c;
			c = getc();
			if(yy_isdigit(c)) {
				continue;
			}
			if(c >= 'a' && c <= 'f') {
				continue;
			}
			if(c >= 'A' && c <= 'F') {
				continue;
			}
			if(cp == lexbuf+2) {
				yyerror("malformed hex constant");
			}
			if(c == 'p') {
				goto caseep;
			}
			goto ncu;
		}
	}

	// 0p begins floating point zero
	if(c == 'p') {
		goto caseep;
	}

	c1 = 0;
	// 八进制

	// 对于 0o755 这种八进制格式, 如遇到字符 o/O, 还要再将指针后移一位再处理.
	// 但仍要保留对 0755 这种格式的支持
	if(c == 'o' || c == 'O') {
		*cp++ = c;
		c = getc();
	}
	for(;;) {
		if(cp+10 >= ep) {
			yyerror("identifier too long");
			errorexit();
		}
		if(!yy_isdigit(c)) {
			break;
		}
		if(c < '0' || c > '7') {
			// 一旦有非0-7的字符出现, 就需要考虑其他情况了.
			// not octal
			c1 = 1;
		}
		*cp++ = c;
		c = getc();
	}
	if(c == '.') {
		goto casedot;
	}
	if(c == 'e' || c == 'E') {
		goto caseep;
	}
	if(c == 'i') {
		goto casei;
	}
	if(c1) {
		yyerror("malformed octal constant");
	}
	goto ncu;
}

dc:
{
	if(c == '.') {
		goto casedot;
	}
	if(c == 'e' || c == 'E' || c == 'p' || c == 'P') {
		goto caseep;
	}
	if(c == 'i') {
		goto casei;
	}
}

ncu:
{
	*cp = 0;
	ungetc(c);

	yylval.val.u.xval = mal(sizeof(*yylval.val.u.xval));
	mpatofix(yylval.val.u.xval, lexbuf);
	if(yylval.val.u.xval->ovf) {
		yyerror("overflow in constant");
		mpmovecfix(yylval.val.u.xval, 0);
	}
	yylval.val.ctype = CTINT;
	DBG("lex: integer literal\n");
	strcpy(litbuf, "literal ");
	strcat(litbuf, lexbuf);
	return LLITERAL;
}

casedot:
	for(;;) {
		if(cp+10 >= ep) {
			yyerror("identifier too long");
			errorexit();
		}
		*cp++ = c;
		c = getc();
		if(!yy_isdigit(c))
			break;
	}
	if(c == 'i')
		goto casei;
	if(c != 'e' && c != 'E')
		goto caseout;

caseep:
	*cp++ = c;
	c = getc();
	if(c == '+' || c == '-') {
		*cp++ = c;
		c = getc();
	}
	if(!yy_isdigit(c))
		yyerror("malformed fp constant exponent");
	while(yy_isdigit(c)) {
		if(cp+10 >= ep) {
			yyerror("identifier too long");
			errorexit();
		}
		*cp++ = c;
		c = getc();
	}
	if(c == 'i')
		goto casei;
	goto caseout;

casei:
	// imaginary constant
	*cp = 0;
	yylval.val.u.cval = mal(sizeof(*yylval.val.u.cval));
	mpmovecflt(&yylval.val.u.cval->real, 0.0);
	mpatoflt(&yylval.val.u.cval->imag, lexbuf);
	if(yylval.val.u.cval->imag.val.ovf) {
		yyerror("overflow in imaginary constant");
		mpmovecflt(&yylval.val.u.cval->real, 0.0);
	}
	yylval.val.ctype = CTCPLX;
	DBG("lex: imaginary literal\n");
	strcpy(litbuf, "literal ");
	strcat(litbuf, lexbuf);
	return LLITERAL;

caseout:
	*cp = 0;
	ungetc(c);

	yylval.val.u.fval = mal(sizeof(*yylval.val.u.fval));
	mpatoflt(yylval.val.u.fval, lexbuf);
	if(yylval.val.u.fval->val.ovf) {
		yyerror("overflow in float constant");
		mpmovecflt(yylval.val.u.fval, 0.0);
	}
	yylval.val.ctype = CTFLT;
	DBG("lex: floating literal\n");
	strcpy(litbuf, "literal ");
	strcat(litbuf, lexbuf);
	return LLITERAL;
}

// caller:
//  1. src/cmd/gc/y.tab.c 作为 YYLEX 被使用
int32 yylex(void)
{
	int lx;
	
	lx = _yylex();
	
	if(curio.nlsemi && lx == EOF) {
		// Treat EOF as "end of line" for the purposes
		// of inserting a semicolon.
		lx = ';';
	}

	switch(lx) {
	case LNAME:
	case LLITERAL:
	case LBREAK:
	case LCONTINUE:
	case LFALL:
	case LRETURN:
	case LINC:
	case LDEC:
	case ')':
	case '}':
	case ']':
		curio.nlsemi = 1;
		break;
	default:
		curio.nlsemi = 0;
		break;
	}

	// Track last two tokens returned by yylex.
	yyprev = yylast;
	yylast = lx;
	return lx;
}
