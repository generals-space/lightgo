// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "a.h"
#include "arg.h"

/*
 * Initialization for any invocation.
 */

// The usual variables.
// 一个临时编译目录, 在这里存在编译过程中生成的 .o 文件. 如 /var/tmp/go-cbuild-XXXXXX
char *workdir;
char *goroot = GOROOT_FINAL;
char *goroot_final = GOROOT_FINAL;

char *gobin; 		// 一般为 ${GOROOT}/bin
char *tooldir; 		// 一般为 ${GOROOT}/pkg/tool/linux_amd64

char *goarch;		// 目标二进制的CPU架构, 如 amd64, arm 等
char *goos;			// 目标二进制的OS类型, 如 linux, windows 等
char *gochar; 		// 5l, 6l, 8l 中的前缀数字, 表示目标二进制的平台类型

char *gohostarch;	// 当前宿主机的CPU架构, 如 amd64, arm 等
char *gohostos;		// 当前宿主机的OS类型, 如 linux, windows 等
char *gohostchar; 	// 5l, 6l, 8l 中的前缀数字, 表示当前宿主机的平台类型

char *go386;
char *goextlinkenabled = "";
char *goversion;
char *slash;		// / for unix, \ for windows
char *defaultcc; 	// gcc
char *defaultcxx; 	// g++
bool	rebuildall;
bool defaultclang;

// The known architecture letters.
char *gochars = "68";
char *okgoarch[2] = {
	// same order as gochars
	"amd64",
	"386",
};

// The known operating systems.
char *okgoos[1] = {
	"linux",
};

// caller:
// 	1. src/cmd/dist/unix.c -> main()
//
// init handles initialization of the various global state, like goroot and goarch.
void init(void)
{
	char *p;
	int i;
	Buf b;

	binit(&b);

	xgetenv(&b, "GOROOT");
	if(b.len > 0) {
		// if not "/", then strip trailing path separator
		if(b.len >= 2 && b.p[b.len - 1] == slash[0]) {
			b.len--;
		}
		goroot = btake(&b);
	}

	xgetenv(&b, "GOBIN");
	if(b.len == 0) {
		bprintf(&b, "%s%sbin", goroot, slash);
	}
	gobin = btake(&b);

	xgetenv(&b, "GOOS");
	if(b.len == 0) {
		bwritestr(&b, gohostos);
	}
	goos = btake(&b);
	if(find(goos, okgoos, nelem(okgoos)) < 0) {
		fatal("unknown $GOOS %s", goos);
	}

	xgetenv(&b, "GO386");
	if(b.len == 0) {
		if(cansse2()) {
			bwritestr(&b, "sse2");
		}
		else {
			bwritestr(&b, "387");
		}
	}
	go386 = btake(&b);

	p = bpathf(&b, "%s/include/u.h", goroot);
	if(!isfile(p)) {
		fatal(
			"$GOROOT is not set correctly or not exported\n"
			"\tGOROOT=%s\n"
			"\t%s does not exist", 
			goroot, p
		);
	}

	xgetenv(&b, "GOHOSTARCH");
	if(b.len > 0) {
		gohostarch = btake(&b);
	}

	i = find(gohostarch, okgoarch, nelem(okgoarch));
	if(i < 0) {
		fatal("unknown $GOHOSTARCH %s", gohostarch);
	}
	bprintf(&b, "%c", gochars[i]);
	gohostchar = btake(&b);

	xgetenv(&b, "GOARCH");
	if(b.len == 0) {
		bwritestr(&b, gohostarch);
	}
	goarch = btake(&b);
	i = find(goarch, okgoarch, nelem(okgoarch));
	if(i < 0) {
		fatal("unknown $GOARCH %s", goarch);
	}
	bprintf(&b, "%c", gochars[i]);
	gochar = btake(&b);

	xgetenv(&b, "GO_EXTLINK_ENABLED");
	if(b.len > 0) {
		goextlinkenabled = btake(&b);
		if(!streq(goextlinkenabled, "0") && !streq(goextlinkenabled, "1")) {
			fatal("unknown $GO_EXTLINK_ENABLED %s", goextlinkenabled);
		}
	}
	
	xgetenv(&b, "CC");
	if(b.len == 0) {
		// Use clang on OS X, because gcc is deprecated there.
		// Xcode for OS X 10.9 Mavericks will ship a fake "gcc" binary that
		// actually runs clang. We prepare different command
		// lines for the two binaries, so it matters what we call it.
		// See golang.org/issue/5822.
		if(defaultclang) {
			bprintf(&b, "clang");
		}
		else {
			bprintf(&b, "gcc");
		}
	}
	defaultcc = btake(&b);

	xgetenv(&b, "CXX");
	if(b.len == 0) {
		if(defaultclang) {
			bprintf(&b, "clang++");
		}
		else {
			bprintf(&b, "g++");
		}
	}
	defaultcxx = btake(&b);

	xsetenv("GOROOT", goroot);
	xsetenv("GOARCH", goarch);
	xsetenv("GOOS", goos);
	xsetenv("GO386", go386);

	// Make the environment more predictable.
	xsetenv("LANG", "C");
	xsetenv("LANGUAGE", "en_US.UTF8");

	goversion = findgoversion();

	workdir = xworkdir();
	xatexit(rmworkdir);

	bpathf(&b, "%s/pkg/tool/%s_%s", goroot, gohostos, gohostarch);
	tooldir = btake(&b);

	bfree(&b);
}

/*
 * C library and tool building
 */

// gccargs is the gcc command line to use for compiling a single C file.
static char *proto_gccargs[] = {
	"-Wall",
	// native Plan 9 compilers don't like non-standard prototypes
	// so let gcc catch them.
	"-Wstrict-prototypes",
	"-Wextra",
	"-Wunused",
	"-Wuninitialized",
	"-Wno-sign-compare",
	"-Wno-missing-braces",
	"-Wno-parentheses",
	"-Wno-unknown-pragmas",
	"-Wno-switch",
	"-Wno-comment",
	"-Wno-missing-field-initializers",
	"-Werror",
	"-fno-common",
	"-ggdb",
	"-pipe",
#if defined(__NetBSD__) && defined(__arm__)
	// GCC 4.5.4 (NetBSD nb1 20120916) on ARM is known to mis-optimize gc/mparith3.c
	// Fix available at http://patchwork.ozlabs.org/patch/64562/.
	"-O1",
#else
	// "-O2",
#endif
};

static Vec gccargs;

// deptab lists changes to the default dependencies for a given prefix.
// deps ending in /* read the whole directory; deps beginning with -
// exclude files with that prefix.
static struct {
	char *prefix;  // prefix of target
	char *dep[20];  // dependency tweaks for targets with that prefix
} deptab[] = {
	{"lib9", {
		"$GOROOT/include/u.h",
		"$GOROOT/include/utf.h",
		"$GOROOT/include/fmt.h",
		"$GOROOT/include/libc.h",
		"fmt/*",
		"utf/*",
	}},
	{"libbio", {
		"$GOROOT/include/u.h",
		"$GOROOT/include/utf.h",
		"$GOROOT/include/fmt.h",
		"$GOROOT/include/libc.h",
		"$GOROOT/include/bio.h",
	}},
	{"libmach", {
		"$GOROOT/include/u.h",
		"$GOROOT/include/utf.h",
		"$GOROOT/include/fmt.h",
		"$GOROOT/include/libc.h",
		"$GOROOT/include/bio.h",
		"$GOROOT/include/ar.h",
		"$GOROOT/include/bootexec.h",
		"$GOROOT/include/mach.h",
		"$GOROOT/include/ureg_amd64.h",
		"$GOROOT/include/ureg_x86.h",
	}},
	{"cmd/cc", {
		"-pgen.c",
		"-pswt.c",
	}},
	{"cmd/gc", {
		"-cplx.c",
		"-pgen.c",
		"-popt.c",
		"-y1.tab.c",  // makefile dreg
		"opnames.h",
	}},
	{"cmd/6c", {
		"../cc/pgen.c",
		"../cc/pswt.c",
		"../6l/enam.c",
		"$GOROOT/pkg/obj/$GOOS_$GOARCH/libcc.a",
	}},
	{"cmd/6g", {
		"../gc/cplx.c",
		"../gc/pgen.c",
		"../gc/popt.c",
		"../gc/popt.h",
		"../6l/enam.c",
		"$GOROOT/pkg/obj/$GOOS_$GOARCH/libgc.a",
	}},
	{"cmd/6l", {
		"../ld/*",
		"enam.c",
	}},
	{"cmd/go", {
		"zdefaultcc.go",
	}},
	{"cmd/", {
		"$GOROOT/pkg/obj/$GOOS_$GOARCH/libmach.a",
		"$GOROOT/pkg/obj/$GOOS_$GOARCH/libbio.a",
		"$GOROOT/pkg/obj/$GOOS_$GOARCH/lib9.a",
	}},
	{"pkg/runtime", {
		"zaexperiment.h", // must sort above zasm
		"zasm_$GOOS_$GOARCH.h",
		"zsys_$GOOS_$GOARCH.s",
		"zgoarch_$GOARCH.go",
		"zgoos_$GOOS.go",
		"zruntime_defs_$GOOS_$GOARCH.go",
		"zversion.go",
	}},
};

// depsuffix records the allowed suffixes for source files.
char *depsuffix[] = {
	".c",
	".h",
	".s",
	".go",
	".goc",
};

// gentab records how to generate some trivial files.
struct gentab_struct gentab[10] = {
	{"opnames.h", gcopnames},
	{"enam.c", mkenam},
	{"zasm_", mkzasm},
	{"zdefaultcc.go", mkzdefaultcc},
	{"zsys_", mkzsys},
	{"zgoarch_", mkzgoarch},
	{"zgoos_", mkzgoos},
	{"zruntime_defs_", mkzruntimedefs},
	{"zversion.go", mkzversion},
	{"zaexperiment.h", mkzexperiment},
};

// buildorder 这里表示 golang 的核心工具及标准库, 在 make 的第2阶段开始构建.
//
// 见"# Building compilers and Go bootstrap tool for host"一节
//
// buildorder records the order of builds for the 'go bootstrap' command.
static char *buildorder[] = {
	"lib9",
	"libbio",
	"libmach",

	"misc/pprof",

	"cmd/addr2line",
	"cmd/nm",
	"cmd/objdump",
	"cmd/pack",
	"cmd/prof",

	"cmd/cc",  // must be before c
	"cmd/gc",  // must be before g
	"cmd/%sl",  // must be before a, c, g
	"cmd/%sa",
	"cmd/%sc",
	"cmd/%sg",

	// The dependency order here was copied from a buildscript
	// back when there were build scripts. 
	// Will have to be maintained by hand, but shouldn't change very often.
	"pkg/runtime",
	"pkg/internal/errors",
	"pkg/sync/atomic",
	"pkg/sync",
	"pkg/io",
	"pkg/unicode",
	"pkg/unicode/utf8",
	"pkg/unicode/utf16",
	"pkg/bytes",
	"pkg/math",
	"pkg/strings",
	"pkg/strconv",
	"pkg/bufio",
	"pkg/sort",
	"pkg/container/heap",
	"pkg/encoding/base64",
	"pkg/syscall",
	"pkg/time",
	"pkg/os",
	"pkg/reflect",
	"pkg/errors",
	"pkg/fmt",
	"pkg/encoding",
	"pkg/encoding/json",
	"pkg/flag",
	"pkg/path/filepath",
	"pkg/path",
	"pkg/io/ioutil",
	"pkg/log",
	"pkg/regexp/syntax",
	"pkg/regexp",
	"pkg/go/token",
	"pkg/go/scanner",
	"pkg/go/ast",
	"pkg/go/parser",
	"pkg/os/exec",
	"pkg/os/signal",
	"pkg/net/url",
	"pkg/text/template/parse",
	"pkg/text/template",
	"pkg/go/doc",
	"pkg/go/build",
	"cmd/go",
};

// 每次调用都构建一个目录.
//
// 	@param *dir: buildorder[]数组中的成员
//
// caller:
// 	1. cmdbootstrap() make.bash 的第2阶段被调用, 使用 dist 构建 6c,6g,6l,go_bootstrap
// 	等命令工具及核心标准库.
// 	2. cmdinstall()
//
// install installs the library, package, or binary associated with dir,
// which is relative to $GOROOT/src.
void install(char *dir)
{
	// name 当前正在编译的源文件名称(只有文件名, 没有路径)
	char *name;
	char *p, *elem, *prefix;
	bool islib, ispkg, isgo, stale;
	Buf b, b1, path;
	// 存储在编译过程中生成的, 待清理的文件列表.
	// 在本函数末尾会有清理的语句.
	Vec clean;
	Vec compile, files, link, go, missing, lib, extra;
	Time ttarg, t;
	int i, j, k, n, doclean, targ;
	// _i 用于 for{} 循环打印调试信息的计数器, 不干预原有变量;

	if(vflag) {
		if(!streq(goos, gohostos) || !streq(goarch, gohostarch)) {
			errprintf("%s (%s/%s)\n", dir, goos, goarch);
		}
		else {
			errprintf("%s\n", dir);
		}
	}

	// 初始化 Buf 结构体
	binit(&b);
	binit(&b1);
	binit(&path);
	// 初始化 Vec 结构体
	vinit(&compile);
	vinit(&files);
	vinit(&link);
	vinit(&go);
	vinit(&missing);
	vinit(&clean);
	vinit(&lib);
	vinit(&extra);


	// path 就是 dir 的绝对路径.
	bpathf(&path, "%s/src/%s", goroot, dir);
	name = lastelem(dir);

	// For misc/prof, copy into the tool directory and we're done.
	if(hasprefix(dir, "misc/")) {
		copy(
			bpathf(&b, "%s/%s", tooldir, name),
			bpathf(&b1, "%s/misc/%s", goroot, name), 
			1
		);
		goto out;
	}

	// For release, cmd/prof is not included.
	if((streq(dir, "cmd/prof")) && !isdir(bstr(&path))) {
		if(vflag > 1) {
			errprintf("skipping %s - does not exist\n", dir);
		}
		goto out;
	}

	// set up gcc command line on first run.
	if(gccargs.len == 0) {
		bprintf(&b, "%s", defaultcc);
		splitfields(&gccargs, bstr(&b));
		for(i=0; i<nelem(proto_gccargs); i++) {
			vadd(&gccargs, proto_gccargs[i]);
		}
		if(contains(gccargs.p[0], "clang")) {
			// disable ASCII art in clang errors, if possible
			vadd(&gccargs, "-fno-caret-diagnostics");
			// clang is too smart about unused command-line arguments
			vadd(&gccargs, "-Qunused-arguments");
		}
	}

	islib = hasprefix(dir, "lib") || streq(dir, "cmd/cc") || streq(dir, "cmd/gc");
	ispkg = hasprefix(dir, "pkg");
	isgo = ispkg || streq(dir, "cmd/go") || streq(dir, "cmd/cgo");

	// Start final link command line.
	// Note: code below knows that link.p[targ] is the target.
	if(islib) {
		// C library.
		vadd(&link, "ar");
		vadd(&link, "rsc");
		prefix = "";
		if(!hasprefix(name, "lib")) {
			prefix = "lib";
		}
		targ = link.len;
		vadd(&link, bpathf(&b, "%s/pkg/obj/%s_%s/%s%s.a", goroot, gohostos, gohostarch, prefix, name));
	} else if(ispkg) {
		// Go library (package).
		vadd(&link, bpathf(&b, "%s/pack", tooldir));
		vadd(&link, "grc");
		// dir+4 指的是 pkg/ 目录后的子目录名称
		p = bprintf(&b, "%s/pkg/%s_%s/%s", goroot, goos, goarch, dir+4);
		// 假如 p 为 /root/go/pkg/linux_amd64/runtime,
		// 执行下一句后, 会变为 /root/go/pkg/linux_amd64
		*xstrrchr(p, '/') = '\0';
		xmkdirall(p);
		targ = link.len;
		vadd(&link, bpathf(&b, "%s/pkg/%s_%s/%s.a", goroot, goos, goarch, dir+4));
	} else if(streq(dir, "cmd/go") || streq(dir, "cmd/cgo")) {
		// Go command.
		vadd(&link, bpathf(&b, "%s/%sl", tooldir, gochar));
		vadd(&link, "-o");
		elem = name;
		if(streq(elem, "go")) {
			elem = "go_bootstrap";
		}
		targ = link.len;
		vadd(&link, bpathf(&b, "%s/%s", tooldir, elem));
	} else {
		// 这里构建 6c,6g,6l 等命令

		// C command. Use gccargs.
		vcopy(&link, gccargs.p, gccargs.len);
		if(sflag) {
			vadd(&link, "-static");
		}
		vadd(&link, "-o");
		targ = link.len;
		vadd(&link, bpathf(&b, "%s/%s", tooldir, name));
		if(streq(gohostarch, "amd64")) {
			vadd(&link, "-m64"); // 64位
		}
		else if(streq(gohostarch, "386")) {
			vadd(&link, "-m32"); // 32位
		}
	}
	ttarg = mtime(link.p[targ]);

	// 遍历目标 path 目录下的所有文件, 添加到 files 数组中.
	//
	// Gather files that are sources for this target.
	// Everything in that directory, and any target-specific additions.
	xreaddir(&files, bstr(&path));

	// Remove files beginning with . or _,
	// which are likely to be editor temporary files.
	// This is the same heuristic build.ScanDir uses.
	// There do exist real C files beginning with _,
	// so limit that check to just Go files.
	n = 0;
	for(i=0; i<files.len; i++) {
		p = files.p[i];
		if(hasprefix(p, ".") || (hasprefix(p, "_") && hassuffix(p, ".go"))) {
			xfree(p);
		}
		else {
			files.p[n++] = p;
		}
	}
	files.len = n;

	for(i=0; i<nelem(deptab); i++) {
		if(hasprefix(dir, deptab[i].prefix)) {
			for(j=0; (p=deptab[i].dep[j])!=nil; j++) {
				breset(&b1);
				bwritestr(&b1, p);
				bsubst(&b1, "$GOROOT", goroot);
				bsubst(&b1, "$GOOS", goos);
				bsubst(&b1, "$GOARCH", goarch);
				p = bstr(&b1);
				if(hassuffix(p, ".a")) {
					vadd(&lib, bpathf(&b, "%s", p));
					continue;
				}
				if(hassuffix(p, "/*")) {
					bpathf(&b, "%s/%s", bstr(&path), p);
					b.len -= 2;
					xreaddir(&extra, bstr(&b));
					bprintf(&b, "%s", p);
					b.len -= 2;
					for(k=0; k<extra.len; k++) {
						vadd(&files, bpathf(&b1, "%s/%s", bstr(&b), extra.p[k]));
					}
					continue;
				}
				if(hasprefix(p, "-")) {
					p++;
					n = 0;
					for(k=0; k<files.len; k++) {
						if(hasprefix(files.p[k], p)) {
							xfree(files.p[k]);
						}
						else {
							files.p[n++] = files.p[k];
						}
					}
					files.len = n;
					continue;
				}
				vadd(&files, p);
			}
		}
	}
	vuniq(&files);

	// Convert to absolute paths.
	for(i=0; i<files.len; i++) {
		if(!isabs(files.p[i])) {
			bpathf(&b, "%s/%s", bstr(&path), files.p[i]);
			xfree(files.p[i]);
			files.p[i] = btake(&b);
		}
	}

	// 下面一段是将未发生变动的文件从编译目标中移除吧, 为了加快编译速度?
	// Is the target up-to-date?
	stale = rebuildall;
	n = 0;
	for(i=0; i<files.len; i++) {
		p = files.p[i];
		// 只编译 .c, .h .s, .go 等后缀的源文件, 其他的像 Makefile, .md 等直接略过
		for(j=0; j<nelem(depsuffix); j++) {
			if(hassuffix(p, depsuffix[j])) {
				goto ok;
			}
		}
		xfree(files.p[i]);
		continue;
	ok:
		t = mtime(p);
		if(t != 0 && !hassuffix(p, ".a") && !shouldbuild(p, dir)) {
			xfree(files.p[i]);
			continue;
		}
		// 将 .go 文件单独存在 go 列表中.
		if(hassuffix(p, ".go")) {
			vadd(&go, p);
		}
		if(t > ttarg) {
			stale = 1;
		}
		if(t == 0) {
			vadd(&missing, p);
			files.p[n++] = files.p[i];
			continue;
		}
		files.p[n++] = files.p[i];
	} // for() end
	files.len = n;

	// If there are no files to compile, we're done.
	if(files.len == 0) {
		goto out;
	}

	for(i=0; i<lib.len && !stale; i++) {
		if(mtime(lib.p[i]) > ttarg) {
			stale = 1;
		}
	}

	if(!stale) {
		// 如果没有发生变动的文件, 直接退出, 不用编译.
		goto out;
	}

	// For package runtime, copy some files into the work space.
	// 在编译 pkg/runtime 包的时候, 还需要再拷贝一些头文件到 workdir.
	if(streq(dir, "pkg/runtime")) {
		copy(
			bpathf(&b, "%s/arch_amd64.h", workdir),
			bpathf(&b1, "%s/arch_%s.h", bstr(&path), goarch), 
			0
		);
		copy(
			bpathf(&b, "%s/defs_GOOS_GOARCH.h", workdir),
			bpathf(&b1, "%s/defs_%s_%s.h", bstr(&path), goos, goarch), 
			0
		);
		p = bpathf(&b1, "%s/signal_%s_%s.h", bstr(&path), goos, goarch);
		if(isfile(p)) {
			copy(bpathf(&b, "%s/signal_GOOS_GOARCH.h", workdir), p, 0);
		}
		copy(
			bpathf(&b, "%s/os_GOOS.h", workdir),
			bpathf(&b1, "%s/os_%s.h", bstr(&path), goos), 
			0
		);
		copy(
			bpathf(&b, "%s/signals_GOOS.h", workdir),
			bpathf(&b1, "%s/signals_%s.h", bstr(&path), goos), 
			0
		);
	}

	// Generate any missing files; regenerate existing ones.
	for(i=0; i<files.len; i++) {
		p = files.p[i];
		elem = lastelem(p);
		for(j=0; j<nelem(gentab); j++) {
			if(hasprefix(elem, gentab[j].nameprefix)) {
				if(vflag > 1) {
					errprintf("generate %s\n", p);
				}
				gentab[j].gen(bstr(&path), p);
				// Do not add generated file to clean list.
				// In pkg/runtime, we want to be able to
				// build the package with the go tool,
				// and it assumes these generated files already
				// exist (it does not know how to build them).
				// The 'clean' command can remove
				// the generated files.
				goto built;
			}
		}
		// Did not rebuild p.
		if(find(p, missing.p, missing.len) >= 0) {
			fatal("missing file %s", p);
		}
	built:;
	}

	// One more copy for package runtime.
	// The last batch was required for the generators.
	// This one is generated.
	if(streq(dir, "pkg/runtime")) {
		copy(
			bpathf(&b, "%s/zasm_GOOS_GOARCH.h", workdir),
			bpathf(&b1, "%s/zasm_%s_%s.h", bstr(&path), goos, goarch), 
			0
		);
	}

	// 在编译 pkg/runtime 时, 先将 .goc 文件转换成 .c 文件
	// Generate .c files from .goc files.
	if(streq(dir, "pkg/runtime")) {
		for(i=0; i<files.len; i++) {
			p = files.p[i];
			if(!hassuffix(p, ".goc")) {
				continue;
			}
			// b = path/zp but with _goos_goarch.c instead of .goc
			bprintf(&b, "%s%sz%s", bstr(&path), slash, lastelem(p));
			b.len -= 4;
			bwritef(&b, "_%s_%s.c", goos, goarch);
			goc2c(p, bstr(&b));
			vadd(&files, bstr(&b));
		}
		vuniq(&files);
	}

	if((!streq(goos, gohostos) || !streq(goarch, gohostarch)) && isgo) {
		// We've generated the right files; the go command can do the build.
		if(vflag > 1) {
			errprintf("skip build for cross-compile %s\n", dir);
		}
		goto nobuild;
	}

	// 1. 编译(然后才是链接)
	// Compile the files.
	for(i=0; i<files.len; i++) {
		if(!hassuffix(files.p[i], ".c") && !hassuffix(files.p[i], ".s")) {
			// 只编译 .c 和 .s 文件
			continue;
		}
		// name 为待编译的源文件名称
		name = lastelem(files.p[i]);

		vreset(&compile);
		if(!isgo) {
			// 这里构建 6c,6g,6l 等命令
			// C library or tool.
			vcopy(&compile, gccargs.p, gccargs.len);
			vadd(&compile, "-c");
			if(streq(gohostarch, "amd64")) {
				vadd(&compile, "-m64");
			}
			else if(streq(gohostarch, "386")) {
				vadd(&compile, "-m32");
			}
			if(streq(dir, "lib9")) {
				vadd(&compile, "-DPLAN9PORT");
			}

			vadd(&compile, "-I");
			vadd(&compile, bpathf(&b, "%s/include", goroot));

			vadd(&compile, "-I");
			vadd(&compile, bstr(&path));

			// lib9/goos.c gets the default constants hard-coded.
			if(streq(name, "goos.c")) {
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b, "GOOS=\"%s\"", goos));
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b, "GOARCH=\"%s\"", goarch));
				bprintf(&b1, "%s", goroot_final);
				bsubst(&b1, "\\", "\\\\");  // turn into C string
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b, "GOROOT=\"%s\"", bstr(&b1)));
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b, "GOVERSION=\"%s\"", goversion));
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b, "GO386=\"%s\"", go386));
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b, "GO_EXTLINK_ENABLED=\"%s\"", goextlinkenabled));
			}

			// gc/lex.c records the GOEXPERIMENT setting used during the build.
			if(streq(name, "lex.c")) {
				xgetenv(&b, "GOEXPERIMENT");
				vadd(&compile, "-D");
				vadd(&compile, bprintf(&b1, "GOEXPERIMENT=\"%s\"", bstr(&b)));
			}
		} else {
			// 对于 .s 源文件, 使用 6a 命令编译
			// 对于 .c 源文件, 使用 6c 命令编译

			// Supporting files for a Go package.
			if(hassuffix(files.p[i], ".s")) {
				vadd(&compile, bpathf(&b, "%s/%sa", tooldir, gochar));
			}
			else {
				vadd(&compile, bpathf(&b, "%s/%sc", tooldir, gochar));
				vadd(&compile, "-F");
				vadd(&compile, "-V");
				vadd(&compile, "-w");
				vadd(&compile, "-N");
				// 使用 C语言预处理器(CPP), 不过会报错
				// vadd(&compile, "-p");
			}
			vadd(&compile, "-I");
			vadd(&compile, workdir);
			vadd(&compile, "-I");
			vadd(&compile, bprintf(&b, "%s/pkg/%s_%s", goroot, goos, goarch));
			vadd(&compile, "-D");
			vadd(&compile, bprintf(&b, "GOOS_%s", goos));
			vadd(&compile, "-D");
			vadd(&compile, bprintf(&b, "GOARCH_%s", goarch));
			vadd(&compile, "-D");
			vadd(&compile, bprintf(&b, "GOOS_GOARCH_%s_%s", goos, goarch));
		}

		// 运行到此处, compile 中的命令为 
		// /root/go/pkg/tool/linux_amd64/6c -F -V -w -N 
		// -I /var/tmp/go-cbuild-juDAqI 
		// -I /root/go/pkg/linux_amd64 
		// -D GOOS_linux -D GOARCH_amd64 -D GOOS_GOARCH_linux_amd64

		bpathf(&b, "%s/%s", workdir, lastelem(files.p[i]));
		doclean = 1;

		// Change the last character of the output file (which was c or s).
		// 在 linux 系统中, 将源文件的的 .c 后缀改为 .o 作为输出对象.
		// 如: 6c main.c -o main.o
		b.p[b.len-1] = 'o';

		vadd(&compile, "-o");
		vadd(&compile, bstr(&b)); // 这里是待输出的 .o 目标文件的路径.
		vadd(&compile, files.p[i]);

		// 这里根据待编译目录下的不同源文件, 生成不同的 .c 和 .o 文件, 如
		// -o /var/tmp/go-cbuild-juDAqI/alg.o /root/go/src/pkg/runtime/alg.c
		// xprintf("compile path: %s, cmd: ", path.p);
		// for(_i=0; _i<compile.len; _i++) {
		// 	xprintf("%s ", compile.p[_i]);
		// }
		// xprintf("\n");

		bgrunv(bstr(&path), CheckExit, &compile);

		vadd(&link, bstr(&b));
		if(doclean) {
			vadd(&clean, bstr(&b));
		}
	}
	// 等待编译过程完成.
	bgwait();

	if(isgo) {
		// 重新初始化 compile 列表
		//
		// The last loop was compiling individual files.
		// Hand the Go files to the compiler en masse.
		vreset(&compile);
		vadd(&compile, bpathf(&b, "%s/%sg", tooldir, gochar));

		bpathf(&b, "%s/_go_.%s", workdir, gochar);
		vadd(&compile, "-o");
		vadd(&compile, bstr(&b));
		vadd(&clean, bstr(&b));
		vadd(&link, bstr(&b));

		vadd(&compile, "-p");
		if(hasprefix(dir, "pkg/")) {
			// dir+4 指的是 pkg/ 目录后的子目录名称
			vadd(&compile, dir+4);
		}
		else {
			vadd(&compile, "main");
		}

		if(streq(dir, "pkg/runtime")) {
			vadd(&compile, "-+");
		}

		// 运行到这里, compile 的值为
		// /root/go/pkg/tool/linux_amd64/6g -o /var/tmp/go-cbuild-juDAqI/_go_.6 -p runtime -+

		// 这里的语句会向 compile 后继续追加一些参数(这里是一些待编译目录下的 .go 文件)
		// 如 /root/go/src/pkg/runtime/{compiler.go, debug.go, error.go..}
		vcopy(&compile, go.p, go.len);

		// 执行 6g 命令, 最终会在 workdir 临时目录, 生成 _go_.6 文件.
		runv(nil, bstr(&path), CheckExit, &compile);
	}

	if(!islib && !isgo) {
		// C binaries need the libraries explicitly, and -lm.
		vcopy(&link, lib.p, lib.len);
		vadd(&link, "-lm");
	}

	// Remove target before writing it.
	xremove(link.p[targ]);

	// 开始链接过程, 会在 /root/go/pkg/linux_amd64/ 下生成 runtime.a 文件.
	// /root/go/pkg/tool/linux_amd64/pack grc /root/go/pkg/linux_amd64/runtime.a /var/tmp/go-cbuild-XXXXX/各种 .o 文件 /var/tmp/go-cbuild-juDAqI/_go_.6
	runv(nil, nil, CheckExit, &link);

nobuild:
	// In package runtime, we install runtime.h and cgocall.h too,
	// for use by cgo compilation.
	if(streq(dir, "pkg/runtime")) {
		copy(
			bpathf(&b, "%s/pkg/%s_%s/cgocall.h", goroot, goos, goarch),
			bpathf(&b1, "%s/src/pkg/runtime/cgocall.h", goroot), 
			0
		);
		copy(
			bpathf(&b, "%s/pkg/%s_%s/runtime.h", goroot, goos, goarch),
			bpathf(&b1, "%s/src/pkg/runtime/runtime.h", goroot), 
			0
		);
	}

out:
	// 清理编译过程中生成的临时文件.
	for(i=0; i<clean.len; i++) {
		xremove(clean.p[i]);
	}

	bfree(&b);
	bfree(&b1);
	bfree(&path);
	vfree(&compile);
	vfree(&files);
	vfree(&link);
	vfree(&go);
	vfree(&missing);
	vfree(&clean);
	vfree(&lib);
	vfree(&extra);
}

// 构建并生成 pkg/tool/linux_amd64{6g, 6l, 6c, go_bootstrap} 等工具.
// 每调用一次 install() 方法, 就构建1个命令.
//
// caller:
// 	1. src/cmd/dist/main.c -> bootstrap 子命令的入口函数.
//
// The bootstrap command runs a build from scratch,
// stopping at having installed the go_bootstrap command.
void cmdbootstrap(int argc, char **argv)
{
	int i;
	Buf b;
	char *oldgoos, *oldgoarch, *oldgochar;

	binit(&b);

	ARGBEGIN{
	case 'a':
		rebuildall = 1;
		break;
	case 's':
		sflag++;
		break;
	case 'v':
		// 每出现一次 -v 参数, 就会加1, 如 -vvv 时, 该值为 3, 值越大, 日志越详细.
		vflag++;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0) {
		usage();
	}

	if(rebuildall) {
		clean();
	}
	goversion = findgoversion();
	setup();

	xsetenv("GOROOT", goroot);
	xsetenv("GOROOT_FINAL", goroot_final);

	// For the main bootstrap, building for host os/arch.
	oldgoos = goos;
	oldgoarch = goarch;
	oldgochar = gochar;
	goos = gohostos;
	goarch = gohostarch;
	gochar = gohostchar;
	xsetenv("GOARCH", goarch);
	xsetenv("GOOS", goos);

	for(i=0; i<nelem(buildorder); i++) {
		install(bprintf(&b, buildorder[i], gohostchar));
		// 这里应该与buildorder[]{"cmd/%sl", "cmd/%sc", "cmd/%sg"} 中的成员有关.
		// xstrstr() 判断目标字符串中是否存在 "%s" 字符.
		if(!streq(oldgochar, gohostchar) && xstrstr(buildorder[i], "%s")) {
			install(bprintf(&b, buildorder[i], oldgochar));
		}
	}

	goos = oldgoos;
	goarch = oldgoarch;
	gochar = oldgochar;
	xsetenv("GOARCH", goarch);
	xsetenv("GOOS", goos);

	// Build pkg/runtime for actual goos/goarch too.
	if(!streq(goos, gohostos) || !streq(goarch, gohostarch)) {
		install("pkg/runtime");
	}

	bfree(&b);
}
