package main

import (
	"bytes"
	"fmt"
	"strings"
	"path/filepath"
	"os"
)

// libgcc returns the filename for libgcc, as determined by invoking gcc with
// the -print-libgcc-file-name option.
func (b *builder) libgcc(p *Package) (string, error) {
	var buf bytes.Buffer

	gccCmd := b.gccCmd(p.Dir)

	prev := b.print
	if buildN {
		// In -n mode we temporarily swap out the builder's
		// print function to capture the command-line. This
		// let's us assign it to $LIBGCC and produce a valid
		// buildscript for cgo packages.
		b.print = func(a ...interface{}) (int, error) {
			return fmt.Fprint(&buf, a...)
		}
	}
	f, err := b.runOut(p.Dir, p.ImportPath, nil, gccCmd, "-print-libgcc-file-name")
	if err != nil {
		return "", fmt.Errorf("gcc -print-libgcc-file-name: %v (%s)", err, f)
	}
	if buildN {
		s := fmt.Sprintf("LIBGCC=$(%s)\n", buf.Next(buf.Len()-1))
		b.print = prev
		b.print(s)
		return "$LIBGCC", nil
	}

	// clang might not be able to find libgcc, and in that case,
	// it will simply return "libgcc.a", which is of no use to us.
	if strings.Contains(gccCmd[0], "clang") && !filepath.IsAbs(string(f)) {
		return "", nil
	}

	return strings.Trim(string(f), "\r\n"), nil
}

// gcc runs the gcc C compiler to create an object from a single C file.
func (b *builder) gcc(p *Package, out string, flags []string, cfile string) error {
	return b.ccompile(p, out, flags, cfile, b.gccCmd(p.Dir))
}

// gxx runs the g++ C++ compiler to create an object from a single C++ file.
func (b *builder) gxx(p *Package, out string, flags []string, cxxfile string) error {
	return b.ccompile(p, out, flags, cxxfile, b.gxxCmd(p.Dir))
}

// ccompile runs the given C or C++ compiler and creates an object from a single source file.
func (b *builder) ccompile(p *Package, out string, flags []string, file string, compiler []string) error {
	file = mkAbs(p.Dir, file)
	return b.run(p.Dir, p.ImportPath, nil, compiler, flags, "-o", out, "-c", file)
}

// gccld runs the gcc linker to create an executable from a set of object files.
func (b *builder) gccld(p *Package, out string, flags []string, obj []string) error {
	var cmd []string
	if len(p.CXXFiles) > 0 {
		cmd = b.gxxCmd(p.Dir)
	} else {
		cmd = b.gccCmd(p.Dir)
	}
	return b.run(p.Dir, p.ImportPath, nil, cmd, "-o", out, obj, flags)
}

// gccCmd returns a gcc command line prefix
// defaultCC is defined in zdefaultcc.go, written by cmd/dist.
func (b *builder) gccCmd(objdir string) []string {
	return b.ccompilerCmd("CC", defaultCC, objdir)
}

// gxxCmd returns a g++ command line prefix
// defaultCXX is defined in zdefaultcc.go, written by cmd/dist.
func (b *builder) gxxCmd(objdir string) []string {
	return b.ccompilerCmd("CXX", defaultCXX, objdir)
}

// ccompilerCmd returns a command line prefix for the given environment
// variable and using the default command when the variable is empty
func (b *builder) ccompilerCmd(envvar, defcmd, objdir string) []string {
	// NOTE: env.go's mkEnv knows that the first three
	// strings returned are "gcc", "-I", objdir (and cuts them off).

	compiler := strings.Fields(os.Getenv(envvar))
	if len(compiler) == 0 {
		compiler = strings.Fields(defcmd)
	}
	a := []string{compiler[0], "-I", objdir, "-g", "-O2"}
	a = append(a, compiler[1:]...)

	// Definitely want -fPIC but on Windows gcc complains
	// "-fPIC ignored for target (all code is position independent)"
	if goos != "windows" {
		a = append(a, "-fPIC")
	}
	a = append(a, b.gccArchArgs()...)
	// gcc-4.5 and beyond require explicit "-pthread" flag
	// for multithreading with pthread library.
	if buildContext.CgoEnabled {
		switch goos {
		default:
			a = append(a, "-pthread")
		}
	}

	if strings.Contains(a[0], "clang") {
		// disable ASCII art in clang errors, if possible
		a = append(a, "-fno-caret-diagnostics")
		// clang is too smart about command-line arguments
		a = append(a, "-Qunused-arguments")
	}

	return a
}

// gccArchArgs returns arguments to pass to gcc based on the architecture.
func (b *builder) gccArchArgs() []string {
	switch archChar {
	case "8":
		return []string{"-m32"}
	case "6":
		return []string{"-m64"}
	case "5":
		return []string{"-marm"} // not thumb
	}
	return nil
}
