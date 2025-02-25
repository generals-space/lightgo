// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "a.h"

/*
 * Helpers for building pkg/runtime.
 */

// mkzversion writes zversion.go:
//
//	package runtime
//	const defaultGoroot = <goroot>
//	const theVersion = <version>
//
void mkzversion(char *dir, char *file)
{
	Buf b, out;
	
	USED(dir);

	binit(&b);
	binit(&out);
	
	bwritestr(&out, bprintf(&b,
		"// auto generated by go tool dist\n"
		"\n"
		"package runtime\n"
		"\n"
		"const defaultGoroot = `%s`\n"
		"const theVersion = `%s`\n", goroot_final, goversion));

	writefile(&out, file, 0);
	
	bfree(&b);
	bfree(&out);
}

// mkzexperiment writes zaexperiment.h (sic):
//
//	#define GOEXPERIMENT "experiment string"
//
void mkzexperiment(char *dir, char *file)
{
	Buf b, out, exp;
	
	USED(dir);

	binit(&b);
	binit(&out);
	binit(&exp);
	
	xgetenv(&exp, "GOEXPERIMENT");
	bwritestr(
		&out, bprintf(&b,
		"// auto generated by go tool dist\n"
		"\n"
		"#define GOEXPERIMENT \"%s\"\n", 
		bstr(&exp))
	);

	writefile(&out, file, 0);
	
	bfree(&b);
	bfree(&out);
	bfree(&exp);
}

// mkzgoarch writes zgoarch_$GOARCH.go:
//
//	package runtime
//	const theGoarch = <goarch>
//
void mkzgoarch(char *dir, char *file)
{
	Buf b, out;

	USED(dir);
	
	binit(&b);
	binit(&out);
	
	bwritestr(&out, bprintf(&b,
		"// auto generated by go tool dist\n"
		"\n"
		"package runtime\n"
		"\n"
		"const theGoarch = `%s`\n", goarch));

	writefile(&out, file, 0);
	
	bfree(&b);
	bfree(&out);
}

// mkzgoos writes zgoos_$GOOS.go:
//
//	package runtime
//	const theGoos = <goos>
//
void mkzgoos(char *dir, char *file)
{
	Buf b, out;

	USED(dir);
	
	binit(&b);
	binit(&out);
	
	bwritestr(&out, bprintf(&b,
		"// auto generated by go tool dist\n"
		"\n"
		"package runtime\n"
		"\n"
		"const theGoos = `%s`\n", goos));

	writefile(&out, file, 0);
	
	bfree(&b);
	bfree(&out);
}

static struct {
	char *goarch;
	char *goos;
	char *hdr;
} zasmhdr[] = {
	{"386", "linux",
		"// On Linux systems, what we call 0(GS) and 4(GS) for g and m\n"
		"// turn into %gs:-8 and %gs:-4 (using gcc syntax to denote\n"
		"// what the machine sees as opposed to 8l input).\n"
		"// 8l rewrites 0(GS) and 4(GS) into these.\n"
		"//\n"
		"// On Linux Xen, it is not allowed to use %gs:-8 and %gs:-4\n"
		"// directly.  Instead, we have to store %gs:0 into a temporary\n"
		"// register and then use -8(%reg) and -4(%reg).  This kind\n"
		"// of addressing is correct even when not running Xen.\n"
		"//\n"
		"// 8l can rewrite MOVL 0(GS), CX into the appropriate pair\n"
		"// of mov instructions, using CX as the intermediate register\n"
		"// (safe because CX is about to be written to anyway).\n"
		"// But 8l cannot handle other instructions, like storing into 0(GS),\n"
		"// which is where these macros come into play.\n"
		"// get_tls sets up the temporary and then g and r use it.\n"
		"//\n"
		"// Another wrinkle is that get_tls needs to read from %gs:0,\n"
		"// but in 8l input it's called 8(GS), because 8l is going to\n"
		"// subtract 8 from all the offsets, as described above.\n"
		"//\n"
		"// The final wrinkle is that when generating an ELF .o file for\n"
		"// external linking mode, we need to be able to relocate the\n"
		"// -8(r) and -4(r) instructions. Tag them with an extra (GS*1)\n"
		"// that is ignored by the linker except for that identification.\n"
		"#define	get_tls(r)	MOVL 8(GS), r\n"
		"#define	g(r)	-8(r)(GS*1)\n"
		"#define	m(r)	-4(r)(GS*1)\n"
	},
	{"386", "",
		"#define	get_tls(r)\n"
		"#define	g(r)	0(GS)\n"
		"#define	m(r)	4(GS)\n"
	},

	// The TLS accessors here are defined here to use initial exec model.
	// If the linker is not outputting a shared library, it will reduce
	// the TLS accessors to the local exec model, effectively removing
	// get_tls().
	{"amd64", "linux",
		"#define	get_tls(r) MOVQ runtime·tlsgm(SB), r\n"
		"#define	g(r) 0(r)(GS*1)\n"
		"#define	m(r) 8(r)(GS*1)\n"
	},
	{"amd64", "",
		"#define get_tls(r)\n"
		"#define g(r) 0(GS)\n"
		"#define m(r) 8(GS)\n"
	},
};

#define MAXWINCB 2000 /* maximum number of windows callbacks allowed */

// mkzasm writes zasm_$GOOS_$GOARCH.h,
// which contains struct offsets for use by
// assembly files.  It also writes a copy to the work space
// under the name zasm_GOOS_GOARCH.h (no expansion).
// 
void mkzasm(char *dir, char *file)
{
	int i, n;
	char *aggr, *p;
	Buf in, b, out, exp;
	Vec argv, lines, fields;

	binit(&in);
	binit(&b);
	binit(&out);
	binit(&exp);
	vinit(&argv);
	vinit(&lines);
	vinit(&fields);
	
	bwritestr(&out, "// auto generated by go tool dist\n\n");
	for(i=0; i<nelem(zasmhdr); i++) {
		if(hasprefix(goarch, zasmhdr[i].goarch) && hasprefix(goos, zasmhdr[i].goos)) {
			bwritestr(&out, zasmhdr[i].hdr);
			goto ok;
		}
	}
	fatal("unknown $GOOS/$GOARCH in mkzasm");
ok:

	// Run 6c -D GOOS_goos -D GOARCH_goarch -I workdir -a -n -o workdir/proc.acid proc.c
	// to get acid [sic] output.
	vreset(&argv);
	vadd(&argv, bpathf(&b, "%s/%sc", tooldir, gochar));
	vadd(&argv, "-D");
	vadd(&argv, bprintf(&b, "GOOS_%s", goos));
	vadd(&argv, "-D");
	vadd(&argv, bprintf(&b, "GOARCH_%s", goarch));
	vadd(&argv, "-I");
	vadd(&argv, bprintf(&b, "%s", workdir));
	vadd(&argv, "-a");
	vadd(&argv, "-n");
	vadd(&argv, "-o");
	vadd(&argv, bpathf(&b, "%s/proc.acid", workdir));
	vadd(&argv, "proc.c");
	runv(nil, dir, CheckExit, &argv);
	readfile(&in, bpathf(&b, "%s/proc.acid", workdir));
	
	// Convert input like
	//	aggr G
	//	{
	//		Gobuf 24 sched;
	//		'Y' 48 stack0;
	//	}
	//	StackMin = 128;
	// into output like
	//	#define g_sched 24
	//	#define g_stack0 48
	//	#define const_StackMin 128
	aggr = nil;
	splitlines(&lines, bstr(&in));
	for(i=0; i<lines.len; i++) {
		splitfields(&fields, lines.p[i]);
		if(fields.len == 2 && streq(fields.p[0], "aggr")) {
			if(streq(fields.p[1], "G"))
				aggr = "g";
			else if(streq(fields.p[1], "M"))
				aggr = "m";
			else if(streq(fields.p[1], "P"))
				aggr = "p";
			else if(streq(fields.p[1], "Gobuf"))
				aggr = "gobuf";
			else if(streq(fields.p[1], "WinCallbackContext"))
				aggr = "cbctxt";
			else if(streq(fields.p[1], "SEH"))
				aggr = "seh";
		}
		if(hasprefix(lines.p[i], "}"))
			aggr = nil;
		if(aggr && hasprefix(lines.p[i], "\t") && fields.len >= 2) {
			n = fields.len;
			p = fields.p[n-1];
			if(p[xstrlen(p)-1] == ';')
				p[xstrlen(p)-1] = '\0';
			bwritestr(&out, bprintf(&b, "#define %s_%s %s\n", aggr, fields.p[n-1], fields.p[n-2]));
		}
		if(fields.len == 3 && streq(fields.p[1], "=")) { // generated from enumerated constants
			p = fields.p[2];
			if(p[xstrlen(p)-1] == ';')
				p[xstrlen(p)-1] = '\0';
			bwritestr(&out, bprintf(&b, "#define const_%s %s\n", fields.p[0], p));
		}
	}

	xgetenv(&exp, "GOEXPERIMENT");
	bwritestr(&out, bprintf(&b, "#define GOEXPERIMENT \"%s\"\n", bstr(&exp)));
	
	// Write both to file and to workdir/zasm_GOOS_GOARCH.h.
	writefile(&out, file, 0);
	writefile(&out, bprintf(&b, "%s/zasm_GOOS_GOARCH.h", workdir), 0);

	bfree(&in);
	bfree(&b);
	bfree(&out);
	bfree(&exp);
	vfree(&argv);
	vfree(&lines);
	vfree(&fields);
}

// mkzsys writes zsys_$GOOS_$GOARCH.h,
// which contains arch or os specific asm code.
// 
void mkzsys(char *dir, char *file)
{
	Buf out;

	USED(dir);
	
	binit(&out);
	
	bwritestr(&out, "// auto generated by go tool dist\n\n");

	writefile(&out, file, 0);
	
	bfree(&out);
}

static char *runtimedefs[] = {
	"proc.c",
	"iface.c",
	"hashmap.c",
	"chan.c",
	"parfor.c",
};

// mkzruntimedefs writes zruntime_defs_$GOOS_$GOARCH.h,
// which contains Go struct definitions equivalent to the C ones.
// Mostly we just write the output of 6c -q to the file.
// However, we run it on multiple files, so we have to delete
// the duplicated definitions, and we don't care about the funcs
// and consts, so we delete those too.
// 
void mkzruntimedefs(char *dir, char *file)
{
	int i, skip;
	char *p;
	Buf in, b, b1, out;
	Vec argv, lines, fields, seen;
	
	binit(&in);
	binit(&b);
	binit(&b1);
	binit(&out);
	vinit(&argv);
	vinit(&lines);
	vinit(&fields);
	vinit(&seen);
	
	bwritestr(&out, "// auto generated by go tool dist\n"
		"\n"
		"package runtime\n"
		"import \"unsafe\"\n"
		"var _ unsafe.Pointer\n"
		"\n"
	);

	
	// Run 6c -D GOOS_goos -D GOARCH_goarch -I workdir -q -n -o workdir/runtimedefs
	// on each of the runtimedefs C files.
	vadd(&argv, bpathf(&b, "%s/%sc", tooldir, gochar));
	vadd(&argv, "-D");
	vadd(&argv, bprintf(&b, "GOOS_%s", goos));
	vadd(&argv, "-D");
	vadd(&argv, bprintf(&b, "GOARCH_%s", goarch));
	vadd(&argv, "-I");
	vadd(&argv, bprintf(&b, "%s", workdir));
	vadd(&argv, "-q");
	vadd(&argv, "-n");
	vadd(&argv, "-o");
	vadd(&argv, bpathf(&b, "%s/runtimedefs", workdir));
	vadd(&argv, "");
	p = argv.p[argv.len-1];
	for(i=0; i<nelem(runtimedefs); i++) {
		argv.p[argv.len-1] = runtimedefs[i];
		runv(nil, dir, CheckExit, &argv);
		readfile(&b, bpathf(&b1, "%s/runtimedefs", workdir));
		bwriteb(&in, &b);
	}
	argv.p[argv.len-1] = p;
		
	// Process the aggregate output.
	skip = 0;
	splitlines(&lines, bstr(&in));
	for(i=0; i<lines.len; i++) {
		p = lines.p[i];
		// Drop comment, func, and const lines.
		if(hasprefix(p, "//") || hasprefix(p, "const") || hasprefix(p, "func"))
			continue;
		
		// Note beginning of type or var decl, which can be multiline.
		// Remove duplicates.  The linear check of seen here makes the
		// whole processing quadratic in aggregate, but there are only
		// about 100 declarations, so this is okay (and simple).
		if(hasprefix(p, "type ") || hasprefix(p, "var ")) {
			splitfields(&fields, p);
			if(fields.len < 2)
				continue;
			if(find(fields.p[1], seen.p, seen.len) >= 0) {
				if(streq(fields.p[fields.len-1], "{"))
					skip = 1;  // skip until }
				continue;
			}
			vadd(&seen, fields.p[1]);
		}
		if(skip) {
			if(hasprefix(p, "}"))
				skip = 0;
			continue;
		}
		
		bwritestr(&out, p);
	}
	
	writefile(&out, file, 0);

	bfree(&in);
	bfree(&b);
	bfree(&b1);
	bfree(&out);
	vfree(&argv);
	vfree(&lines);
	vfree(&fields);
	vfree(&seen);
}
