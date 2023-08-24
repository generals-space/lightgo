package main

import (
	"runtime"
	"path/filepath"
	"bytes"
)

// An action represents a single action in the action graph.
type action struct {
	p          *Package      // the package this action works on
	deps       []*action     // actions that must happen before this one
	triggers   []*action     // inverse of deps
	cgo        *action       // action for cgo binary if needed
	args       []string      // additional args for runProgram
	testOutput *bytes.Buffer // test output buffer

	// go run 时, 该函数为 src/cmd/go/run.go -> builder.runProgram()
	// 在 src/cmd/go/run.go -> runRun() 末尾被赋值
	//
	// go build 时, 该函数为 builder.build()
	// 在 builder.action() 的"case modeBuild"块中被赋值
	//
	// the action itself (nil = no-op)
	f          func(*builder, *action) error
	ignoreFail bool // whether to run f even if dependencies fail

	// Generated files, directories.

	// 当 package 为 main 包时, link 为 true, 如果是非 main 包则为 false.
	// target is executable, not just package
	link   bool
	pkgdir string // the -I or -L argument to use when importing this package
	objdir string // directory for intermediate objects
	objpkg string // the intermediate package .a file created during the action
	// 比如 go build 构建 main 包时, 通过 -o 参数指定的目标二进制文件(路径字符串).
	// goal of the action: the created package or executable
	target string

	// Execution state.
	pending  int  // number of deps yet to complete
	priority int  // relative execution priority
	failed   bool // whether the action failed
}

// action 按 import 层级组织 action 并返回, main 包的 action 就像是一棵树的根节点.
//
//                      action{main}
//                           deps
//                [action{log}, action{bytes}]
//                 deps                   deps
//                /                           \
//[action{fmt}, action{io}]     [action{unicode}, action{unicode/utf8}]
//
// caller:
// 	1. src/cmd/go/run.go -> runRun()
//
// action returns the action for applying the given operation (mode) to the package.
// depMode is the action to use when building dependencies.
func (b *builder) action(mode buildMode, depMode buildMode, p *Package) *action {
	key := cacheKey{mode, p}
	a := b.actionCache[key]
	if a != nil {
		return a
	}

	a = &action{p: p, pkgdir: p.build.PkgRoot}
	if p.pkgdir != "" { // overrides p.t
		a.pkgdir = p.pkgdir
	}

	b.actionCache[key] = a

	for _, p1 := range p.imports {
		a.deps = append(a.deps, b.action(depMode, depMode, p1))
	}

	// If we are not doing a cross-build, then record the binary we'll
	// generate for cgo as a dependency of the build of any package
	// using cgo, to make sure we do not overwrite the binary while
	// a package is using it.  If this is a cross-build, then the cgo we
	// are writing is not the cgo we need to use.
	if goos == runtime.GOOS && goarch == runtime.GOARCH && !buildRace {
		// 该 if 块只适用于 cgo 场景.
		if len(p.CgoFiles) > 0 || p.Standard && p.ImportPath == "runtime/cgo" {
			var stk importStack
			p1 := loadPackage("cmd/cgo", &stk)
			if p1.Error != nil {
				fatalf("load cmd/cgo: %v", p1.Error)
			}
			a.cgo = b.action(depMode, depMode, p1)
			a.deps = append(a.deps, a.cgo)
		}
	}

	if p.Standard {
		switch p.ImportPath {
		case "builtin", "unsafe":
			// Fake packages - nothing to build.
			return a
		}
		// gccgo standard library is "fake" too.
		if _, ok := buildToolchain.(gccgoToolchain); ok {
			// the target name is needed for cgo.
			a.target = p.target
			return a
		}
	}

	if !p.Stale && p.target != "" {
		// p.Stale==false implies that p.target is up-to-date.
		// Record target name for use by actions depending on this one.
		a.target = p.target
		return a
	}

	if p.local && p.target == "" {
		// 进入该块, 一般表示 p 为 main 包.
		// Imported via local path. No permanent target.
		mode = modeBuild
	}
	work := p.pkgdir
	if work == "" {
		work = b.work
	}
	a.objdir = filepath.Join(work, a.p.ImportPath, "_obj") + string(filepath.Separator)
	a.objpkg = buildToolchain.pkgpath(work, a.p)
	a.link = p.Name == "main"

	switch mode {
	case modeInstall:
		a.f = (*builder).install
		a.deps = []*action{b.action(modeBuild, depMode, p)}
		a.target = a.p.target
	case modeBuild:
		a.f = (*builder).build
		a.target = a.objpkg
		if a.link {
			// An executable file. (This is the name of a temporary file.)
			// Because we run the temporary file in 'go run' and 'go test',
			// the name will show up in ps listings. If the caller has specified
			// a name, use that instead of a.out. The binary is generated
			// in an otherwise empty subdirectory named exe to avoid
			// naming conflicts.  The only possible conflict is if we were
			// to create a top-level package named exe.
			name := "a.out"
			if p.exeName != "" {
				name = p.exeName
			}
			a.target = a.objdir + filepath.Join("exe", name) + exeSuffix
		}
	}

	return a
}

// 可以把 root 看成是一棵树的根节点, 本函数使用后序遍历返回其中的所有节点列表.
// 后序遍历: 左子树->右子树->树本身
// 
//    1
//  /   \      ->  2, 3, 1
// 2     3
//
// caller:
// 	1. builder.do()
// 	2. builder.build()
// 	3. src/cmd/go/test.go -> runTest()
//
// actionList returns the list of actions in the dag rooted at root
// as visited in a depth-first(深度优先) post-order traversal(后序遍历).
func actionList(root *action) []*action {
	seen := map[*action]bool{}
	all := []*action{}
	var walk func(*action)
	walk = func(a *action) {
		if seen[a] {
			return
		}
		seen[a] = true
		for _, a1 := range a.deps {
			walk(a1)
		}
		all = append(all, a)
	}
	walk(root)
	return all
}
