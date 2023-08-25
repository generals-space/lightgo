// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"strings"
	"sync"
)

// 	@param args: go build 所要构建的目标, 一般为 xxx.go,yyy.go 或是某个目录.
//
// caller:
// 	1. src/cmd/go/main.go -> main()
func runBuild(cmd *Command, args []string) {
	raceInit()
	var b builder
	b.init()

	pkgs := packagesForBuild(args)

	if len(pkgs) == 1 && pkgs[0].Name == "main" && *buildO == "" {
		_, *buildO = path.Split(pkgs[0].ImportPath)
		*buildO += exeSuffix
	}

	// sanity check some often mis-used options
	switch buildContext.Compiler {
	case "gccgo":
		if len(buildGcflags) != 0 {
			fmt.Println("go build: when using gccgo toolchain, please pass compiler flags using -gccgoflags, not -gcflags")
		}
		if len(buildLdflags) != 0 {
			fmt.Println("go build: when using gccgo toolchain, please pass linker flags using -gccgoflags, not -ldflags")
		}
	case "gc":
		if len(buildGccgoflags) != 0 {
			fmt.Println("go build: when using gc toolchain, please pass compile flags using -gcflags, and linker flags using -ldflags")
		}
	}

	// 一般来说 go build -o 参数是不会为空的, 必须指定一个输出文件.
	if *buildO != "" {
		// go build 的目标必须是单一的一个 package(一般为 main 包)
		if len(pkgs) > 1 {
			fatalf("go build: cannot use -o with multiple packages")
		}
		p := pkgs[0]
		p.target = "" // must build - not up to date
		a := b.action(modeInstall, modeBuild, p)
		a.target = *buildO
		b.do(a)
		return
	}

	a := &action{}
	for _, p := range packages(args) {
		a.deps = append(a.deps, b.action(modeBuild, modeBuild, p))
	}
	b.do(a)
}

// A builder holds global state about a build.
// It does not hold per-package state, because we
// build packages in parallel, and the builder is shared.
type builder struct {
	// 如 /tmp/go-build642732376
	// the temporary work directory (ends in filepath.Separator)
	work        string
	actionCache map[cacheKey]*action // a cache of already-constructed actions
	mkdirCache  map[string]bool      // a cache of created directories
	print       func(args ...interface{}) (int, error)

	output    sync.Mutex
	scriptDir string // current directory in printed script

	exec      sync.Mutex
	readySema chan bool
	ready     actionQueue
}

// cacheKey is the key for the action cache.
type cacheKey struct {
	mode buildMode
	p    *Package
}

func (b *builder) init() {
	var err error
	b.print = func(a ...interface{}) (int, error) {
		return fmt.Fprint(os.Stderr, a...)
	}
	b.actionCache = make(map[cacheKey]*action)
	b.mkdirCache = make(map[string]bool)

	if buildN {
		b.work = "$WORK"
	} else {
		b.work, err = ioutil.TempDir("", "go-build")
		if err != nil {
			fatalf("%s", err)
		}
		if buildX || buildWork {
			fmt.Fprintf(os.Stderr, "WORK=%s\n", b.work)
		}
		if !buildWork {
			atexit(func() { os.RemoveAll(b.work) })
		}
	}
}

// 	@param root: 一般为 main 包的 action, deps 中包含着 main 包引入的依赖包信息.
//
// caller:
// 	1. runBuild()
//
// do runs the action graph rooted at root.
func (b *builder) do(root *action) {
	// 从根节点进行深度优先遍历, 得到的所有节点.
	// 将 root 具象化成树, all 可以看成是从最底层叶子节点一层一层向上排列的列表.
	//
	// Build list of all actions, assigning depth-first post-order priority.
	// The original implementation here was a true queue
	// (using a channel) but it had the effect of getting
	// distracted by low-level leaf actions to the detriment
	// of completing higher-level actions.
	// The order of work does not matter much to overall execution time,
	// but when running "go test std" it is nice to see each test
	// results as soon as possible.
	// The priorities assigned ensure that, all else being equal,
	// the execution prefers to do what it would have done first in
	// a simple depth-first(深度优先) dependency order traversal.
	all := actionList(root)
	for i, a := range all {
		a.priority = i
	}

	b.readySema = make(chan bool, len(all))

	// Initialize per-action execution state.
	for _, a := range all {
		for _, a1 := range a.deps {
			// a1 是 a 的依赖包, triggers 中保留着哪些 package 依赖了自己.
			a1.triggers = append(a1.triggers, a)
		}
		// pending 表示 a 包所依赖的子包中, 还未编译的数量.
		// 当 pending = 0 时, 表示所有条件已准备就绪, 可以编译 a 包本身了.
		a.pending = len(a.deps)
		if a.pending == 0 {
			b.ready.push(a)
			b.readySema <- true
		}
	}

	// Handle runs a single action and takes care of triggering
	// any actions that are runnable as a result.
	handle := func(a *action) {
		var err error
		if a.f != nil && (!a.failed || a.ignoreFail) {
			err = a.f(b, a)
		}

		// The actions run in parallel but all the updates to the
		// shared work state are serialized through b.exec.
		b.exec.Lock()
		defer b.exec.Unlock()

		if err != nil {
			if err == errPrintedOutput {
				setExitStatus(2)
			} else {
				errorf("%s", err)
			}
			a.failed = true
		}

		// 当前 action 已经构建完成, 可以通知父级 action 了.
		for _, a0 := range a.triggers {
			if a.failed {
				a0.failed = true
			}
			// 当前 action 构建完成, 父级 action 所依赖的包就少了一个,
			// 但也可能父级 action 还有其他子 action 没编译, 可以用 pending 表示.
			// 如果确认父级 action 的 pending = 0, 则可以进行父级 action 的编译了.
			if a0.pending--; a0.pending == 0 {
				b.ready.push(a0)
				b.readySema <- true
			}
		}

		// 由于在 go build 时, 依赖关系是已树结构形式存在的, 而且一定是优先构建
		// 最底层的 package(不依赖其他 package 的包).
		// 当 action == root 时, 表示最后的结构被构建完成(一般是 main 包).
		if a == root {
			close(b.readySema)
		}
	}

	var wg sync.WaitGroup

	// par 并行构建的线程数
	//
	// Kick off goroutines according to parallelism.
	// If we are using the -n flag (just printing commands)
	// drop the parallelism to 1, both to make the output
	// deterministic and because there is no real work anyway.
	par := buildP
	if buildN {
		par = 1
	}
	for i := 0; i < par; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for {
				select {
				case _, ok := <-b.readySema:
					if !ok {
						return
					}
					// Receiving a value from b.readySema entitles
					// us to take from the ready queue.
					b.exec.Lock()
					a := b.ready.pop()
					b.exec.Unlock()
					handle(a)
				case <-interrupted:
					setExitStatus(1)
					return
				}
			}
		}()
	}
	// 等待上面 for{} 循环中所有 go func() 后台线程全部完成.
	wg.Wait()
}

// 在 builder.action() 的"case modeBuild"块中被赋值到 action.f 成员
//
// caller:
// 	1. builder.do() -> handle() 作为 a.f() 方法被调用.
//
// build is the action for building a single package or command.
func (b *builder) build(a *action) (err error) {
	// Return an error if the package has CXX files but it's not using
	// cgo nor SWIG, since the CXX files can only be processed by cgo
	// and SWIG (it's possible to have packages with C files without
	// using cgo, they will get compiled with the plan9 C compiler and
	// linked with the rest of the package).
	if len(a.p.CXXFiles) > 0 && !a.p.usesCgo() && !a.p.usesSwig() {
		return fmt.Errorf(
			"can't build package %s because it contains C++ files (%s) but it's not using cgo nor SWIG",
			a.p.ImportPath, strings.Join(a.p.CXXFiles, ","),
		)
	}
	defer func() {
		if err != nil && err != errPrintedOutput {
			err = fmt.Errorf("go build %s: %v", a.p.ImportPath, err)
		}
	}()
	if buildN {
		// In -n mode, print a banner between packages.
		// The banner is five lines so that when changes to
		// different sections of the bootstrap script have to be merged,
		// the banners give patch something to use to find its context.
		fmt.Printf("\n#\n# %s\n#\n\n", a.p.ImportPath)
	}

	if buildV {
		fmt.Fprintf(os.Stderr, "%s\n", a.p.ImportPath)
	}

	if a.p.Standard && a.p.ImportPath == "runtime" && buildContext.Compiler == "gc" &&
		!hasString(a.p.HFiles, "zasm_"+buildContext.GOOS+"_"+buildContext.GOARCH+".h") {
		return fmt.Errorf(
			"%s/%s must be bootstrapped using make%v",
			buildContext.GOOS, buildContext.GOARCH, defaultSuffix(),
		)
	}

	// obj: 如 /tmp/go-build321795794/sort/_obj/
	// Make build directory.
	obj := a.objdir
	if err := b.mkdir(obj); err != nil {
		return err
	}

	// make target directory
	dir, _ := filepath.Split(a.target)
	if dir != "" {
		if err := b.mkdir(dir); err != nil {
			return err
		}
	}

	var gofiles, cfiles, sfiles, objects, cgoObjects []string

	// If we're doing coverage, preprocess the .go files and put them in the work directory
	if a.p.coverMode != "" {
		for _, file := range a.p.GoFiles {
			sourceFile := filepath.Join(a.p.Dir, file)
			cover := a.p.coverVars[file]
			if cover == nil || isTestFile(file) {
				// Not covering this file.
				gofiles = append(gofiles, file)
				continue
			}
			coverFile := filepath.Join(obj, file)
			if err := b.cover(a, coverFile, sourceFile, 0666, cover.Var); err != nil {
				return err
			}
			gofiles = append(gofiles, coverFile)
		}
	} else {
		gofiles = append(gofiles, a.p.GoFiles...)
	}
	cfiles = append(cfiles, a.p.CFiles...)
	sfiles = append(sfiles, a.p.SFiles...)

	// Run cgo.
	if a.p.usesCgo() {
		// In a package using cgo, cgo compiles the C, C++ and assembly files with gcc.
		// There is one exception: runtime/cgo's job is to bridge the
		// cgo and non-cgo worlds, so it necessarily has files in both.
		// In that case gcc only gets the gcc_* files.
		var gccfiles []string
		if a.p.Standard && a.p.ImportPath == "runtime/cgo" {
			filter := func(files, nongcc, gcc []string) ([]string, []string) {
				for _, f := range files {
					if strings.HasPrefix(f, "gcc_") {
						gcc = append(gcc, f)
					} else {
						nongcc = append(nongcc, f)
					}
				}
				return nongcc, gcc
			}
			cfiles, gccfiles = filter(cfiles, cfiles[:0], gccfiles)
			sfiles, gccfiles = filter(sfiles, sfiles[:0], gccfiles)
		} else {
			gccfiles = append(cfiles, sfiles...)
			cfiles = nil
			sfiles = nil
		}

		cgoExe := tool("cgo")
		if a.cgo != nil && a.cgo.target != "" {
			cgoExe = a.cgo.target
		}
		outGo, outObj, err := b.cgo(a.p, cgoExe, obj, gccfiles, a.p.CXXFiles)
		if err != nil {
			return err
		}
		cgoObjects = append(cgoObjects, outObj...)
		gofiles = append(gofiles, outGo...)
	}

	// Run SWIG.
	if a.p.usesSwig() {
		// In a package using SWIG, any .c or .s files are
		// compiled with gcc.
		gccfiles := append(cfiles, sfiles...)
		cfiles = nil
		sfiles = nil
		outGo, outObj, err := b.swig(a.p, obj, gccfiles, a.p.CXXFiles)
		if err != nil {
			return err
		}
		cgoObjects = append(cgoObjects, outObj...)
		gofiles = append(gofiles, outGo...)
	}

	// inc 如 ["-I", "/tmp/go-build321795794"]
	// Prepare Go import path list.
	inc := b.includeArgs("-I", a.deps)

	////////////////////////////////////////////////////////////////////////////
	// 编译开始
	// Compile Go.
	if len(gofiles) > 0 {
		// ofile /tmp/go-build642732376/sort/_obj/_go_.6
		ofile, out, err := buildToolchain.gc(b, a.p, obj, inc, gofiles)
		if len(out) > 0 {
			b.showOutput(a.p.Dir, a.p.ImportPath, b.processOutput(out))
			if err != nil {
				return errPrintedOutput
			}
		}
		if err != nil {
			return err
		}
		objects = append(objects, ofile)
	}

	// Copy .h files named for goos or goarch or goos_goarch
	// to names using GOOS and GOARCH.
	// For example, defs_linux_amd64.h becomes defs_GOOS_GOARCH.h.
	_goos_goarch := "_" + goos + "_" + goarch
	_goos := "_" + goos
	_goarch := "_" + goarch
	for _, file := range a.p.HFiles {
		name, ext := fileExtSplit(file)
		switch {
		case strings.HasSuffix(name, _goos_goarch):
			targ := file[:len(name)-len(_goos_goarch)] + "_GOOS_GOARCH." + ext
			if err := b.copyFile(a, obj+targ, filepath.Join(a.p.Dir, file), 0666); err != nil {
				return err
			}
		case strings.HasSuffix(name, _goarch):
			targ := file[:len(name)-len(_goarch)] + "_GOARCH." + ext
			if err := b.copyFile(a, obj+targ, filepath.Join(a.p.Dir, file), 0666); err != nil {
				return err
			}
		case strings.HasSuffix(name, _goos):
			targ := file[:len(name)-len(_goos)] + "_GOOS." + ext
			if err := b.copyFile(a, obj+targ, filepath.Join(a.p.Dir, file), 0666); err != nil {
				return err
			}
		}
	}

	objExt := archChar
	if _, ok := buildToolchain.(gccgoToolchain); ok {
		objExt = "o"
	}

	// clang源码
	for _, file := range cfiles {
		out := file[:len(file)-len(".c")] + "." + objExt
		if err := buildToolchain.cc(b, a.p, obj, obj+out, file); err != nil {
			return err
		}
		objects = append(objects, out)
	}
	// 汇编源码
	// Assemble .s files.
	for _, file := range sfiles {
		out := file[:len(file)-len(".s")] + "." + objExt
		if err := buildToolchain.asm(b, a.p, obj, obj+out, file); err != nil {
			return err
		}
		objects = append(objects, out)
	}

	// NOTE(rsc): On Windows, it is critically important that the
	// gcc-compiled objects (cgoObjects) be listed after the ordinary
	// objects in the archive.  I do not know why this is.
	// http://golang.org/issue/2601
	objects = append(objects, cgoObjects...)

	// Add system object files.
	for _, syso := range a.p.SysoFiles {
		objects = append(objects, filepath.Join(a.p.Dir, syso))
	}

	// Pack into archive in obj directory
	if err := buildToolchain.pack(b, a.p, obj, a.objpkg, objects); err != nil {
		return err
	}

	// 依赖包的 link 都是 false, 只有最终的 main 包才是 true, 进入该 if 块,
	// 生成最终的二进制可执行文件.
	// Link if needed.
	if a.link {
		// The compiler only cares about direct imports,
		// but the linker needs the whole dependency tree.
		all := actionList(a)
		all = all[:len(all)-1] // drop a
		if err := buildToolchain.ld(b, a.p, a.target, all, a.objpkg, objects); err != nil {
			return err
		}
	}

	return nil
}
