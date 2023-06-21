// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "a.h"

// vflag 是一个数值, 每出现一次 -v 参数, 就会加1, 如 -vvv 时, 该值为 3.
// 在 src/cmd/dist/build.c -> cmdbootstrap() 中进行赋值.
//
int vflag;
int sflag;
char *argv0;

// cmdtab records the available commands.
static struct {
	char *name;
	void (*f)(int, char**);
} cmdtab[] = {
	{"banner", cmdbanner},
	{"bootstrap", cmdbootstrap},
	{"clean", cmdclean},
	{"env", cmdenv},
	{"install", cmdinstall},
	{"version", cmdversion},
};

// caller:
// 	1. src/cmd/dist/unix.c -> main()
//
// The OS-specific main calls into the portable code here.
void xmain(int argc, char **argv)
{
	int i;

	if(argc <= 1) {
		usage();
	}
	
	for(i=0; i<nelem(cmdtab); i++) {
		if(streq(cmdtab[i].name, argv[1])) {
			cmdtab[i].f(argc-1, argv+1);
			return;
		}
	}

	xprintf("unknown command %s\n", argv[1]);
	usage();
}
