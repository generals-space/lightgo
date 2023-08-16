// 本文件由 build.go 文件拆分而来
// 20230824
package main

import (
	"fmt"
	"os"
	"path/filepath"
)

func runInstall(cmd *Command, args []string) {
	raceInit()
	pkgs := packagesForBuild(args)

	for _, p := range pkgs {
		if p.Target == "" && (!p.Standard || p.ImportPath != "unsafe") {
			if p.cmdline {
				errorf("go install: no install location for .go files listed on command line (GOBIN not set)")
			} else if p.ConflictDir != "" {
				errorf("go install: no install location for %s: hidden by %s", p.Dir, p.ConflictDir)
			} else {
				errorf("go install: no install location for directory %s outside GOPATH", p.Dir)
			}
		}
	}
	exitIfErrors()

	var b builder
	b.init()
	a := &action{}
	for _, p := range pkgs {
		a.deps = append(a.deps, b.action(modeInstall, modeInstall, p))
	}
	b.do(a)
}

// install is the action for installing a single package or executable.
func (b *builder) install(a *action) (err error) {
	defer func() {
		if err != nil && err != errPrintedOutput {
			err = fmt.Errorf("go install %s: %v", a.p.ImportPath, err)
		}
	}()
	a1 := a.deps[0]
	perm := os.FileMode(0666)
	if a1.link {
		perm = 0777
	}

	// make target directory
	dir, _ := filepath.Split(a.target)
	if dir != "" {
		if err := b.mkdir(dir); err != nil {
			return err
		}
	}

	// remove object dir to keep the amount of
	// garbage down in a large build.  On an operating system
	// with aggressive buffering, cleaning incrementally like
	// this keeps the intermediate objects from hitting the disk.
	if !buildWork {
		defer os.RemoveAll(a1.objdir)
		defer os.Remove(a1.target)
	}

	if a.p.usesSwig() {
		for _, f := range stringList(a.p.SwigFiles, a.p.SwigCXXFiles) {
			dir = a.p.swigDir(&buildContext)
			if err := b.mkdir(dir); err != nil {
				return err
			}
			soname := a.p.swigSoname(f)
			source := filepath.Join(a.objdir, soname)
			target := filepath.Join(dir, soname)
			if err = b.copyFile(a, target, source, perm); err != nil {
				return err
			}
		}
	}

	return b.copyFile(a, a.target, a1.target, perm)
}
