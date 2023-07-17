#include	<u.h>
#include	<libc.h>

#include	"go.h"
#include	"lex.h"
#include	<ar.h>

/*
 *	macro to portably read/write archive header.
 *	'cmd' is read/write/Bread/Bwrite, etc.
 */
#define	HEADER_IO(cmd, f, h)	cmd(f, h.name, sizeof(h.name)) != sizeof(h.name)\
				|| cmd(f, h.date, sizeof(h.date)) != sizeof(h.date)\
				|| cmd(f, h.uid, sizeof(h.uid)) != sizeof(h.uid)\
				|| cmd(f, h.gid, sizeof(h.gid)) != sizeof(h.gid)\
				|| cmd(f, h.mode, sizeof(h.mode)) != sizeof(h.mode)\
				|| cmd(f, h.size, sizeof(h.size)) != sizeof(h.size)\
				|| cmd(f, h.fmag, sizeof(h.fmag)) != sizeof(h.fmag)

// caller:
// 	1. skiptopkgdef() 只有这一处
static int arsize(Biobuf *b, char *name)
{
	struct ar_hdr a;

	if (HEADER_IO(Bread, b, a))
		return -1;

	if(strncmp(a.name, name, strlen(name)) != 0)
		return -1;

	return atoi(a.size);
}

// caller:
// 	1. importfile() 只有这一处
static int skiptopkgdef(Biobuf *b)
{
	char *p;
	int sz;

	/* archive header */
	if((p = Brdline(b, '\n')) == nil)
		return 0;
	if(Blinelen(b) != 8)
		return 0;
	if(memcmp(p, "!<arch>\n", 8) != 0)
		return 0;
	/* symbol table is first; skip it */
	sz = arsize(b, "__.GOSYMDEF");
	if(sz < 0)
		return 0;
	Bseek(b, sz, 1);
	/* package export block is second */
	sz = arsize(b, "__.PKGDEF");
	if(sz <= 0)
		return 0;
	return 1;
}

// islocalname 判断目标 name 是否为本地路径(golang 不允许通过本地路径指定 package)
//
// caller:
// 	1. findpkg()
// 	2. importfile()
//
// is this path a local name?  begins with ./ or ../ or /
static int islocalname(Strlit *name)
{
	if(name->len >= 1 && name->s[0] == '/')
		return 1;
	if(name->len >= 2 && strncmp(name->s, "./", 2) == 0)
		return 1;
	if(name->len == 1 && strncmp(name->s, ".", 1) == 0)
		return 1;
	if(name->len >= 3 && strncmp(name->s, "../", 3) == 0)
		return 1;
	if(name->len == 2 && strncmp(name->s, "..", 2) == 0)
		return 1;
	return 0;
}

// findpkg 在 pkg/linux_amd64 目录下寻找目标库, 找到则返回1, 否则返回0.
//
// 但只能找到标准库对应的 .a 静态链接库, 无法找到第三方库, 看起来第三方库是在链接过程处理的.
//
// 	@param name: 开发者通过 import() 语句引入的库(可以是标准库, 也可以是第三方库)
//
// caller:
// 	1. importfile() 只有这一处
static int findpkg(Strlit *name)
{
	Idir *p;
	char *q, *suffix, *suffixsep;

	if(islocalname(name)) {
		if(safemode) {
			return 0;
		}
		// try .a before .6.  important for building libraries:
		// if there is an array.6 in the array.a library,
		// want to find all of array.a, not just array.6.
		snprint(namebuf, sizeof(namebuf), "%Z.a", name);
		if(access(namebuf, 0) >= 0) {
			return 1;
		}
		snprint(namebuf, sizeof(namebuf), "%Z.%c", name, thechar);
		if(access(namebuf, 0) >= 0) {
			return 1;
		}
		return 0;
	}

	// local imports should be canonicalized already.
	// don't want to see "encoding/../encoding/base64"
	// as different from "encoding/base64".
	q = mal(name->len+1);
	memmove(q, name->s, name->len);
	q[name->len] = '\0';
	cleanname(q);
	if(strlen(q) != name->len || memcmp(q, name->s, name->len) != 0) {
		yyerror("non-canonical import path %Z (should be %s)", name, q);
		return 0;
	}

	for(p = idirs; p != nil; p = p->link) {
		snprint(namebuf, sizeof(namebuf), "%s/%Z.a", p->dir, name);
		if(access(namebuf, 0) >= 0) {
			return 1;
		}
		snprint(namebuf, sizeof(namebuf), "%s/%Z.%c", p->dir, name, thechar);
		if(access(namebuf, 0) >= 0) {
			return 1;
		}
	}
	if(goroot != nil) {
		suffix = "";
		suffixsep = "";
		if(flag_installsuffix != nil) {
			suffixsep = "_";
			suffix = flag_installsuffix;
		} else if(flag_race) {
			suffixsep = "_";
			suffix = "race";
		}
		// 在 pkg/linux_amd64 目录下寻找目标库, 不过只能找到标准库.
		snprint(
			namebuf, sizeof(namebuf), "%s/pkg/%s_%s%s%s/%Z.a", 
			goroot, goos, goarch, suffixsep, suffix, name
		);
		if(access(namebuf, 0) >= 0) {
			return 1;
		}
		snprint(
			namebuf, sizeof(namebuf), "%s/pkg/%s_%s%s%s/%Z.%c", 
			goroot, goos, goarch, suffixsep, suffix, name, thechar
		);
		if(access(namebuf, 0) >= 0) {
			return 1;
		}
	}
	return 0;
}

static void fakeimport(void)
{
	importpkg = mkpkg(strlit("fake"));
	cannedimports("fake.6", "$$\n");
}

//
// caller:
// 	1. src/cmd/gc/y.tab.c -> yyparse() 在 main() 函数中调用.
void importfile(Val *f, int line)
{
	Biobuf *imp;
	char *file, *p, *q, *tag;
	int32 c;
	int len;
	Strlit *path;
	char *cleanbuf, *prefix;

	USED(line);

	if(f->ctype != CTSTR) {
		yyerror("import statement not a string");
		fakeimport();
		return;
	}

	if(f->u.sval->len == 0) {
		yyerror("import path is empty");
		fakeimport();
		return;
	}

	if(isbadimport(f->u.sval)) {
		fakeimport();
		return;
	}

	// The package name main is no longer reserved,
	// but we reserve the import path "main" to identify
	// the main package, just as we reserve the import 
	// path "math" to identify the standard math package.
	if(strcmp(f->u.sval->s, "main") == 0) {
		yyerror("cannot import \"main\"");
		errorexit();
	}

	if(myimportpath != nil && strcmp(f->u.sval->s, myimportpath) == 0) {
		yyerror("import \"%Z\" while compiling that package (import cycle)", f->u.sval);
		errorexit();
	}

	if(strcmp(f->u.sval->s, "unsafe") == 0) {
		if(safemode) {
			yyerror("cannot import package unsafe");
			errorexit();
		}
		importpkg = mkpkg(f->u.sval);
		cannedimports("unsafe.6", unsafeimport);
		return;
	}

	path = f->u.sval;
	if(islocalname(path)) {
		// golang 不支持相对路径的本地包吧? 只支持在 gopath 下存放的包
		if(path->s[0] == '/') {
			yyerror("import path cannot be absolute path");
			fakeimport();
			return;
		}
		prefix = pathname;
		if(localimport != nil) {
			prefix = localimport;
		}
		cleanbuf = mal(strlen(prefix) + strlen(path->s) + 2);
		strcpy(cleanbuf, prefix);
		strcat(cleanbuf, "/");
		strcat(cleanbuf, path->s);
		cleanname(cleanbuf);
		path = strlit(cleanbuf);
		
		if(isbadimport(path)) {
			fakeimport();
			return;
		}
	}
	// print("importfile() import path: %s\n", f->u.sval->s);

	if(!findpkg(path)) {
		// 在 pkg/linux_amd64 目录下寻找目标库, 但编译期间只能找到标准库对应的 .a 静态链接库,
		// 无法找到第三方库, 看起来第三方库是在链接过程处理的.
		//
		// 引入第三方库时, 直接使用 6g 命令会报这里的错误, 不过 go run/build 则不会.
		yyerror("can't find import: \"%Z\"", f->u.sval);
		errorexit();
	}
	importpkg = mkpkg(path);

	// 运行到这里, namebuf 一般为 pkg/linux_amd64 目录下某个标准库的 .a 文件全路径.

	// 如果之前已经加载过了, 不要重复加载, 直接返回.
	//
	// If we already saw that package, feed a dummy statement
	// to the lexer to avoid parsing export data twice.
	if(importpkg->imported) {
		file = strdup(namebuf);
		tag = "";
		if(importpkg->safe) {
			tag = "safe";
		}
		p = smprint("package %s %s\n$$\n", importpkg->name, tag);
		cannedimports(file, p);
		return;
	}
	importpkg->imported = 1;

	// 打开目标库的 .a 静态链接库文件, 确认文件格式.
	// 编译期间不进行其他额外操作.

	imp = Bopen(namebuf, OREAD);
	if(imp == nil) {
		yyerror("can't open import: \"%Z\": %r", f->u.sval);
		errorexit();
	}
	file = strdup(namebuf);

	len = strlen(namebuf);
	if(len > 2 && namebuf[len-2] == '.' && namebuf[len-1] == 'a') {
		if(!skiptopkgdef(imp)) {
			yyerror("import %s: not a package file", file);
			errorexit();
		}
	}

	// check object header
	p = Brdstr(imp, '\n', 1);
	if(strcmp(p, "empty archive") != 0) {
		if(strncmp(p, "go object ", 10) != 0) {
			yyerror("import %s: not a go object file", file);
			errorexit();
		}
		q = smprint("%s %s %s %s", getgoos(), thestring, getgoversion(), expstring());
		if(strcmp(p+10, q) != 0) {
			yyerror("import %s: object is [%s] expected [%s]", file, p+10, q);
			errorexit();
		}
		free(q);
	}

	// assume files move (get installed)
	// so don't record the full path.
	linehist(file + len - path->len - 2, -1, 1);	// acts as #pragma lib

	/*
	 * 定位 $$ 出现的位置并返回, 如果没找到则表示这并不是合法的包, 需要回退一些操作.
	 * position the input right after $$ and return
	 */
	pushedio = curio;
	curio.bin = imp;
	curio.peekc = 0;
	curio.peekc1 = 0;
	curio.infile = file;
	curio.nlsemi = 0;
	typecheckok = 1;

	for(;;) {
		c = getc();
		if(c == EOF) {
			break;
		}
		if(c != '$') {
			continue;
		}
		c = getc();
		if(c == EOF) {
			break;
		}
		if(c != '$') {
			continue;
		}
		return;
	}
	// 没找到则表示这并不是合法的包, 需要回退一些操作.
	yyerror("no import in \"%Z\"", f->u.sval);
	unimportfile();
}

// caller:
// 	1. importfile() 在 pkg/linux_amd64 中找到了目标包, 但不是合法包, 需要回退一些操作.
void unimportfile(void)
{
	if(curio.bin != nil) {
		Bterm(curio.bin);
		curio.bin = nil;
	} else {
		lexlineno--;	// re correct sys.6 line number
	}

	curio = pushedio;
	pushedio.bin = nil;
	incannedimport = 0;
	typecheckok = 0;
}

// caller:
// 	1. fakeimport()
// 	2. importfile()
// 	3. src/cmd/gc/go.y -> loadsys{} 
void cannedimports(char *file, char *cp)
{
	// if sys.6 is included on line 1,
	lexlineno++;

	pushedio = curio;
	curio.bin = nil;
	curio.peekc = 0;
	curio.peekc1 = 0;
	curio.infile = file;
	curio.cp = cp;
	curio.nlsemi = 0;
	curio.importsafe = 0;

	typecheckok = 1;
	incannedimport = 1;
}
