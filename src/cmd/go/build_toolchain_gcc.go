package main

import (
	"fmt"
	"strings"
	"os/exec"
	"path/filepath"
)

type gccgoToolchain struct{}

var gccgoBin, _ = exec.LookPath("gccgo")

func (gccgoToolchain) compiler() string {
	return gccgoBin
}

func (gccgoToolchain) linker() string {
	return gccgoBin
}

func (gccgoToolchain) gc(b *builder, p *Package, obj string, importArgs []string, gofiles []string) (ofile string, output []byte, err error) {
	out := p.Name + ".o"
	ofile = obj + out
	gcargs := []string{"-g"}
	gcargs = append(gcargs, b.gccArchArgs()...)
	if pkgpath := gccgoPkgpath(p); pkgpath != "" {
		gcargs = append(gcargs, "-fgo-pkgpath="+pkgpath)
	}
	if p.localPrefix != "" {
		gcargs = append(gcargs, "-fgo-relative-import-path="+p.localPrefix)
	}
	args := stringList("gccgo", importArgs, "-c", gcargs, "-o", ofile, buildGccgoflags)
	for _, f := range gofiles {
		args = append(args, mkAbs(p.Dir, f))
	}

	output, err = b.runOut(p.Dir, p.ImportPath, nil, args)
	return ofile, output, err
}

func (gccgoToolchain) asm(b *builder, p *Package, obj, ofile, sfile string) error {
	sfile = mkAbs(p.Dir, sfile)
	defs := []string{"-D", "GOOS_" + goos, "-D", "GOARCH_" + goarch}
	if pkgpath := gccgoCleanPkgpath(p); pkgpath != "" {
		defs = append(defs, `-D`, `GOPKGPATH="`+pkgpath+`"`)
	}
	defs = append(defs, b.gccArchArgs()...)
	return b.run(p.Dir, p.ImportPath, nil, "gccgo", "-I", obj, "-o", ofile, defs, sfile)
}

func (gccgoToolchain) pkgpath(basedir string, p *Package) string {
	end := filepath.FromSlash(p.ImportPath + ".a")
	afile := filepath.Join(basedir, end)
	// add "lib" to the final element
	return filepath.Join(filepath.Dir(afile), "lib"+filepath.Base(afile))
}

func (gccgoToolchain) pack(b *builder, p *Package, objDir, afile string, ofiles []string) error {
	var absOfiles []string
	for _, f := range ofiles {
		absOfiles = append(absOfiles, mkAbs(objDir, f))
	}
	return b.run(p.Dir, p.ImportPath, nil, "ar", "cru", mkAbs(objDir, afile), absOfiles)
}

func (tools gccgoToolchain) ld(b *builder, p *Package, out string, allactions []*action, mainpkg string, ofiles []string) error {
	// gccgo needs explicit linking with all package dependencies,
	// and all LDFLAGS from cgo dependencies.
	afiles := make(map[*Package]string)
	sfiles := make(map[*Package][]string)
	ldflags := b.gccArchArgs()
	cgoldflags := []string{}
	usesCgo := false
	cxx := false
	for _, a := range allactions {
		if a.p != nil {
			if !a.p.Standard {
				if afiles[a.p] == "" || a.objpkg != a.target {
					afiles[a.p] = a.target
				}
			}
			cgoldflags = append(cgoldflags, a.p.CgoLDFLAGS...)
			if len(a.p.CgoFiles) > 0 {
				usesCgo = true
			}
			if a.p.usesSwig() {
				sd := a.p.swigDir(&buildContext)
				if a.objdir != "" {
					sd = a.objdir
				}
				for _, f := range stringList(a.p.SwigFiles, a.p.SwigCXXFiles) {
					soname := a.p.swigSoname(f)
					sfiles[a.p] = append(sfiles[a.p], filepath.Join(sd, soname))
				}
				usesCgo = true
			}
			if len(a.p.CXXFiles) > 0 {
				cxx = true
			}
		}
	}
	for _, afile := range afiles {
		ldflags = append(ldflags, afile)
	}
	for _, sfiles := range sfiles {
		ldflags = append(ldflags, sfiles...)
	}
	ldflags = append(ldflags, cgoldflags...)
	if usesCgo && goos == "linux" {
		ldflags = append(ldflags, "-Wl,-E")
	}
	if cxx {
		ldflags = append(ldflags, "-lstdc++")
	}
	return b.run(".", p.ImportPath, nil, "gccgo", "-o", out, ofiles, "-Wl,-(", ldflags, "-Wl,-)", buildGccgoflags)
}

func (gccgoToolchain) cc(b *builder, p *Package, objdir, ofile, cfile string) error {
	inc := filepath.Join(goroot, "pkg", fmt.Sprintf("%s_%s", goos, goarch))
	cfile = mkAbs(p.Dir, cfile)
	defs := []string{"-D", "GOOS_" + goos, "-D", "GOARCH_" + goarch}
	defs = append(defs, b.gccArchArgs()...)
	if pkgpath := gccgoCleanPkgpath(p); pkgpath != "" {
		defs = append(defs, `-D`, `GOPKGPATH="`+pkgpath+`"`)
	}
	// TODO: Support using clang here (during gccgo build)?
	return b.run(p.Dir, p.ImportPath, nil, "gcc", "-Wall", "-g",
		"-I", objdir, "-I", inc, "-o", ofile, defs, "-c", cfile)
}

func gccgoPkgpath(p *Package) string {
	if p.build.IsCommand() && !p.forceLibrary {
		return ""
	}
	return p.ImportPath
}

func gccgoCleanPkgpath(p *Package) string {
	clean := func(r rune) rune {
		switch {
		case 'A' <= r && r <= 'Z', 'a' <= r && r <= 'z',
			'0' <= r && r <= '9':
			return r
		}
		return '_'
	}
	return strings.Map(clean, gccgoPkgpath(p))
}
