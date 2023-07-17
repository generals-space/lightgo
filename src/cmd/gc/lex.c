// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include	<u.h>
#include	<libc.h>
#include	"go.h"
#include	"lex.h"
#include	"y.tab.h"
#include	<ar.h>

extern int yychar;
int windows;
int yyprev;
int yylast;

char *goos, *goarch, *goroot;

#define	BOM	0xFEFF

// Debug arguments.
// These can be specified with the -d flag, as in "-d checknil"
// to set the debug_checknil variable. In general the list passed
// to -d can be comma-separated.
static struct {
	char *name;
	int *val;
} debugtab[] = {
	{"nil", &debug_checknil},
	{nil, nil},
};

// 	@note: 本来这个函数应该放到 lex_exper.c 文件中的, 不过这个函数体中用到了
// `GOEXPERIMENT`变量, 这个变量没有地方显式定义, 而是通过在编译第二阶段(dist命令) 
// src/cmd/dist/build.c -> install() 中, 在单独编译名为"lex.c"的源文件时,
// 用`-D`参数传入的, 暂时不动那里的代码了.
//
// caller:
// 	1. main() 只有这一处
static void setexp(void)
{
	char *f[20];
	int i, nf;
	
	// The makefile #defines GOEXPERIMENT for us.
	nf = getfields(GOEXPERIMENT, f, nelem(f), 1, ",");
	for(i=0; i<nf; i++) {
		addexp(f[i]);
	}
}

// 这个函数是 6g 命令的入口, 每编译一个 package 时, 都会调用一次.
// 	@note: 对于一个完整的工程, 会根据各个 package 的引用顺序, 依次编译. 
// 	也就是说, 每个 package 在编译时都会运行到该 main() 方法中,
// 	localpkg 对象就表示当前正在被编译的 package 信息.
//
// 	@note: 
// 	@todo: 本来在拆分 lex.c 时, 想把 main() 拆分到独立的 main.c 文件中, 更方便查询,
// 	但编译时总是报如下错误:
// ...省略
// cmd/6g
// /usr/local/go/pkg/obj/linux_amd64/lib9.a(main.o): In function `main':
// /usr/local/go/src/lib9/main.c:35: undefined reference to `p9main'
// collect2: error: ld returned 1 exit status
//
// 目前还没有发现 src/lib9/main.c -> main() 时如何通过 p9main() 调用到这里的...
// 所以只能先把 yylex() 函数拆分出去了.
//
// caller:
// 	1. src/lib9/main.c -> main()
int main(int argc, char *argv[])
{
	int i;
	NodeList *l;
	char *p;

#ifdef	SIGBUS	
	signal(SIGBUS, fault);
	signal(SIGSEGV, fault);
#endif
	localpkg = mkpkg(strlit(""));
	localpkg->prefix = "\"\"";

	// pseudo-package, for scoping
	builtinpkg = mkpkg(strlit("go.builtin"));

	// pseudo-package, accessed by import "unsafe"
	unsafepkg = mkpkg(strlit("unsafe"));
	unsafepkg->name = "unsafe";

	// real package, referred to by generated runtime calls
	runtimepkg = mkpkg(strlit("runtime"));
	runtimepkg->name = "runtime";

	// pseudo-packages used in symbol tables
	gostringpkg = mkpkg(strlit("go.string"));
	gostringpkg->name = "go.string";
	gostringpkg->prefix = "go.string";	// not go%2estring

	itabpkg = mkpkg(strlit("go.itab"));
	itabpkg->name = "go.itab";
	itabpkg->prefix = "go.itab";	// not go%2eitab

	weaktypepkg = mkpkg(strlit("go.weak.type"));
	weaktypepkg->name = "go.weak.type";
	weaktypepkg->prefix = "go.weak.type";  // not go%2eweak%2etype

	typelinkpkg = mkpkg(strlit("go.typelink"));
	typelinkpkg->name = "go.typelink";
	typelinkpkg->prefix = "go.typelink"; // not go%2etypelink

	trackpkg = mkpkg(strlit("go.track"));
	trackpkg->name = "go.track";
	trackpkg->prefix = "go.track";  // not go%2etrack

	typepkg = mkpkg(strlit("type"));
	typepkg->name = "type";

	goroot = getgoroot();
	goos = getgoos();
	goarch = thestring;


	setexp();

	outfile = nil;
	// 下面的选项是 6g 命令的参数选项.
	flagcount("+", "compiling runtime", &compiling_runtime);
	flagcount("%", "debug non-static initializers", &debug['%']);
	flagcount("A", "for bootstrapping, allow 'any' type", &debug['A']);
	flagcount("B", "disable bounds checking", &debug['B']);
	flagstr("D", "path: set relative path for local imports", &localimport);
	flagcount("E", "debug symbol export", &debug['E']);
	flagfn1("I", "dir: add dir to import search path", addidir);
	flagcount("K", "debug missing line numbers", &debug['K']);
	flagcount("L", "use full (long) path in error messages", &debug['L']);
	flagcount("M", "debug move generation", &debug['M']);
	flagcount("N", "disable optimizations", &debug['N']);
	flagcount("P", "debug peephole optimizer", &debug['P']);
	flagcount("R", "debug register optimizer", &debug['R']);
	flagcount("S", "print assembly listing", &debug['S']);
	flagfn0("V", "print compiler version", doversion);
	flagcount("W", "debug parse tree after type checking", &debug['W']);
	flagcount("complete", "compiling complete package (no C or assembly)", &pure_go);
	flagstr("d", "list: print debug information about items in list", &debugstr);
	flagcount("e", "no limit on number of errors reported", &debug['e']);
	flagcount("f", "debug stack frames", &debug['f']);
	flagcount("g", "debug code generation", &debug['g']);
	flagcount("h", "halt on error", &debug['h']);
	flagcount("i", "debug line number stack", &debug['i']);
	flagstr("installsuffix", "pkg directory suffix", &flag_installsuffix);
	flagcount("j", "debug runtime-initialized variables", &debug['j']);
	flagcount("l", "disable inlining", &debug['l']);
	flagcount("m", "print optimization decisions", &debug['m']);
	flagstr("o", "obj: set output file", &outfile);
	flagstr("p", "path: set expected package import path", &myimportpath);
	flagcount("r", "debug generated wrappers", &debug['r']);
	flagcount("race", "enable race detector", &flag_race);
	flagcount("s", "warn about composite literals that can be simplified", &debug['s']);
	flagcount("u", "reject unsafe code", &safemode);
	flagcount("v", "increase debug verbosity", &debug['v']);
	flagcount("w", "debug type checking", &debug['w']);
	flagcount("x", "debug lexer", &debug['x']);
	flagcount("y", "debug declarations in canned imports (with -d)", &debug['y']);
	flagcount("nostrict", "disable strict mode for unused variable and package", &nostrictmode);
	if(thechar == '6') {
		flagcount("largemodel", "generate code that assumes a large memory model", &flag_largemodel);
	}

	flagparse(&argc, &argv, usage);

	if(argc < 1) {
		usage();
	}

	if(flag_race) {
		racepkg = mkpkg(strlit("runtime/race"));
		racepkg->name = "race";
	}
	
	// parse -d argument
	if(debugstr) {
		char *f[100];
		int i, j, nf;

		nf = getfields(debugstr, f, nelem(f), 1, ",");
		for(i=0; i<nf; i++) {
			for(j=0; debugtab[j].name != nil; j++) {
				if(strcmp(debugtab[j].name, f[i]) == 0) {
					*debugtab[j].val = 1;
					break;
				}
			}
			if(j == nelem(debugtab)) {
				fatal("unknown debug information -d '%s'\n", f[i]);
			}
		}
	}

	// enable inlining.  for now:
	//	default: inlining on.  (debug['l'] == 1)
	//	-l: inlining off  (debug['l'] == 0)
	//	-ll, -lll: inlining on again, with extra debugging (debug['l'] > 1)
	if(debug['l'] <= 1) {
		debug['l'] = 1 - debug['l'];
	}

	pathname = mal(1000);
	if(getwd(pathname, 999) == 0) {
		strcpy(pathname, "/???");
	}

	if(yy_isalpha(pathname[0]) && pathname[1] == ':') {
		// On Windows.
		windows = 1;

		// Canonicalize path by converting \ to / (Windows accepts both).
		for(p=pathname; *p; p++) {
			if(*p == '\\') {
				*p = '/';
			}
		}
	}

	fmtinstallgo();
	betypeinit();
	if(widthptr == 0) {
		fatal("betypeinit failed");
	}

	lexinit();
	typeinit();
	lexinit1();
	yytinit();

	blockgen = 1;
	dclcontext = PEXTERN;
	nerrors = 0;
	lexlineno = 1;

	for(i=0; i<argc; i++) {
		infile = argv[i];
		linehist(infile, 0, 0);

		curio.infile = infile;
		curio.bin = Bopen(infile, OREAD);
		if(curio.bin == nil) {
			print("open %s: %r\n", infile);
			errorexit();
		}
		curio.peekc = 0;
		curio.peekc1 = 0;
		curio.nlsemi = 0;
		curio.eofnl = 0;
		curio.last = 0;

		// Skip initial BOM if present.
		if(Bgetrune(curio.bin) != BOM) {
			Bungetrune(curio.bin);
		}

		block = 1;
		iota = -1000000;
		// 1. 完成 xtop 对象的初始化
		// 2. 完成 localpkg 对象的初始化
		yyparse();
		if(nsyntaxerrors != 0) {
			errorexit();
		}

		linehist(nil, 0, 0);
		if(curio.bin != nil) {
			Bterm(curio.bin);
		}
	}
	testdclstack();
	mkpackage(localpkg->name);	// final import not used checks
	lexfini();

	typecheckok = 1;
	if(debug['f']) {
		frame(1);
	}

	// Process top-level declarations in phases.

	// Phase 1: const, type, and names and types of funcs.
	//   This will gather all the information about types
	//   and methods but doesn't depend on any of it.
	defercheckwidth();
	// xtop 由上面的 yyparse() 函数完成初始化.
	for(l=xtop; l; l=l->next) {
		// 对 const 常量, type struct xxx 类型别名, 以及函数名称和类型等语句, 进行类型检查.
		if(l->n->op != ODCL && l->n->op != OAS) {
			typecheck(&l->n, Etop);
		}
	}

	// Phase 2: Variable assignments.
	//   To check interface assignments, depends on phase 1.
	for(l=xtop; l; l=l->next) {
		// 对初始化/赋值类型的变量声明语句, 进行类型检查.
		if(l->n->op == ODCL || l->n->op == OAS) {
			typecheck(&l->n, Etop);
		}
	}
	resumecheckwidth();

	// Phase 3: Type check function bodies.
	for(l=xtop; l; l=l->next) {
		if(l->n->op == ODCLFUNC || l->n->op == OCLOSURE) {
			curfn = l->n;
			saveerrors();
			typechecklist(l->n->nbody, Etop);
			checkreturn(l->n);
			if(nerrors != 0) {
				l->n->nbody = nil;  // type errors; do not compile
			}
		}
	}

	curfn = nil;
	
	if(nsavederrors+nerrors) {
		errorexit();
	}

	// Phase 4: Inlining
	//
	// 如果禁用内联, 进入此块
	if(debug['l'] > 1) {
		// Typecheck imported function bodies if debug['l'] > 1,
		// otherwise lazily when used or re-exported.
		for(l=importlist; l; l=l->next) {
			if (l->n->inl) {
				saveerrors();
				typecheckinl(l->n);
			}
		}

		if(nsavederrors+nerrors) {
			errorexit();
		}
	}
	// 如果允许内联, 则进入此块
	if(debug['l']) {
		// Find functions that can be inlined and clone them before walk expands them.
		for(l=xtop; l; l=l->next) {
			if(l->n->op == ODCLFUNC) {
				caninl(l->n);
			}
		}

		// Expand inlineable calls in all functions
		for(l=xtop; l; l=l->next) {
			if(l->n->op == ODCLFUNC) {
				inlcalls(l->n);
			}
		}
	}

	// Phase 5: Escape analysis.
	//
	// 禁用优化 - 逃逸分析
	// 1. 指针变量
	// 2. 大对象
	//
	if(!debug['N']) {
		escapes(xtop);
	}
	// Escape analysis moved escaped values off stack.
	// Move large values off stack too.
	movelarge(xtop);

	// Phase 6: Compile top level functions.
	for(l=xtop; l; l=l->next) {
		if(l->n->op == ODCLFUNC) {
			funccompile(l->n, 0);
		}
	}

	if(nsavederrors+nerrors == 0) {
		fninit(xtop);
	}

	// Phase 7: Check external declarations.
	//
	// externdcl 由上面的 yyparse() 函数完成初始化.
	for(l=externdcl; l; l=l->next) {
		if(l->n->op == ONAME) {
			typecheck(&l->n, Erv);
		}
	}

	if(nerrors+nsavederrors) {
		errorexit();
	}

	// 这里是实际输出 main.6 文件的地方.
	dumpobj();

	if(nerrors+nsavederrors) {
		errorexit();
	}

	flusherrors();
	exits(0);
	return 0;
}
