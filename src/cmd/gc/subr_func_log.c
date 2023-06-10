#include	<u.h>
#include	<libc.h>
#include	"go.h"
#include	"y.tab.h"
#include	"yerr.h"

typedef struct Error Error;
struct Error
{
	int lineno;
	int seq;
	char *msg;
};
static Error *err;
static int nerr;
static int merr;

static void adderr(int line, char *fmt, va_list arg)
{
	Fmt f;
	Error *p;

	fmtstrinit(&f);
	fmtprint(&f, "%L: ", line);
	fmtvprint(&f, fmt, arg);
	fmtprint(&f, "\n");

	if(nerr >= merr) {
		if(merr == 0) {
			merr = 16;
		}
		else {
			merr *= 2;
		}
		p = realloc(err, merr*sizeof err[0]);
		if(p == nil) {
			merr = nerr;
			flusherrors();
			print("out of memory\n");
			errorexit();
		}
		err = p;
	}
	err[nerr].seq = nerr;
	err[nerr].lineno = line;
	err[nerr].msg = fmtstrflush(&f);
	nerr++;
}

static int errcmp(const void *va, const void *vb)
{
	Error *a, *b;

	a = (Error*)va;
	b = (Error*)vb;
	if(a->lineno != b->lineno)
		return a->lineno - b->lineno;
	if(a->seq != b->seq)
		return a->seq - b->seq;
	return strcmp(a->msg, b->msg);
}

void flusherrors(void)
{
	int i;

	if(nerr == 0)
		return;
	qsort(err, nerr, sizeof err[0], errcmp);
	for(i=0; i<nerr; i++)
		if(i==0 || strcmp(err[i].msg, err[i-1].msg) != 0)
			print("%s", err[i].msg);
	nerr = 0;
}

static void hcrash(void)
{
	if(debug['h']) {
		flusherrors();
		if(outfile)
			remove(outfile);
		*(volatile int*)0 = 0;
	}
}

void yyerrorl(int line, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	adderr(line, fmt, arg);
	va_end(arg);

	hcrash();
	nerrors++;
	if(nsavederrors+nerrors >= 10 && !debug['e']) {
		flusherrors();
		print("%L: too many errors\n", line);
		errorexit();
	}
}

extern int yystate, yychar;

void yyerror(char *fmt, ...)
{
	int i;
	static int lastsyntax;
	va_list arg;
	char buf[512], *p;

	if(strncmp(fmt, "syntax error", 12) == 0) {
		nsyntaxerrors++;
		
		if(debug['x'])	
			print("yyerror: yystate=%d yychar=%d\n", yystate, yychar);

		// An unexpected EOF caused a syntax error. Use the previous
		// line number since getc generated a fake newline character.
		if(curio.eofnl)
			lexlineno = prevlineno;

		// only one syntax error per line
		if(lastsyntax == lexlineno)
			return;
		lastsyntax = lexlineno;
			
		if(strstr(fmt, "{ or {") || strstr(fmt, " or ?") || strstr(fmt, " or @")) {
			// The grammar has { and LBRACE but both show up as {.
			// Rewrite syntax error referring to "{ or {" to say just "{".
			strecpy(buf, buf+sizeof buf, fmt);
			p = strstr(buf, "{ or {");
			if(p)
				memmove(p+1, p+6, strlen(p+6)+1);
			
			// The grammar has ? and @ but only for reading imports.
			// Silence them in ordinary errors.
			p = strstr(buf, " or ?");
			if(p)
				memmove(p, p+5, strlen(p+5)+1);
			p = strstr(buf, " or @");
			if(p)
				memmove(p, p+5, strlen(p+5)+1);
			fmt = buf;
		}
		
		// look for parse state-specific errors in list (see go.errors).
		for(i=0; i<nelem(yymsg); i++) {
			if(yymsg[i].yystate == yystate && yymsg[i].yychar == yychar) {
				yyerrorl(lexlineno, "syntax error: %s", yymsg[i].msg);
				return;
			}
		}
		
		// plain "syntax error" gets "near foo" added
		if(strcmp(fmt, "syntax error") == 0) {
			yyerrorl(lexlineno, "syntax error near %s", lexbuf);
			return;
		}
		
		// if bison says "syntax error, more info"; print "syntax error: more info".
		if(fmt[12] == ',') {
			yyerrorl(lexlineno, "syntax error:%s", fmt+13);
			return;
		}

		yyerrorl(lexlineno, "%s", fmt);
		return;
	}

	va_start(arg, fmt);
	adderr(parserline(), fmt, arg);
	va_end(arg);

	hcrash();
	nerrors++;
	if(nsavederrors+nerrors >= 10 && !debug['e']) {
		flusherrors();
		print("%L: too many errors\n", parserline());
		errorexit();
	}
}

void mywarn(char *fmt, ...)
{
	va_list arg;
	Fmt f;

	int line = parserline();
	va_start(arg, fmt);
	fmtstrinit(&f);
	fmtprint(&f, "warning: ");
	fmtprint(&f, "%L: ", line);
	fmtvprint(&f, fmt, arg);
	print("%s", fmtstrflush(&f));
	va_end(arg);
}

void warn(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	adderr(parserline(), fmt, arg);
	va_end(arg);

	hcrash();
}

void warnl(int line, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	adderr(line, fmt, arg);
	va_end(arg);
	if(debug['m'])
		flusherrors();
}

void fatal(char *fmt, ...)
{
	va_list arg;

	flusherrors();

	print("%L: internal compiler error: ", lineno);
	va_start(arg, fmt);
	vfprint(1, fmt, arg);
	va_end(arg);
	print("\n");
	
	// If this is a released compiler version, ask for a bug report.
	if(strncmp(getgoversion(), "release", 7) == 0) {
		print("\n");
		print("Please file a bug report including a short program that triggers the error.\n");
		print("http://code.google.com/p/go/issues/entry?template=compilerbug\n");
	}
	hcrash();
	errorexit();
}
