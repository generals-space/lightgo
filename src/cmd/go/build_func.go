package main

import (
	"path/filepath"
	"strings"
	"os"
	"bytes"
	"strconv"
	"fmt"
	"flag"
	"runtime"
)

// go build 构建的其中一种情况: 目标为 n 个 .go 文件, 为这些 .go 文件单独创建一个 main 包.
//
// 	@param gofiles: go build 构建目标的 n 个 .go 文件.
//
// caller:
// 	1. src/cmd/go/pkg.go -> packagesAndErrors()
//
// goFilesPackage creates a package for building a collection of Go files
// (typically named on the command line).  The target is named p.a for
// package p or named after the first Go file for package main.
func goFilesPackage(gofiles []string) *Package {
	// TODO: Remove this restriction.
	for _, f := range gofiles {
		if !strings.HasSuffix(f, ".go") {
			fatalf("named files must be .go files")
		}
	}

	var stk importStack
	ctxt := buildContext
	ctxt.UseAllFiles = true

	// Synthesize fake "directory" that only shows the named files,
	// to make it look like this is a standard package or
	// command directory.  So that local imports resolve
	// consistently, the files must all be in the same directory.
	var dirent []os.FileInfo
	var dir string
	for _, file := range gofiles {
		fi, err := os.Stat(file)
		if err != nil {
			fatalf("%s", err)
		}
		if fi.IsDir() {
			fatalf("%s is a directory, should be a Go file", file)
		}
		dir1, _ := filepath.Split(file)
		if dir == "" {
			dir = dir1
		} else if dir != dir1 {
			fatalf("named files must all be in one directory; have %s and %s", dir, dir1)
		}
		dirent = append(dirent, fi)
	}
	ctxt.ReadDir = func(string) ([]os.FileInfo, error) { return dirent, nil }

	if !filepath.IsAbs(dir) {
		dir = filepath.Join(cwd, dir)
	}

	bp, err := ctxt.ImportDir(dir, 0)
	pkg := new(Package)
	pkg.local = true
	pkg.cmdline = true
	pkg.load(&stk, bp, err)
	pkg.localPrefix = dirToImportPath(dir)
	pkg.ImportPath = "command-line-arguments"
	pkg.target = ""

	if pkg.Name == "main" {
		_, elem := filepath.Split(gofiles[0])
		exe := elem[:len(elem)-len(".go")] + exeSuffix
		if *buildO == "" {
			*buildO = exe
		}
		if gobin != "" {
			pkg.target = filepath.Join(gobin, exe)
		}
	} else {
		if *buildO == "" {
			*buildO = pkg.Name + ".a"
		}
	}
	pkg.Target = pkg.target
	pkg.Stale = true

	computeStale(pkg)
	return pkg
}

func isSpaceByte(c byte) bool {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r'
}

// fileExtSplit expects a filename and returns the name
// and ext (without the dot). If the file has no
// extension, ext will be empty.
func fileExtSplit(file string) (name, ext string) {
	dotExt := filepath.Ext(file)
	name = file[:len(file)-len(dotExt)]
	if dotExt != "" {
		ext = dotExt[1:]
	}
	return
}

// hasString reports whether s appears in the list of strings.
func hasString(strings []string, s string) bool {
	for _, t := range strings {
		if s == t {
			return true
		}
	}
	return false
}

// shortPath returns an absolute or relative name for path, whatever is shorter.
func shortPath(path string) string {
	if rel, err := filepath.Rel(cwd, path); err == nil && len(rel) < len(path) {
		return rel
	}
	return path
}

// relPaths returns a copy of paths with absolute paths
// made relative to the current directory if they would be shorter.
func relPaths(paths []string) []string {
	var out []string
	pwd, _ := os.Getwd()
	for _, p := range paths {
		rel, err := filepath.Rel(pwd, p)
		if err == nil && len(rel) < len(p) {
			p = rel
		}
		out = append(out, p)
	}
	return out
}

// caller:
// 	1. builder.runOut() 只有这一处
//
// joinUnambiguously prints the slice, quoting where necessary to make the
// output unambiguous.
// TODO: See issue 5279. The printing of commands needs a complete redo.
func joinUnambiguously(a []string) string {
	var buf bytes.Buffer
	for i, s := range a {
		if i > 0 {
			buf.WriteByte(' ')
		}
		q := strconv.Quote(s)
		if s == "" || strings.Contains(s, " ") || len(q) > len(s)+2 {
			buf.WriteString(q)
		} else {
			buf.WriteString(s)
		}
	}
	return buf.String()
}

// mkAbs returns an absolute path corresponding to
// evaluating f in the directory dir.
// We always pass absolute paths of source files so that
// the error messages will include the full path to a file
// in need of attention.
func mkAbs(dir, f string) string {
	// Leave absolute paths alone.
	// Also, during -n mode we use the pseudo-directory $WORK
	// instead of creating an actual work directory that won't be used.
	// Leave paths beginning with $WORK alone too.
	if filepath.IsAbs(f) || strings.HasPrefix(f, "$WORK") {
		return f
	}
	return filepath.Join(dir, f)
}

func envList(key string) []string {
	return strings.Fields(os.Getenv(key))
}

func raceInit() {
	if !buildRace {
		return
	}
	if goarch != "amd64" || goos != "linux" {
		fmt.Fprintf(os.Stderr, "go %s: -race is only supported on linux/amd64\n", flag.Args()[0])
		os.Exit(2)
	}
	buildGcflags = append(buildGcflags, "-race")
	buildLdflags = append(buildLdflags, "-race")
	buildCcflags = append(buildCcflags, "-D", "RACE")
	if buildContext.InstallSuffix != "" {
		buildContext.InstallSuffix += "_"
	}
	buildContext.InstallSuffix += "race"
	buildContext.BuildTags = append(buildContext.BuildTags, "race")
}

// defaultSuffix returns file extension used for command files in
// current os environment.
func defaultSuffix() string {
	switch runtime.GOOS {
	default:
		return ".bash"
	}
}
