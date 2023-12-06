package build

import "go/token"

// A Package describes the Go package found in a directory.
type Package struct {
	Dir         string   // directory containing package sources
	Name        string   // package name
	Doc         string   // documentation synopsis
	ImportPath  string   // import path of package ("" if unknown)
	// 可以是 GOROOT 本身(此时 Goroot = true), 也可以是 GOPATH 中的某个路径
	//
	Root        string   // root of Go tree where this package lives
	// 如果 Goroot = true, SrcRoot = GOROOT/src/pkg, 否则为 GOPATH/src
	//
	// package source root directory ("" if unknown)
	SrcRoot     string
	PkgRoot     string   // package install root directory ("" if unknown)
	BinDir      string   // command install directory ("" if unknown)
	// Goroot 如果当前 package 是 GOROOT 下的包(一般是标准库)
	// package found in Go root
	Goroot      bool
	PkgObj      string   // installed .a file
	AllTags     []string // tags that can influence file selection in this directory
	ConflictDir string   // this directory shadows Dir in $GOPATH

	// Source files
	GoFiles        []string // .go source files (excluding CgoFiles, TestGoFiles, XTestGoFiles)
	CgoFiles       []string // .go source files that import "C"
	// 当前 package 所在目录下的 .go 文件, 不参与构建的(只是文件名, 不包含路径).
	// 比如文件名以下划线/点号开头, 或是包含不兼容的"CPU架构/OS系统",
	// 或是文件内容中的'// +build'语句与 -tag 不符合.
	//
	// .go source files ignored for this build
	IgnoredGoFiles []string
	CFiles         []string // .c source files
	CXXFiles       []string // .cc, .cpp and .cxx source files
	HFiles         []string // .h, .hh, .hpp and .hxx source files
	// 当前 package 目录下的 .s 汇编源码文件
	// .s source files
	SFiles         []string
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
