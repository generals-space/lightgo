/*
 * build_func.c 由 build.c 拆分而来, build.c 只保留 bootstrap 核心流程
 * 
 * 20230609
 */

#include "a.h"
#include "arg.h"

// find reports the first index of p in l[0:n], or else -1.
int find(char *p, char **l, int n)
{
	int i;

	for(i=0; i<n; i++) {
		if(streq(p, l[i])) {
			return i;
		}
	}
	return -1;
}

// rmworkdir deletes the work directory.
void rmworkdir(void)
{
	if(vflag > 1) {
		errprintf("rm -rf %s\n", workdir);
	}
	xremoveall(workdir);
}

// Remove trailing spaces.
void chomp(Buf *b)
{
	int c;

	while(b->len > 0 && ((c=b->p[b->len-1]) == ' ' || c == '\t' || c == '\r' || c == '\n')) {
		b->len--;
	}
}

// matchfield reports whether the field matches this build.
bool matchfield(char *f)
{
	char *p;
	bool res;

	p = xstrrchr(f, ',');
	if(p == nil) {
		return streq(f, goos) || streq(f, goarch) || streq(f, "cmd_go_bootstrap") || streq(f, "go1.1");
	}
	*p = 0;
	res = matchfield(f) && matchfield(p+1);
	*p = ',';
	return res;
}

// shouldbuild reports whether we should build this file.
// It applies the same rules that are used with context tags
// in package go/build, except that the GOOS and GOARCH
// can appear anywhere in the file name, not just after _.
// In particular, they can be the entire file name (like windows.c).
// We also allow the special tag cmd_go_bootstrap.
// See ../go/bootstrap.go and package go/build.
bool shouldbuild(char *file, char *dir)
{
	char *name, *p;
	int i, j, ret;
	Buf b;
	Vec lines, fields;

	// Check file name for GOOS or GOARCH.
	name = lastelem(file);
	for(i=0; i<nelem(okgoos); i++) {
		if(contains(name, okgoos[i]) && !streq(okgoos[i], goos)) {
			return 0;
		}
	}
	for(i=0; i<nelem(okgoarch); i++) {
		if(contains(name, okgoarch[i]) && !streq(okgoarch[i], goarch)) {
			return 0;
		}
	}

	// Omit test files.
	if(contains(name, "_test")) {
		return 0;
	}

	// cmd/go/doc.go has a giant /* */ comment before
	// it gets to the important detail that it is not part of
	// package main.  We don't parse those comments,
	// so special case that file.
	if(hassuffix(file, "cmd/go/doc.go") || hassuffix(file, "cmd\\go\\doc.go")) {
		return 0;
	}
	if(hassuffix(file, "cmd/cgo/doc.go") || hassuffix(file, "cmd\\cgo\\doc.go")) {
		return 0;
	}

	// Check file contents for // +build lines.
	binit(&b);
	vinit(&lines);
	vinit(&fields);

	ret = 1;
	readfile(&b, file);
	splitlines(&lines, bstr(&b));
	for(i=0; i<lines.len; i++) {
		p = lines.p[i];
		while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
			p++;
		}
		if(*p == '\0') {
			continue;
		}
		if(contains(p, "package documentation")) {
			ret = 0;
			goto out;
		}
		if(contains(p, "package main") && !streq(dir, "cmd/go") && !streq(dir, "cmd/cgo")) {
			ret = 0;
			goto out;
		}
		if(!hasprefix(p, "//")) {
			break;
		}
		if(!contains(p, "+build")) {
			continue;
		}
		splitfields(&fields, lines.p[i]);
		if(fields.len < 2 || !streq(fields.p[1], "+build")) {
			continue;
		}
		for(j=2; j<fields.len; j++) {
			p = fields.p[j];
			if((*p == '!' && !matchfield(p+1)) || matchfield(p)) {
				goto fieldmatch;
			}
		}
		ret = 0;
		goto out;
	fieldmatch:;
	}

out:
	bfree(&b);
	vfree(&lines);
	vfree(&fields);

	return ret;
}

// copy copies the file src to dst, via memory (so only good for small files).
void copy(char *dst, char *src, int exec)
{
	Buf b;

	if(vflag > 1) {
		errprintf("cp %s %s\n", src, dst);
	}

	binit(&b);
	readfile(&b, src);
	writefile(&b, dst, exec);
	bfree(&b);
}
////////////////////////////////////////////////////////////////////////////////

/*
 * Initial tree setup.
 */

// The old tools that no longer live in $GOBIN or $GOROOT/bin.
static char *oldtool[] = {
	// "5a", "5c", "5g", "5l",
	"6a", "6c", "6g", "6l",
	// "8a", "8c", "8g", "8l",
	"6cov",
	"6nm",
	"6prof",
	"cgo",
	"ebnflint",
	"goapi",
	"gofix",
	"goinstall",
	"gomake",
	"gopack",
	"gopprof",
	"gotest",
	"gotype",
	"govet",
	"goyacc",
	"quietgcc",
};

// Unreleased directories (relative to $GOROOT) that should
// not be in release branches.
static char *unreleased[] = {
	"src/cmd/prof",
	"src/pkg/old",
};

// 好像只做了一些清理工作, 查看一下 bin, pkg/tools, pkg/linux_amd64 等目录, 
// 如果这些目录不存在则创建, 如果这此目录下有文件就删掉.
//
// caller:
// 	1. cmdbootstrap() 只有这一处
//
// setup sets up the tree for the initial build.
void setup(void)
{
	int i;
	Buf b;
	char *p;

	binit(&b);

	// Create bin directory.
	p = bpathf(&b, "%s/bin", goroot);
	if(!isdir(p)) {
		xmkdir(p);
	}

	// Create package directory.
	p = bpathf(&b, "%s/pkg", goroot);
	if(!isdir(p)) {
		xmkdir(p);
	}
	p = bpathf(&b, "%s/pkg/%s_%s", goroot, gohostos, gohostarch);
	if(rebuildall) {
		xremoveall(p);
	}
	xmkdirall(p);
	if(!streq(goos, gohostos) || !streq(goarch, gohostarch)) {
		p = bpathf(&b, "%s/pkg/%s_%s", goroot, goos, goarch);
		if(rebuildall) {
			xremoveall(p);
		}
		xmkdirall(p);
	}

	// Create object directory.
	// We keep it in pkg/ so that all the generated binaries
	// are in one tree.  If pkg/obj/libgc.a exists, it is a dreg from
	// before we used subdirectories of obj.  Delete all of obj
	// to clean up.
	bpathf(&b, "%s/pkg/obj/libgc.a", goroot);
	if(isfile(bstr(&b))) {
		xremoveall(bpathf(&b, "%s/pkg/obj", goroot));
	}
	p = bpathf(&b, "%s/pkg/obj/%s_%s", goroot, gohostos, gohostarch);
	if(rebuildall) {
		xremoveall(p);
	}
	xmkdirall(p);

	// Create tool directory.
	// We keep it in pkg/, just like the object directory above.
	if(rebuildall) {
		xremoveall(tooldir);
	}
	xmkdirall(tooldir);

	// Remove tool binaries from before the tool/gohostos_gohostarch
	xremoveall(bpathf(&b, "%s/bin/tool", goroot));

	// Remove old pre-tool binaries.
	for(i=0; i<nelem(oldtool); i++) {
		xremove(bpathf(&b, "%s/bin/%s", goroot, oldtool[i]));
	}

	// If $GOBIN is set and has a Go compiler, it must be cleaned.
	for(i=0; gochars[i]; i++) {
		if(isfile(bprintf(&b, "%s%s%c%s", gobin, slash, gochars[i], "g"))) {
			for(i=0; i<nelem(oldtool); i++) {
				xremove(bprintf(&b, "%s%s%s", gobin, slash, oldtool[i]));
			}
			break;
		}
	}

	// For release, make sure excluded things are excluded.
	if(hasprefix(goversion, "release.") || hasprefix(goversion, "go")) {
		for(i=0; i<nelem(unreleased); i++) {
			if(isdir(bpathf(&b, "%s/%s", goroot, unreleased[i]))) {
				fatal("%s should not exist in release build", bstr(&b));
			}
		}
	}

	bfree(&b);
}

////////////////////////////////////////////////////////////////////////////////

// findgoversion determines the Go version to use in the version string.
char* findgoversion(void)
{
	char *tag, *rev, *p;
	int i, nrev;
	Buf b, path, bmore, branch;
	Vec tags;

	binit(&b);
	binit(&path);
	binit(&bmore);
	binit(&branch);
	vinit(&tags);

	// The $GOROOT/VERSION file takes priority, for distributions
	// without the Mercurial repo.
	bpathf(&path, "%s/VERSION", goroot);
	if(isfile(bstr(&path))) {
		readfile(&b, bstr(&path));
		chomp(&b);
		// Commands such as "dist version > VERSION" will cause
		// the shell to create an empty VERSION file and set dist's
		// stdout to its fd. dist in turn looks at VERSION and uses
		// its content if available, which is empty at this point.
		if(b.len > 0)
			goto done;
	}

	// The $GOROOT/VERSION.cache file is a cache to avoid invoking
	// hg every time we run this command.  Unlike VERSION, it gets
	// deleted by the clean command.
	bpathf(&path, "%s/VERSION.cache", goroot);
	if(isfile(bstr(&path))) {
		readfile(&b, bstr(&path));
		chomp(&b);
		goto done;
	}

	// Otherwise, use Mercurial.
	// What is the current branch?
	run(&branch, goroot, CheckExit, "hg", "identify", "-b", nil);
	chomp(&branch);

	// What are the tags along the current branch?
	tag = "devel";
	rev = ".";
	run(&b, goroot, CheckExit, "hg", "log", "-b", bstr(&branch), "-r", ".:0", "--template", "{tags} + ", nil);
	splitfields(&tags, bstr(&b));
	nrev = 0;
	for(i=0; i<tags.len; i++) {
		p = tags.p[i];
		if(streq(p, "+")) {
			nrev++;
        }
		// NOTE: Can reenable the /* */ code when we want to
		// start reporting versions named 'weekly' again.
		if(/*hasprefix(p, "weekly.") ||*/ hasprefix(p, "go")) {
			tag = xstrdup(p);
			// If this tag matches the current checkout
			// exactly (no "+" yet), don't show extra
			// revision information.
			if(nrev == 0) {
				rev = "";
            }
			break;
		}
	}

	if(tag[0] == '\0') {
		// Did not find a tag; use branch name.
		bprintf(&b, "branch.%s", bstr(&branch));
		tag = btake(&b);
	}

	if(rev[0]) {
		// Tag is before the revision we're building.
		// Add extra information.
		run(
            &bmore, goroot, CheckExit, 
            "hg", "log", "--template", " +{node|short} {date|date}", "-r", rev, nil
        );
		chomp(&bmore);
	}

	bprintf(&b, "%s", tag);
	if(bmore.len > 0) {
		bwriteb(&b, &bmore);
	}

	// Cache version.
	writefile(&b, bstr(&path), 0);

done:
	p = btake(&b);

	bfree(&b);
	bfree(&path);
	bfree(&bmore);
	bfree(&branch);
	vfree(&tags);

	return p;
}
