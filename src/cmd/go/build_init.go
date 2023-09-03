package main

import (
	"go/build"
	"os"
	"path/filepath"
	"runtime"
)

func init() {
	// break init cycle
	cmdBuild.Run = runBuild
	cmdInstall.Run = runInstall

	// 	@compatible: addAt v1.3
	cmdBuild.Flag.BoolVar(&buildI, "i", false, "")

	addBuildFlags(cmdBuild)
	addBuildFlags(cmdInstall)
}

// Flags set by multiple commands.
var buildA bool // -a flag
var buildN bool // -n flag
// par 并行构建的线程数
var buildP = runtime.NumCPU() // -p flag
var buildV bool               // -v flag
var buildX bool               // -x flag
// 	@compatible: addAt v1.3
var buildI bool // -i flag
var buildO = cmdBuild.Flag.String("o", "", "output file")
var buildWork bool           // -work flag
var buildGcflags []string    // -gcflags flag
var buildCcflags []string    // -ccflags flag
var buildLdflags []string    // -ldflags flag
var buildGccgoflags []string // -gccgoflags flag
var buildRace bool           // -race flag
var buildNoStrict bool       // -nostrict flag

var buildContext = build.Default
var buildToolchain toolchain = noToolchain{}

func init() {
	switch build.Default.Compiler {
	case "gc":
		buildToolchain = gcToolchain{}
	case "gccgo":
		buildToolchain = gccgoToolchain{}
	}
}

// addBuildFlags adds the flags common to the build and install commands.
func addBuildFlags(cmd *Command) {
	// NOTE: If you add flags here, also add them to testflag.go.
	cmd.Flag.BoolVar(&buildA, "a", false, "")
	cmd.Flag.BoolVar(&buildN, "n", false, "")
	cmd.Flag.IntVar(&buildP, "p", buildP, "")
	cmd.Flag.StringVar(&buildContext.InstallSuffix, "installsuffix", "", "")
	cmd.Flag.BoolVar(&buildV, "v", false, "")
	cmd.Flag.BoolVar(&buildX, "x", false, "")
	cmd.Flag.BoolVar(&buildWork, "work", false, "")
	cmd.Flag.Var((*stringsFlag)(&buildGcflags), "gcflags", "")
	cmd.Flag.Var((*stringsFlag)(&buildCcflags), "ccflags", "")
	cmd.Flag.Var((*stringsFlag)(&buildLdflags), "ldflags", "")
	cmd.Flag.Var((*stringsFlag)(&buildGccgoflags), "gccgoflags", "")
	cmd.Flag.Var((*stringsFlag)(&buildContext.BuildTags), "tags", "")
	cmd.Flag.Var(buildCompiler{}, "compiler", "")
	cmd.Flag.BoolVar(&buildRace, "race", false, "")
	cmd.Flag.BoolVar(&buildNoStrict, "nostrict", false, "disable strict mode for unused variable and package")
}

// Global build parameters (used during package load)
var (
	goarch    string
	goos      string
	archChar  string
	exeSuffix string
)

func init() {
	goarch = buildContext.GOARCH
	goos = buildContext.GOOS

	var err error
	archChar, err = build.ArchChar(goarch)
	if err != nil {
		fatalf("%s", err)
	}
}

// buildMode 构建的两种模式, 一般 go run/build 的模式是 modeBuild, ta们的入口必须是 main 包,
// 而 go install 的模式是 modeInstall, 不需要 main 包.
//
// buildMode specifies the build mode:
// are we just building things or also installing the results?
type buildMode int

const (
	modeBuild buildMode = iota
	modeInstall
)

var (
	goroot       = filepath.Clean(runtime.GOROOT())
	gobin        = os.Getenv("GOBIN")
	gorootBin    = filepath.Join(goroot, "bin")
	gorootSrcPkg = filepath.Join(goroot, "src/pkg")
	gorootPkg    = filepath.Join(goroot, "pkg")
	gorootSrc    = filepath.Join(goroot, "src")
)
