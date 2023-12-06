// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package build

import (
	"errors"
	"fmt"
	"go/ast"
	"go/doc"
	"go/parser"
	"go/token"
	"io"
	"log"
	"os"
	pathpkg "path"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
)

// A Context specifies the supporting context for a build.
type Context struct {
	GOARCH string // target architecture
	GOOS   string // target operating system
	GOROOT string // Go root
	GOPATH string // Go path
	// 由 CGO_ENABLED 环境变量控制, 在 defaultContext() 中赋值.
	// whether cgo can be used
	CgoEnabled  bool
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

// gopath 遍历 GOPATH 环境变量, 返回所有合法路径.
//
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

// SrcDirs 获取所有源码路径并返回, 主要包括2项: GOPATH/src, GOROOT/src/pkg(标准库)
//
// caller:
// 	1. src/cmd/go/main.go -> matchPackages() 只有这一处
//
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

// 被赋值为 src/cmd/go/build_init.go -> buildContext 对象
//
// Default is the default Context for builds.
// It uses the GOARCH, GOOS, GOROOT, and GOPATH environment variables
// if set, or else the compiled code's GOARCH, GOOS, and GOROOT.
var Default Context = defaultContext()

var cgoEnabled = map[string]bool{
	"linux/386":   true,
	"linux/amd64": true,
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

// ImportDir is like Import but processes the Go package found in
// the named directory.
func (ctxt *Context) ImportDir(dir string, mode ImportMode) (*Package, error) {
	return ctxt.Import(".", dir, mode)
}

// NoGoError is the error used by Import to describe a directory
// containing no buildable Go source files.
// (It may still contain test files, files hidden by build tags, and so on.)
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

// Import 在标准库以及 GOPATH 目录下寻找 path 所表示的 packge, 如不存在则报错.
// 然后为该 package 下所有文件进行语法解析, 并记录ta们的 import(),
// 以便获取该 package 下依赖的子 package 信息.
//
// 	@param path: import 的目录路径, 如"fmt", "math", 也可能是相对路径, 如"."
// 	@param srcDir: path 参数对应的绝对路径?
//
// caller:
// 	1. src/cmd/go/pkg.go -> loadImport()
// 	2. Context.ImportDir()
// 	3. Import() 通过 Default Context 对象调用, 则实际上没有地方调用, 可忽略这一处.
//
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

	// pkga 指的是某个 package, 如果不存在目录, 那么 .a 文件也是可以的, 比如 fmt.a,
	// 见 pkg/linux_amd64/fmt.a
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

	// 在标准库目录, 以及其余 GOPATH 目录等寻找 path 所表示的 package.
	// 如果没找到则报错, 找到了则跳转到 Found 块.
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

		// 1. 先在标准库中找
		if ctxt.GOROOT != "" {
			dir := ctxt.joinPath(ctxt.GOROOT, "src", "pkg", path)
			// 1.1 如果存在目标目录(path 是 import 语句中的某个包名)
			isDir := ctxt.isDir(dir)
			// 1.2 或者 pkg/linux_amd64/ 下存在 .a 文件
			binaryOnly = !isDir && mode&AllowBinary != 0 && pkga != "" && ctxt.isFile(ctxt.joinPath(ctxt.GOROOT, pkga))
			if isDir || binaryOnly {
				p.Dir = dir
				p.Goroot = true
				p.Root = ctxt.GOROOT
				goto Found
			}
			tried.goroot = dir
		}
		// 2. 然后在各个 gopath 目录下找
		for _, root := range ctxt.gopath() {
			dir := ctxt.joinPath(root, "src", path)
			// 1.1 如果存在目标目录(path 是 import 语句中的某个包名)
			isDir := ctxt.isDir(dir)
			// 1.2 或者 pkg/linux_amd64/ 下存在 .a 文件
			binaryOnly = !isDir && mode&AllowBinary != 0 && pkga != "" && ctxt.isFile(ctxt.joinPath(root, pkga))
			if isDir || binaryOnly {
				p.Dir = dir
				p.Root = root
				goto Found
			}
			tried.gopath = append(tried.gopath, dir)
		}

		// 如果没找到
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
	// 遍历 package 所在目录下的所有文件(忽略子目录)
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

		// 解析当前文件, 得到语法树对象
		pf, err := parser.ParseFile(
			fset, filename, data, parser.ImportsOnly|parser.ParseComments,
		)
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
			// 如果一个 package 目录下存在2个(甚至多个)不同的 package 名称, 则报错.
			return p, fmt.Errorf(
				"found packages %s (%s) and %s (%s) in %s",
				p.Name, firstFile, pkg, name, p.Dir,
			)
		}
		if pf.Doc != nil && p.Doc == "" {
			p.Doc = doc.Synopsis(pf.Doc.Text())
		}

		// Record imports and information about cgo.
		isCgo := false
		// Decls 是当前 .go 文件中的全局声明内容, 一般为 import(), var(), const(), type A = B 块等
		for _, decl := range pf.Decls {
			d, ok := decl.(*ast.GenDecl)
			if !ok {
				continue
			}
			for _, dspec := range d.Specs {
				// 只处理 import() 部分
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

func cleanImports(m map[string][]token.Position) ([]string, map[string][]token.Position) {
	all := make([]string, 0, len(m))
	for path := range m {
		all = append(all, path)
	}
	sort.Strings(all)
	return all, m
}

// caller: 无处调用
//
// Import is shorthand for Default.Import.
func Import(path, srcDir string, mode ImportMode) (*Package, error) {
	return Default.Import(path, srcDir, mode)
}

// caller:
// 	1. src/cmd/go/main.go -> matchPackagesInFS() 只有这一处
//
// ImportDir is shorthand for Default.ImportDir.
func ImportDir(dir string, mode ImportMode) (*Package, error) {
	return Default.ImportDir(dir, mode)
}

// ToolDir is the directory containing build tools.
var ToolDir = filepath.Join(runtime.GOROOT(), "pkg/tool/"+runtime.GOOS+"_"+runtime.GOARCH)

// IsLocalImport 判断 import 的目录路径是否为本地路径(相对路径).
//
// 	@param path: import 的目录路径, 如"fmt", "math", 也可能是相对路径, 如"."
//
// IsLocalImport reports whether the import path is
// a local import path, like ".", "..", "./foo", or "../foo".
func IsLocalImport(path string) bool {
	return path == "." || path == ".." ||
		strings.HasPrefix(path, "./") || strings.HasPrefix(path, "../")
}

// caller:
// 	1. src/cmd/go/build_init.go -> init() 只有这一处
//
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
