package main

import (
	"log"
)

// toolchain 由 gcToolchain{} 结构体实现
type toolchain interface {
	// gc runs the compiler in a specific directory on a set of files
	// and returns the name of the generated output file.
	// The compiler runs in the directory dir.
	gc(b *builder, p *Package, obj string, importArgs []string, gofiles []string) (ofile string, out []byte, err error)
	// cc runs the toolchain's C compiler in a directory on a C file
	// to produce an output file.
	cc(b *builder, p *Package, objdir, ofile, cfile string) error
	// asm runs the assembler in a specific directory on a specific file
	// to generate the named output file.
	asm(b *builder, p *Package, obj, ofile, sfile string) error
	// pkgpath builds an appropriate path for a temporary package file.
	pkgpath(basedir string, p *Package) string
	// pack runs the archive packer in a specific directory to create
	// an archive from a set of object files.
	// typically it is run in the object directory.
	pack(b *builder, p *Package, objDir, afile string, ofiles []string) error
	// ld runs the linker to create a package starting at mainpkg.
	ld(b *builder, p *Package, out string, allactions []*action, mainpkg string, ofiles []string) error

	compiler() string
	linker() string
}

type noToolchain struct{}

func noCompiler() error {
	log.Fatalf("unknown compiler %q", buildContext.Compiler)
	return nil
}

func (noToolchain) compiler() string {
	noCompiler()
	return ""
}

func (noToolchain) linker() string {
	noCompiler()
	return ""
}

func (noToolchain) gc(b *builder, p *Package, obj string, importArgs []string, gofiles []string) (ofile string, out []byte, err error) {
	return "", nil, noCompiler()
}

func (noToolchain) asm(b *builder, p *Package, obj, ofile, sfile string) error {
	return noCompiler()
}

func (noToolchain) pkgpath(basedir string, p *Package) string {
	noCompiler()
	return ""
}

func (noToolchain) pack(b *builder, p *Package, objDir, afile string, ofiles []string) error {
	return noCompiler()
}

func (noToolchain) ld(b *builder, p *Package, out string, allactions []*action, mainpkg string, ofiles []string) error {
	return noCompiler()
}

func (noToolchain) cc(b *builder, p *Package, objdir, ofile, cfile string) error {
	return noCompiler()
}
