package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

// gcToolchain 实现了 toolchain 接口.
//
// The Go toolchain.
type gcToolchain struct{}

func (gcToolchain) compiler() string {
	return tool(archChar + "g")
}

func (gcToolchain) linker() string {
	return tool(archChar + "l")
}

// gc 调用 6g 工具编译指定 package.
//
// 	@param p: 待编译的某个 package, 可以是标准库, 第三方库, 或者 main 库.
// 	@param obj: 如 /tmp/go-build044877922/sort/_obj/(参数 p 所表示的库)
// 	@param importArgs: 如 ["-I", "/tmp/go-build044877922"]
// 	@param gofiles: p 库下的所有 .go 文件(不包含 _test.go 文件)
//
// 	@return ofile: 6g 生成的中间文件路径, 如/tmp/go-build642732376/sort/_obj/_go_.6.
//
// caller:
// 	1. builder.build()
func (gcToolchain) gc(
	b *builder, p *Package, obj string, importArgs []string, gofiles []string,
) (ofile string, output []byte, err error) {
	out := "_go_." + archChar
	ofile = obj + out
	gcargs := []string{"-p", p.ImportPath}
	if p.Standard && p.ImportPath == "runtime" {
		// runtime compiles with a special 6g flag to emit
		// additional reflect type data.
		gcargs = append(gcargs, "-+")
	}

	// If we're giving the compiler the entire package (no C etc files), tell it that,
	// so that it can give good error messages about forward declarations.
	// Exceptions: a few standard packages have forward declarations for
	// pieces supplied behind-the-scenes by package runtime.
	extFiles := len(p.CgoFiles) + len(p.CFiles) + len(p.CXXFiles) + len(p.SFiles) +
		len(p.SysoFiles) + len(p.SwigFiles) + len(p.SwigCXXFiles)
	if p.Standard {
		switch p.ImportPath {
		case "os", "runtime/pprof", "sync", "time":
			extFiles++
		}
	}
	if extFiles == 0 {
		gcargs = append(gcargs, "-complete")
	}
	if buildContext.InstallSuffix != "" {
		gcargs = append(gcargs, "-installsuffix", buildContext.InstallSuffix)
	}
	if buildNoStrict {
		gcargs = append(gcargs, "-nostrict")
	}
	// tool 获取名为 toolName 的编译工具所在的路径并返回, 这里并未执行.
	// 注意: 6g 只是编译, 而不是链接, 这里的 ofile 只生成编译的中间文件.
	args := stringList(
		tool(archChar+"g"), "-o", ofile, buildGcflags, gcargs, "-D", p.localPrefix, importArgs,
	)
	for _, f := range gofiles {
		args = append(args, mkAbs(p.Dir, f))
	}
	// 执行上述命令, 调用编译工具.
	/*
	args 列表如:
		"/usr/local/go/pkg/tool/linux_amd64/6g"
		"-o"
		"/tmp/go-build562581380/sort/_obj/_go_.6"
		"-p"
		"sort"
		"-complete"
		"-D"
		"_/usr/local/go/src/pkg/sort"
		"-I"
		"/tmp/go-build562581380"
		"/usr/local/go/src/pkg/sort/search.go"
		"/usr/local/go/src/pkg/sort/sort.go"
		"/usr/local/go/src/pkg/sort/sort_compatible.go"
		"/usr/local/go/src/pkg/sort/sort_string.go"
		"/usr/local/go/src/pkg/sort/zfuncversion_compatible.go"
	*/
	output, err = b.runOut(p.Dir, p.ImportPath, nil, args)
	return ofile, output, err
}

// 	@param p: 待编译的某个 package, 可以是标准库, 第三方库, 或者 main 库.
// 	@param objDir: /tmp/go-build562581380/sort/_obj/
// 	@param afile: /tmp/go-build562581380/sort.a
// 	@param ofiles: [/tmp/go-build562581380/sort/_obj/_go_.6]
//
// caller:
// 	1. src/cmd/go/build.go -> builder.build()
func (gcToolchain) pack(b *builder, p *Package, objDir, afile string, ofiles []string) error {
	var absOfiles []string
	for _, f := range ofiles {
		absOfiles = append(absOfiles, mkAbs(objDir, f))
	}
	return b.run(
		p.Dir, p.ImportPath, nil,
		tool("pack"), "grcP", b.work, mkAbs(objDir, afile), absOfiles,
	)
}

// 	@param out: 一般为 go build -o 指定的二进制可执行文件
func (gcToolchain) ld(
	b *builder, p *Package, out string, allactions []*action, mainpkg string, ofiles []string,
) error {
	importArgs := b.includeArgs("-L", allactions)
	swigDirs := make(map[string]bool)
	swigArg := []string{}
	cxx := false
	for _, a := range allactions {
		if a.p != nil && a.p.usesSwig() {
			sd := a.p.swigDir(&buildContext)
			if len(swigArg) == 0 {
				swigArg = []string{"-r", sd}
			} else if !swigDirs[sd] {
				swigArg[1] += ":"
				swigArg[1] += sd
			}
			swigDirs[sd] = true
			if a.objdir != "" && !swigDirs[a.objdir] {
				swigArg[1] += ":"
				swigArg[1] += a.objdir
				swigDirs[a.objdir] = true
			}
		}
		if a.p != nil && len(a.p.CXXFiles) > 0 {
			cxx = true
		}
	}
	ldflags := buildLdflags
	if buildContext.InstallSuffix != "" {
		ldflags = append(ldflags, "-installsuffix", buildContext.InstallSuffix)
	}
	if cxx {
		// The program includes C++ code.  If the user has not
		// specified the -extld option, then default to
		// linking with the compiler named by the CXX
		// environment variable, or g++ if CXX is not set.
		extld := false
		for _, f := range ldflags {
			if f == "-extld" || strings.HasPrefix(f, "-extld=") {
				extld = true
				break
			}
		}
		if !extld {
			compiler := strings.Fields(os.Getenv("CXX"))
			if len(compiler) == 0 {
				compiler = []string{"g++"}
			}
			ldflags = append(ldflags, "-extld="+compiler[0])
			if len(compiler) > 1 {
				extldflags := false
				add := strings.Join(compiler[1:], " ")
				for i, f := range ldflags {
					if f == "-extldflags" && i+1 < len(ldflags) {
						ldflags[i+1] = add + " " + ldflags[i+1]
						extldflags = true
						break
					} else if strings.HasPrefix(f, "-extldflags=") {
						ldflags[i] = "-extldflags=" + add + " " + ldflags[i][len("-extldflags="):]
						extldflags = true
						break
					}
				}
				if !extldflags {
					ldflags = append(ldflags, "-extldflags="+add)
				}
			}
		}
	}
	return b.run(
		".", p.ImportPath, nil,
		tool(archChar+"l"), "-o", out, importArgs, swigArg, ldflags, mainpkg,
	)
}

func (gcToolchain) cc(b *builder, p *Package, objdir, ofile, cfile string) error {
	inc := filepath.Join(goroot, "pkg", fmt.Sprintf("%s_%s", goos, goarch))
	cfile = mkAbs(p.Dir, cfile)
	args := stringList(
		tool(archChar+"c"), "-F", "-V", "-w", "-I", objdir, "-I", inc, "-o", ofile,
		buildCcflags, "-D", "GOOS_"+goos, "-D", "GOARCH_"+goarch, cfile,
	)
	return b.run(p.Dir, p.ImportPath, nil, args)
}

func (gcToolchain) asm(b *builder, p *Package, obj, ofile, sfile string) error {
	sfile = mkAbs(p.Dir, sfile)
	return b.run(
		p.Dir, p.ImportPath, nil,
		tool(archChar+"a"), "-I", obj, "-o", ofile, "-D", "GOOS_"+goos, "-D", "GOARCH_"+goarch, sfile,
	)
}

func (gcToolchain) pkgpath(basedir string, p *Package) string {
	end := filepath.FromSlash(p.ImportPath + ".a")
	return filepath.Join(basedir, end)
}
