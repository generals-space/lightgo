/*
 * build_cmd.c 由 build.c 拆分而来, build.c 只保留 bootstrap 核心流程,
 * 本文件则主要负责 dist 的其他子命令, 如 install, env, clean 等.
 * 
 * 20230609
 */

#include "a.h"
#include "arg.h"

////////////////////////////////////////////////////////////////////////////////

void usage(void)
{
	xprintf("usage: go tool dist [command]\n"
		"Commands are:\n"
		"\n"
		"banner         print installation banner\n"
		"bootstrap      rebuild everything\n"
		"clean          deletes all built files\n"
		"env [-p]       print environment (-p: include $PATH)\n"
		"install [dir]  install individual directory\n"
		"version        print Go version\n"
		"\n"
		"All commands take -v flags to emit extra information.\n"
	);
	xexit(2);
}

// caller:
//  1. cmdinstall() 只有这一处
static char* defaulttarg(void)
{
	char *p;
	Buf pwd, src, real_src;

	binit(&pwd);
	binit(&src);
	binit(&real_src);

	// xgetwd might return a path with symlinks fully resolved, and if
	// there happens to be symlinks in goroot, then the hasprefix test
	// will never succeed. Instead, we use xrealwd to get a canonical
	// goroot/src before the comparison to avoid this problem.
	xgetwd(&pwd);
	p = btake(&pwd);
	bpathf(&src, "%s/src/", goroot);
	xrealwd(&real_src, bstr(&src));
	if(!hasprefix(p, bstr(&real_src))) {
		fatal("current directory %s is not under %s", p, bstr(&real_src));
	}
	p += real_src.len;
	// guard againt xrealwd return the directory without the trailing /
	if(*p == slash[0]) {
		p++;
	}

	bfree(&pwd);
	bfree(&src);
	bfree(&real_src);

	return p;
}

// caller:
// 	1. src/cmd/dist/main.c -> install 子命令的入口函数.
//
// Install installs the list of packages named on the command line.
void cmdinstall(int argc, char **argv)
{
	int i;

	ARGBEGIN{
	case 's':
		sflag++;
		break;
	case 'v':
		vflag++;
		break;
	default:
		usage();
	}ARGEND

	if(argc == 0) {
		install(defaulttarg());
	}

	for(i=0; i<argc; i++) {
		install(argv[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////

// cleantab records the directories to clean in 'go clean'.
// It is bigger than the buildorder because we clean all the
// compilers but build only the $GOARCH ones.
static char *cleantab[] = {
	"cmd/6a", "cmd/6c", "cmd/6g", "cmd/6l",
	"cmd/addr2line",
	"cmd/cc",
	"cmd/gc",
	"cmd/go",
	"cmd/nm",
	"cmd/objdump",
	"cmd/pack",
	"cmd/prof",
	"lib9",
	"libbio",
	"libmach",
	"pkg/bufio",
	"pkg/bytes",
	"pkg/encoding",
	"pkg/encoding/base64",
	"pkg/encoding/json",
	"pkg/errors",
	"pkg/flag",
	"pkg/fmt",
	"pkg/go/ast",
	"pkg/go/build",
	"pkg/go/doc",
	"pkg/go/parser",
	"pkg/go/scanner",
	"pkg/go/token",
	"pkg/io",
	"pkg/io/ioutil",
	"pkg/log",
	"pkg/math",
	"pkg/os",
	"pkg/os/exec",
	"pkg/path",
	"pkg/path/filepath",
	"pkg/reflect",
	"pkg/regexp",
	"pkg/regexp/syntax",
	"pkg/runtime",
	"pkg/sort",
	"pkg/strconv",
	"pkg/strings",
	"pkg/sync",
	"pkg/sync/atomic",
	"pkg/syscall",
	"pkg/text/template",
	"pkg/text/template/parse",
	"pkg/time",
	"pkg/unicode",
	"pkg/unicode/utf16",
	"pkg/unicode/utf8",
};

// caller:
//  1. cmdbootstrap()
//  2. cmdinstall()
void clean(void)
{
	int i, j, k;
	Buf b, path;
	Vec dir;

	binit(&b);
	binit(&path);
	vinit(&dir);

	for(i=0; i<nelem(cleantab); i++) {
		if((streq(cleantab[i], "cmd/prof")) && !isdir(cleantab[i])) {
			continue;
		}
		bpathf(&path, "%s/src/%s", goroot, cleantab[i]);
		xreaddir(&dir, bstr(&path));
		// Remove generated files.
		for(j=0; j<dir.len; j++) {
			for(k=0; k<nelem(gentab); k++) {
				if(hasprefix(dir.p[j], gentab[k].nameprefix)) {
					xremove(bpathf(&b, "%s/%s", bstr(&path), dir.p[j]));
				}
			}
		}
		// Remove generated binary named for directory.
		if(hasprefix(cleantab[i], "cmd/")) {
			xremove(bpathf(&b, "%s/%s", bstr(&path), cleantab[i]+4));
		}
	}

	// remove src/pkg/runtime/z* unconditionally
	vreset(&dir);
	bpathf(&path, "%s/src/pkg/runtime", goroot);
	xreaddir(&dir, bstr(&path));
	for(j=0; j<dir.len; j++) {
		if(hasprefix(dir.p[j], "z")) {
			xremove(bpathf(&b, "%s/%s", bstr(&path), dir.p[j]));
		}
	}

	if(rebuildall) {
		// Remove object tree.
		xremoveall(bpathf(&b, "%s/pkg/obj/%s_%s", goroot, gohostos, gohostarch));

		// Remove installed packages and tools.
		xremoveall(bpathf(&b, "%s/pkg/%s_%s", goroot, gohostos, gohostarch));
		xremoveall(bpathf(&b, "%s/pkg/%s_%s", goroot, goos, goarch));
		xremoveall(tooldir);

		// Remove cached version info.
		xremove(bpathf(&b, "%s/VERSION.cache", goroot));
	}

	bfree(&b);
	bfree(&path);
	vfree(&dir);
}

// Clean deletes temporary objects.
// Clean -i deletes the installed objects too.
void cmdclean(int argc, char **argv)
{
	ARGBEGIN{
	case 'v':
		vflag++;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0) {
		usage();
	}

	clean();
}

////////////////////////////////////////////////////////////////////////////////

// The env command prints the default environment.
void cmdenv(int argc, char **argv)
{
	bool pflag;
	char *sep;
	Buf b, b1;
	char *format;

	binit(&b);
	binit(&b1);

	format = "%s=\"%s\"\n";
	pflag = 0;
	ARGBEGIN{
	case '9':
		format = "%s='%s'\n";
		break;
	case 'p':
		pflag = 1;
		break;
	case 'v':
		vflag++;
		break;
	case 'w':
		format = "set %s=%s\r\n";
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0) {
		usage();
	}

	xprintf(format, "CC", defaultcc);
	xprintf(format, "GOROOT", goroot);
	xprintf(format, "GOBIN", gobin);
	xprintf(format, "GOARCH", goarch);
	xprintf(format, "GOOS", goos);
	xprintf(format, "GOHOSTARCH", gohostarch);
	xprintf(format, "GOHOSTOS", gohostos);
	xprintf(format, "GOTOOLDIR", tooldir);
	xprintf(format, "GOCHAR", gochar);
	if(streq(goarch, "386")) {
		xprintf(format, "GO386", go386);
	}

	if(pflag) {
		sep = ":";
		xgetenv(&b, "PATH");
		bprintf(&b1, "%s%s%s", gobin, sep, bstr(&b));
		xprintf(format, "PATH", bstr(&b1));
	}

	bfree(&b);
	bfree(&b1);
}

// Banner prints the 'now you've installed Go' banner.
void cmdbanner(int argc, char **argv)
{
	char *pathsep;
	Buf b, b1, search, path;

	ARGBEGIN{
	case 'v':
		vflag++;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0) {
		usage();
	}

	binit(&b);
	binit(&b1);
	binit(&search);
	binit(&path);

	xprintf("\n");
	xprintf("---\n");
	xprintf("Installed Go for %s/%s in %s\n", goos, goarch, goroot);
	xprintf("Installed commands in %s\n", gobin);

	if(!xsamefile(goroot_final, goroot)) {
		// If the files are to be moved, don't check that gobin
		// is on PATH; assume they know what they are doing.
	} else {
		// Check that gobin appears in $PATH.
		xgetenv(&b, "PATH");
		pathsep = ":";
		bprintf(&b1, "%s%s%s", pathsep, bstr(&b), pathsep);
		bprintf(&search, "%s%s%s", pathsep, gobin, pathsep);
		if(xstrstr(bstr(&b1), bstr(&search)) == nil) {
			xprintf("*** You need to add %s to your PATH.\n", gobin);
		}
	}

	if(!xsamefile(goroot_final, goroot)) {
		xprintf(
            "\n"
			"The binaries expect %s to be copied or moved to %s\n",
			goroot, goroot_final
        );
	}

	bfree(&b);
	bfree(&b1);
	bfree(&search);
	bfree(&path);
}

// Version prints the Go version.
void cmdversion(int argc, char **argv)
{
	ARGBEGIN{
	case 'v':
		vflag++;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0) {
		usage();
	}

	xprintf("%s\n", goversion);
}
