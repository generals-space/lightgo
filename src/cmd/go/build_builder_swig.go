package main

import (
	"bytes"
	"errors"
	"io/ioutil"
	"path/filepath"
)

// Run SWIG on all SWIG input files.
// TODO: Don't build a shared library, once SWIG emits the necessary
// pragmas for external linking.
func (b *builder) swig(p *Package, obj string, gccfiles, gxxfiles []string) (outGo, outObj []string, err error) {

	var extraObj []string
	for _, file := range gccfiles {
		ofile := obj + cgoRe.ReplaceAllString(file[:len(file)-1], "_") + "o"
		if err := b.gcc(p, ofile, nil, file); err != nil {
			return nil, nil, err
		}
		extraObj = append(extraObj, ofile)
	}

	for _, file := range gxxfiles {
		// Append .o to the file, just in case the pkg has file.c and file.cpp
		ofile := obj + cgoRe.ReplaceAllString(file, "_") + ".o"
		if err := b.gxx(p, ofile, nil, file); err != nil {
			return nil, nil, err
		}
		extraObj = append(extraObj, ofile)
	}

	intgosize, err := b.swigIntSize(obj)
	if err != nil {
		return nil, nil, err
	}

	for _, f := range p.SwigFiles {
		goFile, objFile, err := b.swigOne(p, f, obj, false, intgosize, extraObj)
		if err != nil {
			return nil, nil, err
		}
		if goFile != "" {
			outGo = append(outGo, goFile)
		}
		if objFile != "" {
			outObj = append(outObj, objFile)
		}
	}
	for _, f := range p.SwigCXXFiles {
		goFile, objFile, err := b.swigOne(p, f, obj, true, intgosize, extraObj)
		if err != nil {
			return nil, nil, err
		}
		if goFile != "" {
			outGo = append(outGo, goFile)
		}
		if objFile != "" {
			outObj = append(outObj, objFile)
		}
	}
	return outGo, outObj, nil
}

// This code fails to build if sizeof(int) <= 32
const swigIntSizeCode = `
package main
const i int = 1 << 32
`

// Determine the size of int on the target system for the -intgosize option
// of swig >= 2.0.9
func (b *builder) swigIntSize(obj string) (intsize string, err error) {
	if buildN {
		return "$INTBITS", nil
	}
	src := filepath.Join(b.work, "swig_intsize.go")
	if err = ioutil.WriteFile(src, []byte(swigIntSizeCode), 0644); err != nil {
		return
	}
	srcs := []string{src}

	p := goFilesPackage(srcs)

	if _, _, e := buildToolchain.gc(b, p, obj, nil, srcs); e != nil {
		return "32", nil
	}
	return "64", nil
}

// Run SWIG on one SWIG input file.
func (b *builder) swigOne(p *Package, file, obj string, cxx bool, intgosize string, extraObj []string) (outGo, outObj string, err error) {
	n := 5 // length of ".swig"
	if cxx {
		n = 8 // length of ".swigcxx"
	}
	base := file[:len(file)-n]
	goFile := base + ".go"
	cBase := base + "_gc."
	gccBase := base + "_wrap."
	gccExt := "c"
	if cxx {
		gccExt = "cxx"
	}
	soname := p.swigSoname(file)

	_, gccgo := buildToolchain.(gccgoToolchain)

	// swig
	args := []string{
		"-go",
		"-intgosize", intgosize,
		"-module", base,
		"-soname", soname,
		"-o", obj + gccBase + gccExt,
		"-outdir", obj,
	}
	if gccgo {
		args = append(args, "-gccgo")
	}
	if cxx {
		args = append(args, "-c++")
	}

	if out, err := b.runOut(p.Dir, p.ImportPath, nil, "swig", args, file); err != nil {
		if len(out) > 0 {
			if bytes.Contains(out, []byte("Unrecognized option -intgosize")) {
				return "", "", errors.New("must have SWIG version >= 2.0.9\n")
			}
			b.showOutput(p.Dir, p.ImportPath, b.processOutput(out))
			return "", "", errPrintedOutput
		}
		return "", "", err
	}

	var cObj string
	if !gccgo {
		// cc
		cObj = obj + cBase + archChar
		if err := buildToolchain.cc(b, p, obj, cObj, obj+cBase+"c"); err != nil {
			return "", "", err
		}
	}

	// gcc
	gccObj := obj + gccBase + "o"
	if err := b.gcc(p, gccObj, []string{"-g", "-fPIC", "-O2"}, obj+gccBase+gccExt); err != nil {
		return "", "", err
	}

	// create shared library
	osldflags := map[string][]string{
		"linux": {"-shared", "-lpthread", "-lm"},
	}
	var cxxlib []string
	if cxx {
		cxxlib = []string{"-lstdc++"}
	}
	ldflags := stringList(osldflags[goos], cxxlib)
	target := filepath.Join(obj, soname)
	b.run(p.Dir, p.ImportPath, nil, b.gccCmd(p.Dir), "-o", target, gccObj, extraObj, ldflags)

	return obj + goFile, cObj, nil
}
