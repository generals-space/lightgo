#include	<u.h>
#include	<libc.h>

#include	"go.h"

// Compiler experiments.
// These are controlled by the GOEXPERIMENT environment
// variable recorded when the compiler is built.
static struct {
	char *name;
	int *val;
} exper[] = {
//	{"rune32", &rune32},
	{"fieldtrack", &fieldtrack_enabled},
	{"precisestack", &precisestack_enabled},
	{nil, nil},
};

// caller:
// 	1. setexp() 只有这一处
void addexp(char *s)
{
	int i;

	for(i=0; exper[i].name != nil; i++) {
		if(strcmp(exper[i].name, s) == 0) {
			*exper[i].val = 1;
			return;
		}
	}

	print("unknown experiment %s\n", s);
	exits("unknown experiment");
}

char* expstring(void)
{
	int i;
	static char buf[512];

	strcpy(buf, "X");
	for(i=0; exper[i].name != nil; i++) {
		if(*exper[i].val) {
			seprint(buf+strlen(buf), buf+sizeof buf, ",%s", exper[i].name);
		}
	}
	if(strlen(buf) == 1) {
		strcpy(buf, "X,none");
	}
	buf[1] = ':';
	return buf;
}

////////////////////////////////////////////////////////////////////////////////

// addidir 将目标 dir 添加到 idirs 末尾.
//
// caller:
// 	1. main() 只有这一处, 通过 6g -I 参数传入
void addidir(char* dir)
{
	Idir** pp;

	if(dir == nil) {
		return;
	}

	for(pp = &idirs; *pp != nil; pp = &(*pp)->link) {
		;
	}
	*pp = mal(sizeof(Idir));
	(*pp)->link = nil;
	(*pp)->dir = dir;
}

void usage(void)
{
	print("usage: %cg [options] file.go...\n", thechar);
	flagprint(1);
	exits("usage");
}

void fault(int s)
{
	USED(s);

	// If we've already complained about things
	// in the program, don't bother complaining
	// about the seg fault too; let the user clean up
	// the code and try again.
	if(nsavederrors + nerrors > 0) {
		errorexit();
	}
	fatal("fault");
}

void doversion(void)
{
	char *p;

	p = expstring();
	if(strcmp(p, "X:none") == 0) {
		p = "";
	}
	print("%cg version %s%s%s\n", thechar, getgoversion(), *p ? " " : "", p);
	exits(0);
}

void saveerrors(void)
{
	nsavederrors += nerrors;
	nerrors = 0;
}
