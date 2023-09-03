package main

var cmdBuild = &Command{
	UsageLine: "build [-o output] [build flags] [packages]",
	Short:     "compile packages and dependencies",
	Long: `
Build compiles the packages named by the import paths,
along with their dependencies, but it does not install the results.

If the arguments are a list of .go files, build treats them as a list
of source files specifying a single package.

When the command line specifies a single main package,
build writes the resulting executable to output.
Otherwise build compiles the packages but discards the results,
serving only as a check that the packages can be built.

The -o flag specifies the output file name. If not specified, the
output file name depends on the arguments and derives from the name
of the package, such as p.a for package p, unless p is 'main'. If
the package is main and file names are provided, the file name
derives from the first file name mentioned, such as f1 for 'go build
f1.go f2.go'; with no files provided ('go build'), the output file
name is the base name of the containing directory.

The build flags are shared by the build, install, run, and test commands:

	-a
		force rebuilding of packages that are already up-to-date.
	-n
		print the commands but do not run them.
	-p n
		the number of builds that can be run in parallel.
		The default is the number of CPUs available.
	-race
		enable data race detection.
		Supported only on linux/amd64, darwin/amd64 and windows/amd64.
	-v
		print the names of packages as they are compiled.
	-work
		print the name of the temporary work directory and
		do not delete it when exiting.
	-x
		print the commands.

	-ccflags 'arg list'
		arguments to pass on each 5c, 6c, or 8c compiler invocation.
	-compiler name
		name of compiler to use, as in runtime.Compiler (gccgo or gc).
	-gccgoflags 'arg list'
		arguments to pass on each gccgo compiler/linker invocation.
	-gcflags 'arg list'
		arguments to pass on each 5g, 6g, or 8g compiler invocation.
	-installsuffix suffix
		a suffix to use in the name of the package installation directory,
		in order to keep output separate from default builds.
		If using the -race flag, the install suffix is automatically set to race
		or, if set explicitly, has _race appended to it.
	-ldflags 'flag list'
		arguments to pass on each 5l, 6l, or 8l linker invocation.
	-tags 'tag list'
		a list of build tags to consider satisfied during the build.
		See the documentation for the go/build package for
		more information about build tags.

The list flags accept a space-separated list of strings. To embed spaces
in an element in the list, surround it with either single or double quotes.

For more about specifying packages, see 'go help packages'.
For more about where packages and binaries are installed,
run 'go help gopath'.  For more about calling between Go and C/C++,
run 'go help c'.

See also: go install, go get, go clean.
	`,
}

var cmdInstall = &Command{
	UsageLine: "install [build flags] [packages]",
	Short:     "compile and install packages and dependencies",
	Long: `
Install compiles and installs the packages named by the import paths,
along with their dependencies.

For more about the build flags, see 'go help build'.
For more about specifying packages, see 'go help packages'.

See also: go build, go get, go clean.
	`,
}

var cmdRun = &Command{
	UsageLine: "run [build flags] gofiles... [arguments...]",
	Short:     "compile and run Go program",
	Long: `
Run compiles and runs the main package comprising the named Go source files.
A Go source file is defined to be a file ending in a literal ".go" suffix.

For more about build flags, see 'go help build'.

See also: go build.
	`,
}

var cmdClean = &Command{
	UsageLine: "clean [-i] [-r] [-n] [-x] [packages]",
	Short:     "remove object files",
	Long: `
Clean removes object files from package source directories.
The go command builds most objects in a temporary directory,
so go clean is mainly concerned with object files left by other
tools or by manual invocations of go build.

Specifically, clean removes the following files from each of the
source directories corresponding to the import paths:

	_obj/            old object directory, left from Makefiles
	_test/           old test directory, left from Makefiles
	_testmain.go     old gotest file, left from Makefiles
	test.out         old test log, left from Makefiles
	build.out        old test log, left from Makefiles
	*.[568ao]        object files, left from Makefiles

	DIR(.exe)        from go build
	DIR.test(.exe)   from go test -c
	MAINFILE(.exe)   from go build MAINFILE.go
	*.so             from SWIG

In the list, DIR represents the final path element of the
directory, and MAINFILE is the base name of any Go source
file in the directory that is not included when building
the package.

The -i flag causes clean to remove the corresponding installed
archive or binary (what 'go install' would create).

The -n flag causes clean to print the remove commands it would execute,
but not run them.

The -r flag causes clean to be applied recursively to all the
dependencies of the packages named by the import paths.

The -x flag causes clean to print remove commands as it executes them.

For more about specifying packages, see 'go help packages'.
	`,
}

var cmdList = &Command{
	UsageLine: "list [-e] [-race] [-f format] [-json] [-tags 'tag list'] [packages]",
	Short:     "list packages",
	Long: `
List lists the packages named by the import paths, one per line.

The default output shows the package import path:

    code.google.com/p/google-api-go-client/books/v1
    code.google.com/p/goauth2/oauth
    code.google.com/p/sqlite

The -f flag specifies an alternate format for the list, using the
syntax of package template.  The default output is equivalent to -f
'{{.ImportPath}}'.  One extra template function is available, "join",
which calls strings.Join. The struct being passed to the template is:

    type Package struct {
        Dir        string // directory containing package sources
        ImportPath string // import path of package in dir
        Name       string // package name
        Doc        string // package documentation string
        Target     string // install path
        Goroot     bool   // is this package in the Go root?
        Standard   bool   // is this package part of the standard Go library?
        Stale      bool   // would 'go install' do anything for this package?
        Root       string // Go root or Go path dir containing this package

        // Source files
        GoFiles  []string       // .go source files (excluding CgoFiles, TestGoFiles, XTestGoFiles)
        CgoFiles []string       // .go sources files that import "C"
        IgnoredGoFiles []string // .go sources ignored due to build constraints
        CFiles   []string       // .c source files
        CXXFiles []string       // .cc, .cxx and .cpp source files
        HFiles   []string       // .h, .hh, .hpp and .hxx source files
        SFiles   []string       // .s source files
        SwigFiles []string      // .swig files
        SwigCXXFiles []string   // .swigcxx files
        SysoFiles []string      // .syso object files to add to archive

        // Cgo directives
        CgoCFLAGS    []string // cgo: flags for C compiler
        CgoCPPFLAGS  []string // cgo: flags for C preprocessor
        CgoCXXFLAGS  []string // cgo: flags for C++ compiler
        CgoLDFLAGS   []string // cgo: flags for linker
        CgoPkgConfig []string // cgo: pkg-config names

        // Dependency information
        Imports []string // import paths used by this package
        Deps    []string // all (recursively) imported dependencies

        // Error information
        Incomplete bool            // this package or a dependency has an error
        Error      *PackageError   // error loading package
        DepsErrors []*PackageError // errors loading dependencies

        TestGoFiles  []string // _test.go files in package
        TestImports  []string // imports from TestGoFiles
        XTestGoFiles []string // _test.go files outside package
        XTestImports []string // imports from XTestGoFiles
    }

The -json flag causes the package data to be printed in JSON format
instead of using the template format.

The -e flag changes the handling of erroneous packages, those that
cannot be found or are malformed.  By default, the list command
prints an error to standard error for each erroneous package and
omits the packages from consideration during the usual printing.
With the -e flag, the list command never prints errors to standard
error and instead processes the erroneous packages with the usual
printing.  Erroneous packages will have a non-empty ImportPath and
a non-nil Error field; other information may or may not be missing
(zeroed).

The -tags flag specifies a list of build tags, like in the 'go build'
command.

The -race flag causes the package data to include the dependencies
required by the race detector.

For more about specifying packages, see 'go help packages'.
	`,
}
