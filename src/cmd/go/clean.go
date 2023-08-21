// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
)

var cleanI bool // clean -i flag
var cleanN bool // clean -n flag
var cleanR bool // clean -r flag
var cleanX bool // clean -x flag

func init() {
	// break init cycle
	cmdClean.Run = runClean

	cmdClean.Flag.BoolVar(&cleanI, "i", false, "")
	cmdClean.Flag.BoolVar(&cleanN, "n", false, "")
	cmdClean.Flag.BoolVar(&cleanR, "r", false, "")
	cmdClean.Flag.BoolVar(&cleanX, "x", false, "")
}

func runClean(cmd *Command, args []string) {
	for _, pkg := range packagesAndErrors(args) {
		clean(pkg)
	}
}

var cleaned = map[*Package]bool{}

// TODO: These are dregs left by Makefile-based builds.
// Eventually, can stop deleting these.
var cleanDir = map[string]bool{
	"_test": true,
	"_obj":  true,
}

var cleanFile = map[string]bool{
	"_testmain.go": true,
	"test.out":     true,
	"build.out":    true,
	"a.out":        true,
}

var cleanExt = map[string]bool{
	".5":  true,
	".6":  true,
	".8":  true,
	".a":  true,
	".o":  true,
	".so": true,
}

func clean(p *Package) {
	if cleaned[p] {
		return
	}
	cleaned[p] = true

	if p.Dir == "" {
		errorf("can't load package: %v", p.Error)
		return
	}
	dirs, err := ioutil.ReadDir(p.Dir)
	if err != nil {
		errorf("go clean %s: %v", p.Dir, err)
		return
	}

	var b builder
	b.print = fmt.Print

	packageFile := map[string]bool{}
	if p.Name != "main" {
		// Record which files are not in package main.
		// The others are.
		keep := func(list []string) {
			for _, f := range list {
				packageFile[f] = true
			}
		}
		keep(p.GoFiles)
		keep(p.CgoFiles)
		keep(p.TestGoFiles)
		keep(p.XTestGoFiles)
	}

	_, elem := filepath.Split(p.Dir)
	var allRemove []string

	// Remove dir-named executable only if this is package main.
	if p.Name == "main" {
		allRemove = append(allRemove,
			elem,
			elem+".exe",
		)
	}

	// Remove package test executables.
	allRemove = append(allRemove,
		elem+".test",
		elem+".test.exe",
	)

	// Remove a potental executable for each .go file in the directory that
	// is not part of the directory's package.
	for _, dir := range dirs {
		name := dir.Name()
		if packageFile[name] {
			continue
		}
		if !dir.IsDir() && strings.HasSuffix(name, ".go") {
			// TODO(adg,rsc): check that this .go file is actually
			// in "package main", and therefore capable of building
			// to an executable file.
			base := name[:len(name)-len(".go")]
			allRemove = append(allRemove, base, base+".exe")
		}
	}

	if cleanN || cleanX {
		b.showcmd(p.Dir, "rm -f %s", strings.Join(allRemove, " "))
	}

	toRemove := map[string]bool{}
	for _, name := range allRemove {
		toRemove[name] = true
	}
	for _, dir := range dirs {
		name := dir.Name()
		if dir.IsDir() {
			// TODO: Remove once Makefiles are forgotten.
			if cleanDir[name] {
				if cleanN || cleanX {
					b.showcmd(p.Dir, "rm -r %s", name)
					if cleanN {
						continue
					}
				}
				if err := os.RemoveAll(filepath.Join(p.Dir, name)); err != nil {
					errorf("go clean: %v", err)
				}
			}
			continue
		}

		if cleanN {
			continue
		}

		if cleanFile[name] || cleanExt[filepath.Ext(name)] || toRemove[name] {
			removeFile(filepath.Join(p.Dir, name))
		}
	}

	if cleanI && p.target != "" {
		if cleanN || cleanX {
			b.showcmd("", "rm -f %s", p.target)
		}
		if !cleanN {
			removeFile(p.target)
		}
	}

	if cleanI && p.usesSwig() {
		for _, f := range stringList(p.SwigFiles, p.SwigCXXFiles) {
			dir := p.swigDir(&buildContext)
			soname := p.swigSoname(f)
			target := filepath.Join(dir, soname)
			if cleanN || cleanX {
				b.showcmd("", "rm -f %s", target)
			}
			if !cleanN {
				removeFile(target)
			}
		}
	}

	if cleanR {
		for _, p1 := range p.imports {
			clean(p1)
		}
	}
}

// removeFile tries to remove file f, if error other than file doesn't exist
// occurs, it will report the error.
func removeFile(f string) {
	err := os.Remove(f)
	if err == nil || os.IsNotExist(err) {
		return
	}
	// Windows does not allow deletion of a binary file while it is executing.
	if toolIsWindows {
		// Remove lingering ~ file from last attempt.
		if _, err2 := os.Stat(f + "~"); err2 == nil {
			os.Remove(f + "~")
		}
		// Try to move it out of the way. If the move fails,
		// which is likely, we'll try again the
		// next time we do an install of this binary.
		if err2 := os.Rename(f, f+"~"); err2 == nil {
			os.Remove(f + "~")
			return
		}
	}
	errorf("go clean: %v", err)
}
