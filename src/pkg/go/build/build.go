// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package build

import (
	"bytes"
	"errors"
	"fmt"
	"go/ast"
	"go/doc"
	"go/parser"
	"go/token"
	"io"
	"io/ioutil"
	"log"
	"os"
	pathpkg "path"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"unicode"
)

// A Context specifies the supporting context for a build.
type Context struct {
	GOARCH      string // target architecture
	GOOS        string // target operating system
	GOROOT      string // Go root
	GOPATH      string // Go path
	CgoEnabled  bool   // whether cgo can be used
	UseAllFiles bool   // use files regardless of +build lines, file names
	Compiler    string // compiler to assume when computing target paths

	// The build and release tags specify build constraints
	// that should be considered satisfied when processing +build lines.
	// Clients creating a new context may customize BuildTags, which
	// defaults to empty, but it is usually an error to customize ReleaseTags,
	// which defaults to the list of Go releases the current release is compatible with.
	// In addition to the BuildTags and ReleaseTags, build constraints
	// consider the values of GOARCH and GOOS as satisfied tags.
	BuildTags   []string
	ReleaseTags []string

	// The install suffix specifies a suffix to use in the name of the installation
	// directory. By default it is empty, but custom builds that need to keep
	// their outputs separate can set InstallSuffix to do so. For example, when
	// using the race detector, the go command uses InstallSuffix = "race", so
	// that on a Linux/386 system, packages are written to a directory named
	// "linux_386_race" instead of the usual "linux_386".
	InstallSuffix string

	// By default, Import uses the operating system's file system calls
	// to read directories and files.  To read from other sources,
	// callers can set the following functions.  They all have default
	// behaviors that use the local file system, so clients need only set
	// the functions whose behaviors they wish to change.

	// JoinPath joins the sequence of path fragments into a single path.
	// If JoinPath is nil, Import uses filepath.Join.
	JoinPath func(elem ...string) string

	// SplitPathList splits the path list into a slice of individual paths.
	// If SplitPathList is nil, Import uses filepath.SplitList.
	SplitPathList func(list string) []string

	// IsAbsPath reports whether path is an absolute path.
	// If IsAbsPath is nil, Import uses filepath.IsAbs.
	IsAbsPath func(path string) bool

	// IsDir reports whether the path names a directory.
	// If IsDir is nil, Import calls os.Stat and uses the result's IsDir method.
	IsDir func(path string) bool

	// HasSubdir reports whether dir is a subdirectory of
	// (perhaps multiple levels below) root.
	// If so, HasSubdir sets rel to a slash-separated path that
	// can be joined to root to produce a path equivalent to dir.
	// If HasSubdir is nil, Import uses an implementation built on
	// filepath.EvalSymlinks.
	HasSubdir func(root, dir string) (rel string, ok bool)

	// ReadDir returns a slice of os.FileInfo, sorted by Name,
	// describing the content of the named directory.
	// If ReadDir is nil, Import uses ioutil.ReadDir.
	ReadDir func(dir string) (fi []os.FileInfo, err error)

	// OpenFile opens a file (not a directory) for reading.
	// If OpenFile is nil, Import uses os.Open.
	OpenFile func(path string) (r io.ReadCloser, err error)
}

// joinPath calls ctxt.JoinPath (if not nil) or else filepath.Join.
func (ctxt *Context) joinPath(elem ...string) string {
	if f := ctxt.JoinPath; f != nil {
		return f(elem...)
	}
	return filepath.Join(elem...)
}

// splitPathList calls ctxt.SplitPathList (if not nil) or else filepath.SplitList.
func (ctxt *Context) splitPathList(s string) []string {
	if f := ctxt.SplitPathList; f != nil {
		return f(s)
	}
	return filepath.SplitList(s)
}

// isAbsPath calls ctxt.IsAbsSPath (if not nil) or else filepath.IsAbs.
func (ctxt *Context) isAbsPath(path string) bool {
	if f := ctxt.IsAbsPath; f != nil {
		return f(path)
	}
	return filepath.IsAbs(path)
}

// isDir calls ctxt.IsDir (if not nil) or else uses os.Stat.
func (ctxt *Context) isDir(path string) bool {
	if f := ctxt.IsDir; f != nil {
		return f(path)
	}
	fi, err := os.Stat(path)
	return err == nil && fi.IsDir()
}

// hasSubdir calls ctxt.HasSubdir (if not nil) or else uses
// the local file system to answer the question.
func (ctxt *Context) hasSubdir(root, dir string) (rel string, ok bool) {
	if f := ctxt.HasSubdir; f != nil {
		return f(root, dir)
	}

	// Try using paths we received.
	if rel, ok = hasSubdir(root, dir); ok {
		return
	}

	// Try expanding symlinks and comparing
	// expanded against unexpanded and
	// expanded against expanded.
	rootSym, _ := filepath.EvalSymlinks(root)
	dirSym, _ := filepath.EvalSymlinks(dir)

	if rel, ok = hasSubdir(rootSym, dir); ok {
		return
	}
	if rel, ok = hasSubdir(root, dirSym); ok {
		return
	}
	return hasSubdir(rootSym, dirSym)
}

func hasSubdir(root, dir string) (rel string, ok bool) {
	const sep = string(filepath.Separator)
	root = filepath.Clean(root)
	if !strings.HasSuffix(root, sep) {
		root += sep
	}
	dir = filepath.Clean(dir)
	if !strings.HasPrefix(dir, root) {
		return "", false
	}
	return filepath.ToSlash(dir[len(root):]), true
}

// readDir calls ctxt.ReadDir (if not nil) or else ioutil.ReadDir.
func (ctxt *Context) readDir(path string) ([]os.FileInfo, error) {
	if f := ctxt.ReadDir; f != nil {
		return f(path)
	}
	return ioutil.ReadDir(path)
}

// openFile calls ctxt.OpenFile (if not nil) or else os.Open.
func (ctxt *Context) openFile(path string) (io.ReadCloser, error) {
	if fn := ctxt.OpenFile; fn != nil {
		return fn(path)
	}

	f, err := os.Open(path)
	if err != nil {
		return nil, err // nil interface
	}
	return f, nil
}

// isFile determines whether path is a file by trying to open it.
// It reuses openFile instead of adding another function to the
// list in Context.
func (ctxt *Context) isFile(path string) bool {
	f, err := ctxt.openFile(path)
	if err != nil {
		return false
	}
	f.Close()
	return true
}

// gopath returns the list of Go path directories.
func (ctxt *Context) gopath() []string {
	var all []string
	for _, p := range ctxt.splitPathList(ctxt.GOPATH) {
		if p == "" || p == ctxt.GOROOT {
			// Empty paths are uninteresting.
			// If the path is the GOROOT, ignore it.
			// People sometimes set GOPATH=$GOROOT, which is useless
			// but would cause us to find packages with import paths
			// like "pkg/math".
			// Do not get confused by this common mistake.
			continue
		}
		if strings.HasPrefix(p, "~") {
			// Path segments starting with ~ on Unix are almost always
			// users who have incorrectly quoted ~ while setting GOPATH,
			// preventing it from expanding to $HOME.
			// The situation is made more confusing by the fact that
			// bash allows quoted ~ in $PATH (most shells do not).
			// Do not get confused by this, and do not try to use the path.
			// It does not exist, and printing errors about it confuses
			// those users even more, because they think "sure ~ exists!".
			// The go command diagnoses this situation and prints a
			// useful error.
			// On Windows, ~ is used in short names, such as c:\progra~1
			// for c:\program files.
			continue
		}
		all = append(all, p)
	}
	return all
}

// SrcDirs returns a list of package source root directories.
// It draws from the current Go root and Go path but omits directories
// that do not exist.
func (ctxt *Context) SrcDirs() []string {
	var all []string
	if ctxt.GOROOT != "" {
		dir := ctxt.joinPath(ctxt.GOROOT, "src", "pkg")
		if ctxt.isDir(dir) {
			all = append(all, dir)
		}
	}
	for _, p := range ctxt.gopath() {
		dir := ctxt.joinPath(p, "src")
		if ctxt.isDir(dir) {
			all = append(all, dir)
		}
	}
	return all
}

// Default is the default Context for builds.
// It uses the GOARCH, GOOS, GOROOT, and GOPATH environment variables
// if set, or else the compiled code's GOARCH, GOOS, and GOROOT.
var Default Context = defaultContext()

var cgoEnabled = map[string]bool{
	"linux/386":       true,
	"linux/amd64":     true,
}

func defaultContext() Context {
	var c Context

	c.GOARCH = envOr("GOARCH", runtime.GOARCH)
	c.GOOS = envOr("GOOS", runtime.GOOS)
	c.GOROOT = runtime.GOROOT()
	c.GOPATH = envOr("GOPATH", "")
	c.Compiler = runtime.Compiler

	// Each major Go release in the Go 1.x series should add a tag here.
	// Old tags should not be removed. That is, the go1.x tag is present
	// in all releases >= Go 1.x. Code that requires Go 1.x or later should
	// say "+build go1.x", and code that should only be built before Go 1.x
	// (perhaps it is the stub to use in that case) should say "+build !go1.x".
	//
	// When we reach Go 1.3 the line will read
	//	c.ReleaseTags = []string{"go1.1", "go1.2", "go1.3"}
	// and so on.
	c.ReleaseTags = []string{"go1.1", "go1.2"}

	switch os.Getenv("CGO_ENABLED") {
	case "1":
		c.CgoEnabled = true
	case "0":
		c.CgoEnabled = false
	default:
		// golang.org/issue/5141
		// cgo should be disabled for cross compilation builds
		if runtime.GOARCH == c.GOARCH && runtime.GOOS == c.GOOS {
			c.CgoEnabled = cgoEnabled[c.GOOS+"/"+c.GOARCH]
			break
		}
		c.CgoEnabled = false
	}

	return c
}

func envOr(name, def string) string {
	s := os.Getenv(name)
	if s == "" {
		return def
	}
	return s
}

// An ImportMode controls the behavior of the Import method.
type ImportMode uint

const (
	// If FindOnly is set, Import stops after locating the directory
	// that should contain the sources for a package.  It does not
	// read any files in the directory.
	FindOnly ImportMode = 1 << iota

	// If AllowBinary is set, Import can be satisfied by a compiled
	// package object without corresponding sources.
	AllowBinary
)

// A Package describes the Go package found in a directory.
type Package struct {
	Dir         string   // directory containing package sources
	Name        string   // package name
	Doc         string   // documentation synopsis
	ImportPath  string   // import path of package ("" if unknown)
	Root        string   // root of Go tree where this package lives
	SrcRoot     string   // package source root directory ("" if unknown)
	PkgRoot     string   // package install root directory ("" if unknown)
	BinDir      string   // command install directory ("" if unknown)
	Goroot      bool     // package found in Go root
	PkgObj      string   // installed .a file
	AllTags     []string // tags that can influence file selection in this directory
	ConflictDir string   // this directory shadows Dir in $GOPATH

	// Source files
	GoFiles        []string // .go source files (excluding CgoFiles, TestGoFiles, XTestGoFiles)
	CgoFiles       []string // .go source files that import "C"
	IgnoredGoFiles []string // .go source files ignored for this build
	CFiles         []string // .c source files
	CXXFiles       []string // .cc, .cpp and .cxx source files
	HFiles         []string // .h, .hh, .hpp and .hxx source files
	SFiles         []string // .s source files
	SwigFiles      []string // .swig files
	SwigCXXFiles   []string // .swigcxx files
	SysoFiles      []string // .syso system object files to add to archive

	// Cgo directives
	CgoCFLAGS    []string // Cgo CFLAGS directives
	CgoCPPFLAGS  []string // Cgo CPPFLAGS directives
	CgoCXXFLAGS  []string // Cgo CXXFLAGS directives
	CgoLDFLAGS   []string // Cgo LDFLAGS directives
	CgoPkgConfig []string // Cgo pkg-config directives

	// Dependency information
	Imports   []string                    // imports from GoFiles, CgoFiles
	ImportPos map[string][]token.Position // line information for Imports

	// Test information
	TestGoFiles    []string                    // _test.go files in package
	TestImports    []string                    // imports from TestGoFiles
	TestImportPos  map[string][]token.Position // line information for TestImports
	XTestGoFiles   []string                    // _test.go files outside package
	XTestImports   []string                    // imports from XTestGoFiles
	XTestImportPos map[string][]token.Position // line information for XTestImports
}

// IsCommand reports whether the package is considered a
// command to be installed (not just a library).
// Packages named "main" are treated as commands.
func (p *Package) IsCommand() bool {
	return p.Name == "main"
}

// ImportDir is like Import but processes the Go package found in
// the named directory.
func (ctxt *Context) ImportDir(dir string, mode ImportMode) (*Package, error) {
	return ctxt.Import(".", dir, mode)
}

// NoGoError is the error used by Import to describe a directory
// containing no buildable Go source files. (It may still contain
// test files, files hidden by build tags, and so on.)
type NoGoError struct {
	Dir string
}

func (e *NoGoError) Error() string {
	return "no buildable Go source files in " + e.Dir
}

func nameExt(name string) string {
	i := strings.LastIndex(name, ".")
	if i < 0 {
		return ""
	}
	return name[i:]
}

// Import returns details about the Go package named by the import path,
// interpreting local import paths relative to the srcDir directory.
// If the path is a local import path naming a package that can be imported
// using a standard import path, the returned package will set p.ImportPath
// to that path.
//
// In the directory containing the package, .go, .c, .h, and .s files are
// considered part of the package except for:
//
//	- .go files in package documentation
//	- files starting with _ or . (likely editor temporary files)
//	- files with build constraints not satisfied by the context
//
// If an error occurs, Import returns a non-nil error and a non-nil
// *Package containing partial information.
//
func (ctxt *Context) Import(path string, srcDir string, mode ImportMode) (*Package, error) {
	p := &Package{
		ImportPath: path,
	}
	if path == "" {
		return p, fmt.Errorf("import %q: invalid import path", path)
	}

	var pkga string
	var pkgerr error
	switch ctxt.Compiler {
	case "gccgo":
		dir, elem := pathpkg.Split(p.ImportPath)
		pkga = "pkg/gccgo_" + ctxt.GOOS + "_" + ctxt.GOARCH + "/" + dir + "lib" + elem + ".a"
	case "gc":
		suffix := ""
		if ctxt.InstallSuffix != "" {
			suffix = "_" + ctxt.InstallSuffix
		}
		pkga = "pkg/" + ctxt.GOOS + "_" + ctxt.GOARCH + suffix + "/" + p.ImportPath + ".a"
	default:
		// Save error for end of function.
		pkgerr = fmt.Errorf("import %q: unknown compiler %q", path, ctxt.Compiler)
	}

	binaryOnly := false
	if IsLocalImport(path) {
		pkga = "" // local imports have no installed path
		if srcDir == "" {
			return p, fmt.Errorf("import %q: import relative to unknown directory", path)
		}
		if !ctxt.isAbsPath(path) {
			p.Dir = ctxt.joinPath(srcDir, path)
		}
		// Determine canonical import path, if any.
		if ctxt.GOROOT != "" {
			root := ctxt.joinPath(ctxt.GOROOT, "src", "pkg")
			if sub, ok := ctxt.hasSubdir(root, p.Dir); ok {
				p.Goroot = true
				p.ImportPath = sub
				p.Root = ctxt.GOROOT
				goto Found
			}
		}
		all := ctxt.gopath()
		for i, root := range all {
			rootsrc := ctxt.joinPath(root, "src")
			if sub, ok := ctxt.hasSubdir(rootsrc, p.Dir); ok {
				// We found a potential import path for dir,
				// but check that using it wouldn't find something
				// else first.
				if ctxt.GOROOT != "" {
					if dir := ctxt.joinPath(ctxt.GOROOT, "src", "pkg", sub); ctxt.isDir(dir) {
						p.ConflictDir = dir
						goto Found
					}
				}
				for _, earlyRoot := range all[:i] {
					if dir := ctxt.joinPath(earlyRoot, "src", sub); ctxt.isDir(dir) {
						p.ConflictDir = dir
						goto Found
					}
				}

				// sub would not name some other directory instead of this one.
				// Record it.
				p.ImportPath = sub
				p.Root = root
				goto Found
			}
		}
		// It's okay that we didn't find a root containing dir.
		// Keep going with the information we have.
	} else {
		if strings.HasPrefix(path, "/") {
			return p, fmt.Errorf("import %q: cannot import absolute path", path)
		}

		// tried records the location of unsuccessful package lookups
		var tried struct {
			goroot string
			gopath []string
		}

		// Determine directory from import path.
		if ctxt.GOROOT != "" {
			dir := ctxt.joinPath(ctxt.GOROOT, "src", "pkg", path)
			isDir := ctxt.isDir(dir)
			binaryOnly = !isDir && mode&AllowBinary != 0 && pkga != "" && ctxt.isFile(ctxt.joinPath(ctxt.GOROOT, pkga))
			if isDir || binaryOnly {
				p.Dir = dir
				p.Goroot = true
				p.Root = ctxt.GOROOT
				goto Found
			}
			tried.goroot = dir
		}
		for _, root := range ctxt.gopath() {
			dir := ctxt.joinPath(root, "src", path)
			isDir := ctxt.isDir(dir)
			binaryOnly = !isDir && mode&AllowBinary != 0 && pkga != "" && ctxt.isFile(ctxt.joinPath(root, pkga))
			if isDir || binaryOnly {
				p.Dir = dir
				p.Root = root
				goto Found
			}
			tried.gopath = append(tried.gopath, dir)
		}

		// package was not found
		var paths []string
		if tried.goroot != "" {
			paths = append(paths, fmt.Sprintf("\t%s (from $GOROOT)", tried.goroot))
		} else {
			paths = append(paths, "\t($GOROOT not set)")
		}
		var i int
		var format = "\t%s (from $GOPATH)"
		for ; i < len(tried.gopath); i++ {
			if i > 0 {
				format = "\t%s"
			}
			paths = append(paths, fmt.Sprintf(format, tried.gopath[i]))
		}
		if i == 0 {
			paths = append(paths, "\t($GOPATH not set)")
		}
		return p, fmt.Errorf("cannot find package %q in any of:\n%s", path, strings.Join(paths, "\n"))
	}

Found:
	if p.Root != "" {
		if p.Goroot {
			p.SrcRoot = ctxt.joinPath(p.Root, "src", "pkg")
		} else {
			p.SrcRoot = ctxt.joinPath(p.Root, "src")
		}
		p.PkgRoot = ctxt.joinPath(p.Root, "pkg")
		p.BinDir = ctxt.joinPath(p.Root, "bin")
		if pkga != "" {
			p.PkgObj = ctxt.joinPath(p.Root, pkga)
		}
	}

	if mode&FindOnly != 0 {
		return p, pkgerr
	}
	if binaryOnly && (mode&AllowBinary) != 0 {
		return p, pkgerr
	}

	dirs, err := ctxt.readDir(p.Dir)
	if err != nil {
		return p, err
	}

	var Sfiles []string // files with ".S" (capital S)
	var firstFile string
	imported := make(map[string][]token.Position)
	testImported := make(map[string][]token.Position)
	xTestImported := make(map[string][]token.Position)
	allTags := make(map[string]bool)
	fset := token.NewFileSet()
	for _, d := range dirs {
		if d.IsDir() {
			continue
		}

		name := d.Name()
		ext := nameExt(name)

		match, data, filename, err := ctxt.matchFile(p.Dir, name, true, allTags)
		if err != nil {
			return p, err
		}
		if !match {
			if ext == ".go" {
				p.IgnoredGoFiles = append(p.IgnoredGoFiles, name)
			}
			continue
		}

		// Going to save the file.  For non-Go files, can stop here.
		switch ext {
		case ".c":
			p.CFiles = append(p.CFiles, name)
			continue
		case ".cc", ".cpp", ".cxx":
			p.CXXFiles = append(p.CXXFiles, name)
			continue
		case ".h", ".hh", ".hpp", ".hxx":
			p.HFiles = append(p.HFiles, name)
			continue
		case ".s":
			p.SFiles = append(p.SFiles, name)
			continue
		case ".S":
			Sfiles = append(Sfiles, name)
			continue
		case ".swig":
			p.SwigFiles = append(p.SwigFiles, name)
			continue
		case ".swigcxx":
			p.SwigCXXFiles = append(p.SwigCXXFiles, name)
			continue
		case ".syso":
			// binary objects to add to package archive
			// Likely of the form foo_windows.syso, but
			// the name was vetted above with goodOSArchFile.
			p.SysoFiles = append(p.SysoFiles, name)
			continue
		}

		pf, err := parser.ParseFile(fset, filename, data, parser.ImportsOnly|parser.ParseComments)
		if err != nil {
			return p, err
		}

		pkg := pf.Name.Name
		if pkg == "documentation" {
			p.IgnoredGoFiles = append(p.IgnoredGoFiles, name)
			continue
		}

		isTest := strings.HasSuffix(name, "_test.go")
		isXTest := false
		if isTest && strings.HasSuffix(pkg, "_test") {
			isXTest = true
			pkg = pkg[:len(pkg)-len("_test")]
		}

		if p.Name == "" {
			p.Name = pkg
			firstFile = name
		} else if pkg != p.Name {
			return p, fmt.Errorf("found packages %s (%s) and %s (%s) in %s", p.Name, firstFile, pkg, name, p.Dir)
		}
		if pf.Doc != nil && p.Doc == "" {
			p.Doc = doc.Synopsis(pf.Doc.Text())
		}

		// Record imports and information about cgo.
		isCgo := false
		for _, decl := range pf.Decls {
			d, ok := decl.(*ast.GenDecl)
			if !ok {
				continue
			}
			for _, dspec := range d.Specs {
				spec, ok := dspec.(*ast.ImportSpec)
				if !ok {
					continue
				}
				quoted := spec.Path.Value
				path, err := strconv.Unquote(quoted)
				if err != nil {
					log.Panicf("%s: parser returned invalid quoted string: <%s>", filename, quoted)
				}
				if isXTest {
					xTestImported[path] = append(xTestImported[path], fset.Position(spec.Pos()))
				} else if isTest {
					testImported[path] = append(testImported[path], fset.Position(spec.Pos()))
				} else {
					imported[path] = append(imported[path], fset.Position(spec.Pos()))
				}
				if path == "C" {
					if isTest {
						return p, fmt.Errorf("use of cgo in test %s not supported", filename)
					}
					cg := spec.Doc
					if cg == nil && len(d.Specs) == 1 {
						cg = d.Doc
					}
					if cg != nil {
						if err := ctxt.saveCgo(filename, p, cg); err != nil {
							return p, err
						}
					}
					isCgo = true
				}
			}
		}
		if isCgo {
			allTags["cgo"] = true
			if ctxt.CgoEnabled {
				p.CgoFiles = append(p.CgoFiles, name)
			} else {
				p.IgnoredGoFiles = append(p.IgnoredGoFiles, name)
			}
		} else if isXTest {
			p.XTestGoFiles = append(p.XTestGoFiles, name)
		} else if isTest {
			p.TestGoFiles = append(p.TestGoFiles, name)
		} else {
			p.GoFiles = append(p.GoFiles, name)
		}
	}
	if len(p.GoFiles)+len(p.CgoFiles)+len(p.TestGoFiles)+len(p.XTestGoFiles) == 0 {
		return p, &NoGoError{p.Dir}
	}

	for tag := range allTags {
		p.AllTags = append(p.AllTags, tag)
	}
	sort.Strings(p.AllTags)

	p.Imports, p.ImportPos = cleanImports(imported)
	p.TestImports, p.TestImportPos = cleanImports(testImported)
	p.XTestImports, p.XTestImportPos = cleanImports(xTestImported)

	// add the .S files only if we are using cgo
	// (which means gcc will compile them).
	// The standard assemblers expect .s files.
	if len(p.CgoFiles) > 0 {
		p.SFiles = append(p.SFiles, Sfiles...)
		sort.Strings(p.SFiles)
	}

	return p, pkgerr
}

// MatchFile reports whether the file with the given name in the given directory
// matches the context and would be included in a Package created by ImportDir
// of that directory.
//
// MatchFile considers the name of the file and may use ctxt.OpenFile to
// read some or all of the file's content.
func (ctxt *Context) MatchFile(dir, name string) (match bool, err error) {
	match, _, _, err = ctxt.matchFile(dir, name, false, nil)
	return
}

// matchFile determines whether the file with the given name in the given directory
// should be included in the package being constructed.
// It returns the data read from the file.
// If returnImports is true and name denotes a Go program, matchFile reads
// until the end of the imports (and returns that data) even though it only
// considers text until the first non-comment.
// If allTags is non-nil, matchFile records any encountered build tag
// by setting allTags[tag] = true.
func (ctxt *Context) matchFile(dir, name string, returnImports bool, allTags map[string]bool) (match bool, data []byte, filename string, err error) {
	if strings.HasPrefix(name, "_") ||
		strings.HasPrefix(name, ".") {
		return
	}

	i := strings.LastIndex(name, ".")
	if i < 0 {
		i = len(name)
	}
	ext := name[i:]

	if !ctxt.goodOSArchFile(name, allTags) && !ctxt.UseAllFiles {
		return
	}

	switch ext {
	case ".go", ".c", ".cc", ".cxx", ".cpp", ".s", ".h", ".hh", ".hpp", ".hxx", ".S", ".swig", ".swigcxx":
		// tentatively okay - read to make sure
	case ".syso":
		// binary, no reading
		match = true
		return
	default:
		// skip
		return
	}

	filename = ctxt.joinPath(dir, name)
	f, err := ctxt.openFile(filename)
	if err != nil {
		return
	}

	if strings.HasSuffix(filename, ".go") {
		data, err = readImports(f, false)
	} else {
		data, err = readComments(f)
	}
	f.Close()
	if err != nil {
		err = fmt.Errorf("read %s: %v", filename, err)
		return
	}

	// Look for +build comments to accept or reject the file.
	if !ctxt.shouldBuild(data, allTags) && !ctxt.UseAllFiles {
		return
	}

	match = true
	return
}

func cleanImports(m map[string][]token.Position) ([]string, map[string][]token.Position) {
	all := make([]string, 0, len(m))
	for path := range m {
		all = append(all, path)
	}
	sort.Strings(all)
	return all, m
}

// Import is shorthand for Default.Import.
func Import(path, srcDir string, mode ImportMode) (*Package, error) {
	return Default.Import(path, srcDir, mode)
}

// ImportDir is shorthand for Default.ImportDir.
func ImportDir(dir string, mode ImportMode) (*Package, error) {
	return Default.ImportDir(dir, mode)
}

var slashslash = []byte("//")

// caller:
// 	1. Context.matchFile() 只有这一处
//
// shouldBuild reports whether it is okay to use this file,
// The rule is that in the file's leading run of // comments
// and blank lines, which must be followed by a blank line
// (to avoid including a Go package clause doc comment),
// lines beginning with '// +build' are taken as build directives.
//
// The file is accepted only if each such line lists something
// matching the file.  For example:
//
//	// +build windows linux
//
// marks the file as applicable only on Windows and Linux.
//
func (ctxt *Context) shouldBuild(content []byte, allTags map[string]bool) bool {
	// Pass 1. Identify leading run of // comments and blank lines,
	// which must be followed by a blank line.
	end := 0
	p := content
	for len(p) > 0 {
		line := p
		if i := bytes.IndexByte(line, '\n'); i >= 0 {
			line, p = line[:i], p[i+1:]
		} else {
			p = p[len(p):]
		}
		line = bytes.TrimSpace(line)
		if len(line) == 0 { // Blank line
			end = len(content) - len(p)
			continue
		}
		if !bytes.HasPrefix(line, slashslash) { // Not comment line
			break
		}
	}
	content = content[:end]

	// Pass 2.  Process each line in the run.
	p = content
	allok := true
	for len(p) > 0 {
		line := p
		if i := bytes.IndexByte(line, '\n'); i >= 0 {
			line, p = line[:i], p[i+1:]
		} else {
			p = p[len(p):]
		}
		line = bytes.TrimSpace(line)
		if bytes.HasPrefix(line, slashslash) {
			line = bytes.TrimSpace(line[len(slashslash):])
			if len(line) > 0 && line[0] == '+' {
				// Looks like a comment +line.
				f := strings.Fields(string(line))
				if f[0] == "+build" {
					ok := false
					for _, tok := range f[1:] {
						if ctxt.match(tok, allTags) {
							ok = true
						}
					}
					if !ok {
						allok = false
					}
				}
			}
		}
	}

	return allok
}

// saveCgo saves the information from the #cgo lines in the import "C" comment.
// These lines set CFLAGS, CPPFLAGS, CXXFLAGS and LDFLAGS and pkg-config directives
// that affect the way cgo's C code is built.
//
// TODO(rsc): This duplicates code in cgo.
// Once the dust settles, remove this code from cgo.
func (ctxt *Context) saveCgo(filename string, di *Package, cg *ast.CommentGroup) error {
	text := cg.Text()
	for _, line := range strings.Split(text, "\n") {
		orig := line

		// Line is
		//	#cgo [GOOS/GOARCH...] LDFLAGS: stuff
		//
		line = strings.TrimSpace(line)
		if len(line) < 5 || line[:4] != "#cgo" || (line[4] != ' ' && line[4] != '\t') {
			continue
		}

		// Split at colon.
		line = strings.TrimSpace(line[4:])
		i := strings.Index(line, ":")
		if i < 0 {
			return fmt.Errorf("%s: invalid #cgo line: %s", filename, orig)
		}
		line, argstr := line[:i], line[i+1:]

		// Parse GOOS/GOARCH stuff.
		f := strings.Fields(line)
		if len(f) < 1 {
			return fmt.Errorf("%s: invalid #cgo line: %s", filename, orig)
		}

		cond, verb := f[:len(f)-1], f[len(f)-1]
		if len(cond) > 0 {
			ok := false
			for _, c := range cond {
				if ctxt.match(c, nil) {
					ok = true
					break
				}
			}
			if !ok {
				continue
			}
		}

		args, err := splitQuoted(argstr)
		if err != nil {
			return fmt.Errorf("%s: invalid #cgo line: %s", filename, orig)
		}
		for _, arg := range args {
			if !safeCgoName(arg) {
				return fmt.Errorf("%s: malformed #cgo argument: %s", filename, arg)
			}
		}

		switch verb {
		case "CFLAGS":
			di.CgoCFLAGS = append(di.CgoCFLAGS, args...)
		case "CPPFLAGS":
			di.CgoCPPFLAGS = append(di.CgoCPPFLAGS, args...)
		case "CXXFLAGS":
			di.CgoCXXFLAGS = append(di.CgoCXXFLAGS, args...)
		case "LDFLAGS":
			di.CgoLDFLAGS = append(di.CgoLDFLAGS, args...)
		case "pkg-config":
			di.CgoPkgConfig = append(di.CgoPkgConfig, args...)
		default:
			return fmt.Errorf("%s: invalid #cgo verb: %s", filename, orig)
		}
	}
	return nil
}

// NOTE: $ is not safe for the shell, but it is allowed here because of linker options like -Wl,$ORIGIN.
// We never pass these arguments to a shell (just to programs we construct argv for), so this should be okay.
// See golang.org/issue/6038.
var safeBytes = []byte("+-.,/0123456789=ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz:$")

func safeCgoName(s string) bool {
	if s == "" {
		return false
	}
	for i := 0; i < len(s); i++ {
		if c := s[i]; c < 0x80 && bytes.IndexByte(safeBytes, c) < 0 {
			return false
		}
	}
	return true
}

// splitQuoted splits the string s around each instance of one or more consecutive
// white space characters while taking into account quotes and escaping, and
// returns an array of substrings of s or an empty list if s contains only white space.
// Single quotes and double quotes are recognized to prevent splitting within the
// quoted region, and are removed from the resulting substrings. If a quote in s
// isn't closed err will be set and r will have the unclosed argument as the
// last element.  The backslash is used for escaping.
//
// For example, the following string:
//
//     a b:"c d" 'e''f'  "g\""
//
// Would be parsed as:
//
//     []string{"a", "b:c d", "ef", `g"`}
//
func splitQuoted(s string) (r []string, err error) {
	var args []string
	arg := make([]rune, len(s))
	escaped := false
	quoted := false
	quote := '\x00'
	i := 0
	for _, rune := range s {
		switch {
		case escaped:
			escaped = false
		case rune == '\\':
			escaped = true
			continue
		case quote != '\x00':
			if rune == quote {
				quote = '\x00'
				continue
			}
		case rune == '"' || rune == '\'':
			quoted = true
			quote = rune
			continue
		case unicode.IsSpace(rune):
			if quoted || i > 0 {
				quoted = false
				args = append(args, string(arg[:i]))
				i = 0
			}
			continue
		}
		arg[i] = rune
		i++
	}
	if quoted || i > 0 {
		args = append(args, string(arg[:i]))
	}
	if quote != 0 {
		err = errors.New("unclosed quote")
	} else if escaped {
		err = errors.New("unfinished escaping")
	}
	return args, err
}

// match returns true if the name is one of:
//
//	$GOOS
//	$GOARCH
//	cgo (if cgo is enabled)
//	!cgo (if cgo is disabled)
//	ctxt.Compiler
//	!ctxt.Compiler
//	tag (if tag is listed in ctxt.BuildTags or ctxt.ReleaseTags)
//	!tag (if tag is not listed in ctxt.BuildTags or ctxt.ReleaseTags)
//	a comma-separated list of any of these
//
func (ctxt *Context) match(name string, allTags map[string]bool) bool {
	if name == "" {
		if allTags != nil {
			allTags[name] = true
		}
		return false
	}
	if i := strings.Index(name, ","); i >= 0 {
		// comma-separated list
		ok1 := ctxt.match(name[:i], allTags)
		ok2 := ctxt.match(name[i+1:], allTags)
		return ok1 && ok2
	}
	if strings.HasPrefix(name, "!!") { // bad syntax, reject always
		return false
	}
	if strings.HasPrefix(name, "!") { // negation
		return len(name) > 1 && !ctxt.match(name[1:], allTags)
	}

	if allTags != nil {
		allTags[name] = true
	}

	// Tags must be letters, digits, underscores or dots.
	// Unlike in Go identifiers, all digits are fine (e.g., "386").
	for _, c := range name {
		if !unicode.IsLetter(c) && !unicode.IsDigit(c) && c != '_' && c != '.' {
			return false
		}
	}

	// special tags
	if ctxt.CgoEnabled && name == "cgo" {
		return true
	}
	if name == ctxt.GOOS || name == ctxt.GOARCH || name == ctxt.Compiler {
		return true
	}

	// other tags
	for _, tag := range ctxt.BuildTags {
		if tag == name {
			return true
		}
	}
	for _, tag := range ctxt.ReleaseTags {
		if tag == name {
			return true
		}
	}

	return false
}

// 判断目标文件是否需要编译, 如果目标文件中包含已知的 os/arch 名称(如 sys_plan9_arm64.go),
// 但与当前所在主机不符, 则不编译.
//
// 	@name: 待编译的文件名(无路径, 有后缀)
//
// goodOSArchFile returns false if the name contains a $GOOS or $GOARCH
// suffix which does not match the current system.
// The recognized name formats are:
//
//     name_$(GOOS).*
//     name_$(GOARCH).*
//     name_$(GOOS)_$(GOARCH).*
//     name_$(GOOS)_test.*
//     name_$(GOARCH)_test.*
//     name_$(GOOS)_$(GOARCH)_test.*
//
func (ctxt *Context) goodOSArchFile(name string, allTags map[string]bool) bool {
	// 移除后缀名
	if dot := strings.Index(name, "."); dot != -1 {
		name = name[:dot]
	}

	////////////////////////////////////////////////////////////////////////////
	// 	@compatible: src/pkg/go/build/syslist.go -> [goosList, goarchList] 中,
	// 扩大了 os/arch 范围, 但源文件中有可能出现 linux.go, amd64.go 这种,
	// 包含 os/arch 名称, 但又明显不是用来做多系统兼容的, 
	// 我们要的是 linux_arm64.go 这种, 必须是用下划线分割 os/arch 的才算.
	//
	// Before Go 1.4, a file called "linux.go" would be equivalent to having a
	// build tag "linux" in that file. For Go 1.4 and beyond, we require this
	// auto-tagging to apply only to files with a non-empty prefix, so
	// "foo_linux.go" is tagged but "linux.go" is not. This allows new operating
	// systems, such as android, to arrive without breaking existing code with
	// innocuous source code in "android.go". The easiest fix: cut everything
	// in the name before the initial _.
	i := strings.Index(name, "_")
	if i < 0 {
		return true
	}
	name = name[i:] // ignore everything before first _
	////////////////////////////////////////////////////////////////////////////

	// _test.go 文件也有可能包含 os/arch 信息
	l := strings.Split(name, "_")
	if n := len(l); n > 0 && l[n-1] == "test" {
		l = l[:n-1]
	}
	n := len(l)
	if n >= 2 && knownOS[l[n-2]] && knownArch[l[n-1]] {
		if allTags != nil {
			allTags[l[n-2]] = true
			allTags[l[n-1]] = true
		}
		return l[n-2] == ctxt.GOOS && l[n-1] == ctxt.GOARCH
	}
	if n >= 1 && knownOS[l[n-1]] {
		if allTags != nil {
			allTags[l[n-1]] = true
		}
		return l[n-1] == ctxt.GOOS
	}
	if n >= 1 && knownArch[l[n-1]] {
		if allTags != nil {
			allTags[l[n-1]] = true
		}
		return l[n-1] == ctxt.GOARCH
	}
	return true
}

var knownOS = make(map[string]bool)
var knownArch = make(map[string]bool)

func init() {
	for _, v := range strings.Fields(goosList) {
		knownOS[v] = true
	}
	for _, v := range strings.Fields(goarchList) {
		knownArch[v] = true
	}
}

// ToolDir is the directory containing build tools.
var ToolDir = filepath.Join(runtime.GOROOT(), "pkg/tool/"+runtime.GOOS+"_"+runtime.GOARCH)

// IsLocalImport reports whether the import path is
// a local import path, like ".", "..", "./foo", or "../foo".
func IsLocalImport(path string) bool {
	return path == "." || path == ".." ||
		strings.HasPrefix(path, "./") || strings.HasPrefix(path, "../")
}

// ArchChar returns the architecture character for the given goarch.
// For example, ArchChar("amd64") returns "6".
func ArchChar(goarch string) (string, error) {
	switch goarch {
	case "386":
		return "8", nil
	case "amd64":
		return "6", nil
	}
	return "", errors.New("unsupported GOARCH " + goarch)
}
