package main

import (
	"strings"
	"regexp"
	"errors"
	"strconv"
	"sync"
)

var cgoRe = regexp.MustCompile(`[/\\:]`)

var (
	cgoLibGccFile     string
	cgoLibGccErr      error
	cgoLibGccFileOnce sync.Once
)

func (b *builder) cgo(p *Package, cgoExe, obj string, gccfiles []string, gxxfiles []string) (outGo, outObj []string, err error) {
	if goos != toolGOOS {
		return nil, nil, errors.New("cannot use cgo when compiling for a different operating system")
	}

	cgoCPPFLAGS := stringList(envList("CGO_CPPFLAGS"), p.CgoCPPFLAGS)
	cgoCFLAGS := stringList(envList("CGO_CFLAGS"), p.CgoCFLAGS)
	cgoCXXFLAGS := stringList(envList("CGO_CXXFLAGS"), p.CgoCXXFLAGS)
	cgoLDFLAGS := stringList(envList("CGO_LDFLAGS"), p.CgoLDFLAGS)

	if pkgs := p.CgoPkgConfig; len(pkgs) > 0 {
		out, err := b.runOut(p.Dir, p.ImportPath, nil, "pkg-config", "--cflags", pkgs)
		if err != nil {
			b.showOutput(p.Dir, "pkg-config --cflags "+strings.Join(pkgs, " "), string(out))
			b.print(err.Error() + "\n")
			return nil, nil, errPrintedOutput
		}
		if len(out) > 0 {
			cgoCPPFLAGS = append(cgoCPPFLAGS, strings.Fields(string(out))...)
		}
		out, err = b.runOut(p.Dir, p.ImportPath, nil, "pkg-config", "--libs", pkgs)
		if err != nil {
			b.showOutput(p.Dir, "pkg-config --libs "+strings.Join(pkgs, " "), string(out))
			b.print(err.Error() + "\n")
			return nil, nil, errPrintedOutput
		}
		if len(out) > 0 {
			cgoLDFLAGS = append(cgoLDFLAGS, strings.Fields(string(out))...)
		}
	}

	// Allows including _cgo_export.h from .[ch] files in the package.
	cgoCPPFLAGS = append(cgoCPPFLAGS, "-I", obj)

	// cgo
	// TODO: CGOPKGPATH, CGO_FLAGS?
	gofiles := []string{obj + "_cgo_gotypes.go"}
	cfiles := []string{"_cgo_main.c", "_cgo_export.c"}
	for _, fn := range p.CgoFiles {
		f := cgoRe.ReplaceAllString(fn[:len(fn)-2], "_")
		gofiles = append(gofiles, obj+f+"cgo1.go")
		cfiles = append(cfiles, f+"cgo2.c")
	}
	defunC := obj + "_cgo_defun.c"

	cgoflags := []string{}
	// TODO: make cgo not depend on $GOARCH?

	objExt := archChar

	if p.Standard && p.ImportPath == "runtime/cgo" {
		cgoflags = append(cgoflags, "-import_runtime_cgo=false")
	}
	if p.Standard && (p.ImportPath == "runtime/race" || p.ImportPath == "runtime/cgo") {
		cgoflags = append(cgoflags, "-import_syscall=false")
	}

	// Update $CGO_LDFLAGS with p.CgoLDFLAGS.
	var cgoenv []string
	if len(cgoLDFLAGS) > 0 {
		flags := make([]string, len(cgoLDFLAGS))
		for i, f := range cgoLDFLAGS {
			flags[i] = strconv.Quote(f)
		}
		cgoenv = []string{"CGO_LDFLAGS=" + strings.Join(flags, " ")}
	}

	if _, ok := buildToolchain.(gccgoToolchain); ok {
		cgoflags = append(cgoflags, "-gccgo")
		if pkgpath := gccgoPkgpath(p); pkgpath != "" {
			cgoflags = append(cgoflags, "-gccgopkgpath="+pkgpath)
		}
		objExt = "o"
	}
	if err := b.run(p.Dir, p.ImportPath, cgoenv, cgoExe, "-objdir", obj, cgoflags, "--", cgoCPPFLAGS, cgoCFLAGS, p.CgoFiles); err != nil {
		return nil, nil, err
	}
	outGo = append(outGo, gofiles...)

	// cc _cgo_defun.c
	defunObj := obj + "_cgo_defun." + objExt
	if err := buildToolchain.cc(b, p, obj, defunObj, defunC); err != nil {
		return nil, nil, err
	}
	outObj = append(outObj, defunObj)

	// gcc
	var linkobj []string

	var bareLDFLAGS []string
	// filter out -lsomelib, -l somelib, *.{so,dll,dylib}, and (on Darwin) -framework X
	for i := 0; i < len(cgoLDFLAGS); i++ {
		f := cgoLDFLAGS[i]
		switch {
		// skip "-lc" or "-l somelib"
		case strings.HasPrefix(f, "-l"):
			if f == "-l" {
				i++
			}
		// skip "*.{dylib,so,dll}"
		case strings.HasSuffix(f, ".dylib"),
			strings.HasSuffix(f, ".so"),
			strings.HasSuffix(f, ".dll"):
			continue
		default:
			bareLDFLAGS = append(bareLDFLAGS, f)
		}
	}

	cgoLibGccFileOnce.Do(func() {
		cgoLibGccFile, cgoLibGccErr = b.libgcc(p)
	})
	if cgoLibGccFile == "" && cgoLibGccErr != nil {
		return nil, nil, err
	}

	var staticLibs []string
	if cgoLibGccFile != "" {
		staticLibs = append(staticLibs, cgoLibGccFile)
	}

	cflags := stringList(cgoCPPFLAGS, cgoCFLAGS)
	for _, cfile := range cfiles {
		ofile := obj + cfile[:len(cfile)-1] + "o"
		if err := b.gcc(p, ofile, cflags, obj+cfile); err != nil {
			return nil, nil, err
		}
		linkobj = append(linkobj, ofile)
		if !strings.HasSuffix(ofile, "_cgo_main.o") {
			outObj = append(outObj, ofile)
		}
	}

	for _, file := range gccfiles {
		ofile := obj + cgoRe.ReplaceAllString(file[:len(file)-1], "_") + "o"
		if err := b.gcc(p, ofile, cflags, file); err != nil {
			return nil, nil, err
		}
		linkobj = append(linkobj, ofile)
		outObj = append(outObj, ofile)
	}

	cxxflags := stringList(cgoCPPFLAGS, cgoCXXFLAGS)
	for _, file := range gxxfiles {
		// Append .o to the file, just in case the pkg has file.c and file.cpp
		ofile := obj + cgoRe.ReplaceAllString(file, "_") + ".o"
		if err := b.gxx(p, ofile, cxxflags, file); err != nil {
			return nil, nil, err
		}
		linkobj = append(linkobj, ofile)
		outObj = append(outObj, ofile)
	}

	linkobj = append(linkobj, p.SysoFiles...)
	dynobj := obj + "_cgo_.o"
	if goarch == "arm" && goos == "linux" { // we need to use -pie for Linux/ARM to get accurate imported sym
		cgoLDFLAGS = append(cgoLDFLAGS, "-pie")
	}
	if err := b.gccld(p, dynobj, cgoLDFLAGS, linkobj); err != nil {
		return nil, nil, err
	}
	if goarch == "arm" && goos == "linux" { // but we don't need -pie for normal cgo programs
		cgoLDFLAGS = cgoLDFLAGS[0 : len(cgoLDFLAGS)-1]
	}

	if _, ok := buildToolchain.(gccgoToolchain); ok {
		// we don't use dynimport when using gccgo.
		return outGo, outObj, nil
	}

	// cgo -dynimport
	importC := obj + "_cgo_import.c"
	cgoflags = []string{}
	if p.Standard && p.ImportPath == "runtime/cgo" {
		cgoflags = append(cgoflags, "-dynlinker") // record path to dynamic linker
	}
	if err := b.run(p.Dir, p.ImportPath, nil, cgoExe, "-objdir", obj, "-dynimport", dynobj, "-dynout", importC, cgoflags); err != nil {
		return nil, nil, err
	}

	// cc _cgo_import.ARCH
	importObj := obj + "_cgo_import." + objExt
	if err := buildToolchain.cc(b, p, obj, importObj, importC); err != nil {
		return nil, nil, err
	}

	ofile := obj + "_all.o"
	var gccObjs, nonGccObjs []string
	for _, f := range outObj {
		if strings.HasSuffix(f, ".o") {
			gccObjs = append(gccObjs, f)
		} else {
			nonGccObjs = append(nonGccObjs, f)
		}
	}
	if err := b.gccld(p, ofile, stringList(bareLDFLAGS, "-Wl,-r", "-nostdlib", staticLibs), gccObjs); err != nil {
		return nil, nil, err
	}

	// NOTE(rsc): The importObj is a 5c/6c/8c object and on Windows
	// must be processed before the gcc-generated objects.
	// Put it first.  http://golang.org/issue/2601
	outObj = stringList(importObj, nonGccObjs, ofile)

	return outGo, outObj, nil
}
