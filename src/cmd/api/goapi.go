// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build api_tool

// Binary api computes the exported API of a set of Go packages.
package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"go/ast"
	"go/build"
	"go/parser"
	"go/token"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strings"

	"code.google.com/p/go.tools/go/types"
)

// Flags
var (
	checkFile  = flag.String("c", "", "optional comma-separated filename(s) to check API against")
	allowNew   = flag.Bool("allow_new", true, "allow API additions")
	exceptFile = flag.String("except", "", "optional filename of packages that are allowed to change without triggering a failure in the tool")
	nextFile   = flag.String("next", "", "optional filename of tentative upcoming API features for the next release. This file can be lazily maintained. It only affects the delta warnings from the -c file printed on success.")
	verbose    = flag.Bool("v", false, "verbose debugging")
	forceCtx   = flag.String("contexts", "", "optional comma-separated list of <goos>-<goarch>[-cgo] to override default contexts.")
)

// contexts are the default contexts which are scanned, unless
// overridden by the -contexts flag.
var contexts = []*build.Context{
	{GOOS: "linux", GOARCH: "386", CgoEnabled: true},
	{GOOS: "linux", GOARCH: "386"},
	{GOOS: "linux", GOARCH: "amd64", CgoEnabled: true},
	{GOOS: "linux", GOARCH: "amd64"},
	{GOOS: "darwin", GOARCH: "386", CgoEnabled: true},
	{GOOS: "darwin", GOARCH: "386"},
	{GOOS: "darwin", GOARCH: "amd64", CgoEnabled: true},
	{GOOS: "darwin", GOARCH: "amd64"},
	{GOOS: "windows", GOARCH: "amd64"},
	{GOOS: "windows", GOARCH: "386"},
	{GOOS: "freebsd", GOARCH: "386", CgoEnabled: true},
	{GOOS: "freebsd", GOARCH: "386"},
	{GOOS: "freebsd", GOARCH: "amd64", CgoEnabled: true},
	{GOOS: "freebsd", GOARCH: "amd64"},
	{GOOS: "netbsd", GOARCH: "386", CgoEnabled: true},
	{GOOS: "netbsd", GOARCH: "386"},
	{GOOS: "netbsd", GOARCH: "amd64", CgoEnabled: true},
	{GOOS: "netbsd", GOARCH: "amd64"},
	{GOOS: "openbsd", GOARCH: "386", CgoEnabled: true},
	{GOOS: "openbsd", GOARCH: "386"},
	{GOOS: "openbsd", GOARCH: "amd64", CgoEnabled: true},
	{GOOS: "openbsd", GOARCH: "amd64"},
}

func contextName(c *build.Context) string {
	s := c.GOOS + "-" + c.GOARCH
	if c.CgoEnabled {
		return s + "-cgo"
	}
	return s
}

func parseContext(c string) *build.Context {
	parts := strings.Split(c, "-")
	if len(parts) < 2 {
		log.Fatalf("bad context: %q", c)
	}
	bc := &build.Context{
		GOOS:   parts[0],
		GOARCH: parts[1],
	}
	if len(parts) == 3 {
		if parts[2] == "cgo" {
			bc.CgoEnabled = true
		} else {
			log.Fatalf("bad context: %q", c)
		}
	}
	return bc
}

func setContexts() {
	contexts = []*build.Context{}
	for _, c := range strings.Split(*forceCtx, ",") {
		contexts = append(contexts, parseContext(c))
	}
}

func main() {
	flag.Parse()

	if !strings.Contains(runtime.Version(), "weekly") && !strings.Contains(runtime.Version(), "devel") {
		if *nextFile != "" {
			fmt.Printf("Go version is %q, ignoring -next %s\n", runtime.Version(), *nextFile)
			*nextFile = ""
		}
	}

	if *forceCtx != "" {
		setContexts()
	}
	for _, c := range contexts {
		c.Compiler = build.Default.Compiler
	}

	var pkgNames []string
	if flag.NArg() > 0 {
		pkgNames = flag.Args()
	} else {
		stds, err := exec.Command("go", "list", "std").Output()
		if err != nil {
			log.Fatal(err)
		}
		pkgNames = strings.Fields(string(stds))
	}

	var featureCtx = make(map[string]map[string]bool) // feature -> context name -> true
	for _, context := range contexts {
		w := NewWalker(context, filepath.Join(build.Default.GOROOT, "src/pkg"))

		for _, name := range pkgNames {
			// - Package "unsafe" contains special signatures requiring
			//   extra care when printing them - ignore since it is not
			//   going to change w/o a language change.
			// - We don't care about the API of commands.
			if name != "unsafe" && !strings.HasPrefix(name, "cmd/") {
				if name == "runtime/cgo" && !context.CgoEnabled {
					// w.Import(name) will return nil
					continue
				}
				w.export(w.Import(name))
			}
		}

		ctxName := contextName(context)
		for _, f := range w.Features() {
			if featureCtx[f] == nil {
				featureCtx[f] = make(map[string]bool)
			}
			featureCtx[f][ctxName] = true
		}
	}

	var features []string
	for f, cmap := range featureCtx {
		if len(cmap) == len(contexts) {
			features = append(features, f)
			continue
		}
		comma := strings.Index(f, ",")
		for cname := range cmap {
			f2 := fmt.Sprintf("%s (%s)%s", f[:comma], cname, f[comma:])
			features = append(features, f2)
		}
	}

	fail := false
	defer func() {
		if fail {
			os.Exit(1)
		}
	}()

	bw := bufio.NewWriter(os.Stdout)
	defer bw.Flush()

	if *checkFile == "" {
		sort.Strings(features)
		for _, f := range features {
			fmt.Fprintln(bw, f)
		}
		return
	}

	var required []string
	for _, file := range strings.Split(*checkFile, ",") {
		required = append(required, fileFeatures(file)...)
	}
	optional := fileFeatures(*nextFile)
	exception := fileFeatures(*exceptFile)
	fail = !compareAPI(bw, features, required, optional, exception)
}

// export emits the exported package features.
func (w *Walker) export(pkg *types.Package) {
	if *verbose {
		log.Println(pkg)
	}
	pop := w.pushScope("pkg " + pkg.Path())
	w.current = pkg
	scope := pkg.Scope()
	for _, name := range scope.Names() {
		if ast.IsExported(name) {
			w.emitObj(scope.Lookup(name))
		}
	}
	pop()
}

func set(items []string) map[string]bool {
	s := make(map[string]bool)
	for _, v := range items {
		s[v] = true
	}
	return s
}

var spaceParensRx = regexp.MustCompile(` \(\S+?\)`)

func featureWithoutContext(f string) string {
	if !strings.Contains(f, "(") {
		return f
	}
	return spaceParensRx.ReplaceAllString(f, "")
}

func compareAPI(w io.Writer, features, required, optional, exception []string) (ok bool) {
	ok = true

	optionalSet := set(optional)
	exceptionSet := set(exception)
	featureSet := set(features)

	sort.Strings(features)
	sort.Strings(required)

	take := func(sl *[]string) string {
		s := (*sl)[0]
		*sl = (*sl)[1:]
		return s
	}

	for len(required) > 0 || len(features) > 0 {
		switch {
		case len(features) == 0 || (len(required) > 0 && required[0] < features[0]):
			feature := take(&required)
			if exceptionSet[feature] {
				// An "unfortunate" case: the feature was once
				// included in the API (e.g. go1.txt), but was
				// subsequently removed. These are already
				// acknowledged by being in the file
				// "api/except.txt". No need to print them out
				// here.
			} else if featureSet[featureWithoutContext(feature)] {
				// okay.
			} else {
				fmt.Fprintf(w, "-%s\n", feature)
				ok = false // broke compatibility
			}
		case len(required) == 0 || (len(features) > 0 && required[0] > features[0]):
			newFeature := take(&features)
			if optionalSet[newFeature] {
				// Known added feature to the upcoming release.
				// Delete it from the map so we can detect any upcoming features
				// which were never seen.  (so we can clean up the nextFile)
				delete(optionalSet, newFeature)
			} else {
				fmt.Fprintf(w, "+%s\n", newFeature)
				if !*allowNew {
					ok = false // we're in lock-down mode for next release
				}
			}
		default:
			take(&required)
			take(&features)
		}
	}

	// In next file, but not in API.
	var missing []string
	for feature := range optionalSet {
		missing = append(missing, feature)
	}
	sort.Strings(missing)
	for _, feature := range missing {
		fmt.Fprintf(w, "±%s\n", feature)
	}
	return
}

func fileFeatures(filename string) []string {
	if filename == "" {
		return nil
	}
	bs, err := ioutil.ReadFile(filename)
	if err != nil {
		log.Fatalf("Error reading file %s: %v", filename, err)
	}
	text := strings.TrimSpace(string(bs))
	if text == "" {
		return nil
	}
	return strings.Split(text, "\n")
}

var fset = token.NewFileSet()

type Walker struct {
	context  *build.Context
	root     string
	scope    []string
	current  *types.Package
	features map[string]bool           // set
	imported map[string]*types.Package // packages already imported
}

func NewWalker(context *build.Context, root string) *Walker {
	return &Walker{
		context:  context,
		root:     root,
		features: map[string]bool{},
		imported: map[string]*types.Package{"unsafe": types.Unsafe},
	}
}

func (w *Walker) Features() (fs []string) {
	for f := range w.features {
		fs = append(fs, f)
	}
	sort.Strings(fs)
	return
}

var parsedFileCache = make(map[string]*ast.File)

func (w *Walker) parseFile(dir, file string) (*ast.File, error) {
	filename := filepath.Join(dir, file)
	f, _ := parsedFileCache[filename]
	if f != nil {
		return f, nil
	}

	var err error

	// generate missing context-dependent files.

	if w.context != nil && file == fmt.Sprintf("zgoos_%s.go", w.context.GOOS) {
		src := fmt.Sprintf("package runtime; const theGoos = `%s`", w.context.GOOS)
		f, err = parser.ParseFile(fset, filename, src, 0)
		if err != nil {
			log.Fatalf("incorrect generated file: %s", err)
		}
	}

	if w.context != nil && file == fmt.Sprintf("zgoarch_%s.go", w.context.GOARCH) {
		src := fmt.Sprintf("package runtime; const theGoarch = `%s`", w.context.GOARCH)
		f, err = parser.ParseFile(fset, filename, src, 0)
		if err != nil {
			log.Fatalf("incorrect generated file: %s", err)
		}
	}

	if f == nil {
		f, err = parser.ParseFile(fset, filename, nil, 0)
		if err != nil {
			return nil, err
		}
	}

	parsedFileCache[filename] = f
	return f, nil
}

func contains(list []string, s string) bool {
	for _, t := range list {
		if t == s {
			return true
		}
	}
	return false
}

var (
	pkgCache = map[string]*types.Package{} // map tagKey to package
	pkgTags  = map[string][]string{}       // map import dir to list of relevant tags
)

// tagKey returns the tag-based key to use in the pkgCache.
// It is a comma-separated string; the first part is dir, the rest tags.
// The satisfied tags are derived from context but only those that
// matter (the ones listed in the tags argument) are used.
// The tags list, which came from go/build's Package.AllTags,
// is known to be sorted.
func tagKey(dir string, context *build.Context, tags []string) string {
	ctags := map[string]bool{
		context.GOOS:   true,
		context.GOARCH: true,
	}
	if context.CgoEnabled {
		ctags["cgo"] = true
	}
	for _, tag := range context.BuildTags {
		ctags[tag] = true
	}
	// TODO: ReleaseTags (need to load default)
	key := dir
	for _, tag := range tags {
		if ctags[tag] {
			key += "," + tag
		}
	}
	return key
}

// Importing is a sentinel taking the place in Walker.imported
// for a package that is in the process of being imported.
var importing types.Package

func (w *Walker) Import(name string) (pkg *types.Package) {
	pkg = w.imported[name]
	if pkg != nil {
		if pkg == &importing {
			log.Fatalf("cycle importing package %q", name)
		}
		return pkg
	}
	w.imported[name] = &importing

	// Determine package files.
	dir := filepath.Join(w.root, filepath.FromSlash(name))
	if fi, err := os.Stat(dir); err != nil || !fi.IsDir() {
		log.Fatalf("no source in tree for package %q", pkg)
	}

	context := w.context
	if context == nil {
		context = &build.Default
	}

	// Look in cache.
	// If we've already done an import with the same set
	// of relevant tags, reuse the result.
	var key string
	if tags, ok := pkgTags[dir]; ok {
		key = tagKey(dir, context, tags)
		if pkg := pkgCache[key]; pkg != nil {
			w.imported[name] = pkg
			return pkg
		}
	}

	info, err := context.ImportDir(dir, 0)
	if err != nil {
		if _, nogo := err.(*build.NoGoError); nogo {
			return
		}
		log.Fatalf("pkg %q, dir %q: ScanDir: %v", name, dir, err)
	}

	// Save tags list first time we see a directory.
	if _, ok := pkgTags[dir]; !ok {
		pkgTags[dir] = info.AllTags
		key = tagKey(dir, context, info.AllTags)
	}

	filenames := append(append([]string{}, info.GoFiles...), info.CgoFiles...)

	// Certain files only exist when building for the specified context.
	// Add them manually.
	if name == "runtime" {
		n := fmt.Sprintf("zgoos_%s.go", w.context.GOOS)
		if !contains(filenames, n) {
			filenames = append(filenames, n)
		}

		n = fmt.Sprintf("zgoarch_%s.go", w.context.GOARCH)
		if !contains(filenames, n) {
			filenames = append(filenames, n)
		}
	}

	// Parse package files.
	var files []*ast.File
	for _, file := range filenames {
		f, err := w.parseFile(dir, file)
		if err != nil {
			log.Fatalf("error parsing package %s: %s", name, err)
		}
		files = append(files, f)
	}

	// Type-check package files.
	conf := types.Config{
		IgnoreFuncBodies: true,
		FakeImportC:      true,
		Import: func(imports map[string]*types.Package, name string) (*types.Package, error) {
			pkg := w.Import(name)
			imports[name] = pkg
			return pkg, nil
		},
	}
	pkg, err = conf.Check(name, fset, files, nil)
	if err != nil {
		ctxt := "<no context>"
		if w.context != nil {
			ctxt = fmt.Sprintf("%s-%s", w.context.GOOS, w.context.GOARCH)
		}
		log.Fatalf("error typechecking package %s: %s (%s)", name, err, ctxt)
	}

	pkgCache[key] = pkg

	w.imported[name] = pkg
	return
}

// pushScope enters a new scope (walking a package, type, node, etc)
// and returns a function that will leave the scope (with sanity checking
// for mismatched pushes & pops)
func (w *Walker) pushScope(name string) (popFunc func()) {
	w.scope = append(w.scope, name)
	return func() {
		if len(w.scope) == 0 {
			log.Fatalf("attempt to leave scope %q with empty scope list", name)
		}
		if w.scope[len(w.scope)-1] != name {
			log.Fatalf("attempt to leave scope %q, but scope is currently %#v", name, w.scope)
		}
		w.scope = w.scope[:len(w.scope)-1]
	}
}

func sortedMethodNames(typ *types.Interface) []string {
	n := typ.NumMethods()
	list := make([]string, n)
	for i := range list {
		list[i] = typ.Method(i).Name()
	}
	sort.Strings(list)
	return list
}

func (w *Walker) writeType(buf *bytes.Buffer, typ types.Type) {
	switch typ := typ.(type) {
	case *types.Basic:
		s := typ.Name()
		switch typ.Kind() {
		case types.UnsafePointer:
			s = "unsafe.Pointer"
		case types.UntypedBool:
			s = "ideal-bool"
		case types.UntypedInt:
			s = "ideal-int"
		case types.UntypedRune:
			// "ideal-char" for compatibility with old tool
			// TODO(gri) change to "ideal-rune"
			s = "ideal-char"
		case types.UntypedFloat:
			s = "ideal-float"
		case types.UntypedComplex:
			s = "ideal-complex"
		case types.UntypedString:
			s = "ideal-string"
		case types.UntypedNil:
			panic("should never see untyped nil type")
		default:
			switch s {
			case "byte":
				s = "uint8"
			case "rune":
				s = "int32"
			}
		}
		buf.WriteString(s)

	case *types.Array:
		fmt.Fprintf(buf, "[%d]", typ.Len())
		w.writeType(buf, typ.Elem())

	case *types.Slice:
		buf.WriteString("[]")
		w.writeType(buf, typ.Elem())

	case *types.Struct:
		buf.WriteString("struct")

	case *types.Pointer:
		buf.WriteByte('*')
		w.writeType(buf, typ.Elem())

	case *types.Tuple:
		panic("should never see a tuple type")

	case *types.Signature:
		buf.WriteString("func")
		w.writeSignature(buf, typ)

	case *types.Interface:
		buf.WriteString("interface{")
		if typ.NumMethods() > 0 {
			buf.WriteByte(' ')
			buf.WriteString(strings.Join(sortedMethodNames(typ), ", "))
			buf.WriteByte(' ')
		}
		buf.WriteString("}")

	case *types.Map:
		buf.WriteString("map[")
		w.writeType(buf, typ.Key())
		buf.WriteByte(']')
		w.writeType(buf, typ.Elem())

	case *types.Chan:
		var s string
		switch typ.Dir() {
		case ast.SEND:
			s = "chan<- "
		case ast.RECV:
			s = "<-chan "
		default:
			s = "chan "
		}
		buf.WriteString(s)
		w.writeType(buf, typ.Elem())

	case *types.Named:
		obj := typ.Obj()
		pkg := obj.Pkg()
		if pkg != nil && pkg != w.current {
			buf.WriteString(pkg.Name())
			buf.WriteByte('.')
		}
		buf.WriteString(typ.Obj().Name())

	default:
		panic(fmt.Sprintf("unknown type %T", typ))
	}
}

func (w *Walker) writeSignature(buf *bytes.Buffer, sig *types.Signature) {
	w.writeParams(buf, sig.Params(), sig.IsVariadic())
	switch res := sig.Results(); res.Len() {
	case 0:
		// nothing to do
	case 1:
		buf.WriteByte(' ')
		w.writeType(buf, res.At(0).Type())
	default:
		buf.WriteByte(' ')
		w.writeParams(buf, res, false)
	}
}

func (w *Walker) writeParams(buf *bytes.Buffer, t *types.Tuple, variadic bool) {
	buf.WriteByte('(')
	for i, n := 0, t.Len(); i < n; i++ {
		if i > 0 {
			buf.WriteString(", ")
		}
		typ := t.At(i).Type()
		if variadic && i+1 == n {
			buf.WriteString("...")
			typ = typ.(*types.Slice).Elem()
		}
		w.writeType(buf, typ)
	}
	buf.WriteByte(')')
}

func (w *Walker) typeString(typ types.Type) string {
	var buf bytes.Buffer
	w.writeType(&buf, typ)
	return buf.String()
}

func (w *Walker) signatureString(sig *types.Signature) string {
	var buf bytes.Buffer
	w.writeSignature(&buf, sig)
	return buf.String()
}

func (w *Walker) emitObj(obj types.Object) {
	switch obj := obj.(type) {
	case *types.Const:
		w.emitf("const %s %s", obj.Name(), w.typeString(obj.Type()))
		w.emitf("const %s = %s", obj.Name(), obj.Val())
	case *types.Var:
		w.emitf("var %s %s", obj.Name(), w.typeString(obj.Type()))
	case *types.TypeName:
		w.emitType(obj)
	case *types.Func:
		w.emitFunc(obj)
	default:
		panic("unknown object: " + obj.String())
	}
}

func (w *Walker) emitType(obj *types.TypeName) {
	name := obj.Name()
	typ := obj.Type()
	switch typ := typ.Underlying().(type) {
	case *types.Struct:
		w.emitStructType(name, typ)
	case *types.Interface:
		w.emitIfaceType(name, typ)
		return // methods are handled by emitIfaceType
	default:
		w.emitf("type %s %s", name, w.typeString(typ.Underlying()))
	}

	// emit methods with value receiver
	var methodNames map[string]bool
	vset := typ.MethodSet()
	for i, n := 0, vset.Len(); i < n; i++ {
		m := vset.At(i)
		if m.Obj().IsExported() {
			w.emitMethod(m)
			if methodNames == nil {
				methodNames = make(map[string]bool)
			}
			methodNames[m.Obj().Name()] = true
		}
	}

	// emit methods with pointer receiver; exclude
	// methods that we have emitted already
	// (the method set of *T includes the methods of T)
	pset := types.NewPointer(typ).MethodSet()
	for i, n := 0, pset.Len(); i < n; i++ {
		m := pset.At(i)
		if m.Obj().IsExported() && !methodNames[m.Obj().Name()] {
			w.emitMethod(m)
		}
	}
}

func (w *Walker) emitStructType(name string, typ *types.Struct) {
	typeStruct := fmt.Sprintf("type %s struct", name)
	w.emitf(typeStruct)
	defer w.pushScope(typeStruct)()

	for i := 0; i < typ.NumFields(); i++ {
		f := typ.Field(i)
		if !f.IsExported() {
			continue
		}
		typ := f.Type()
		if f.Anonymous() {
			w.emitf("embedded %s", w.typeString(typ))
			continue
		}
		w.emitf("%s %s", f.Name(), w.typeString(typ))
	}
}

func (w *Walker) emitIfaceType(name string, typ *types.Interface) {
	pop := w.pushScope("type " + name + " interface")

	var methodNames []string
	complete := true
	mset := typ.MethodSet()
	for i, n := 0, mset.Len(); i < n; i++ {
		m := mset.At(i).Obj().(*types.Func)
		if !m.IsExported() {
			complete = false
			continue
		}
		methodNames = append(methodNames, m.Name())
		w.emitf("%s%s", m.Name(), w.signatureString(m.Type().(*types.Signature)))
	}

	if !complete {
		// The method set has unexported methods, so all the
		// implementations are provided by the same package,
		// so the method set can be extended. Instead of recording
		// the full set of names (below), record only that there were
		// unexported methods. (If the interface shrinks, we will notice
		// because a method signature emitted during the last loop
		// will disappear.)
		w.emitf("unexported methods")
	}

	pop()

	if !complete {
		return
	}

	if len(methodNames) == 0 {
		w.emitf("type %s interface {}", name)
		return
	}

	sort.Strings(methodNames)
	w.emitf("type %s interface { %s }", name, strings.Join(methodNames, ", "))
}

func (w *Walker) emitFunc(f *types.Func) {
	sig := f.Type().(*types.Signature)
	if sig.Recv() != nil {
		panic("method considered a regular function: " + f.String())
	}
	w.emitf("func %s%s", f.Name(), w.signatureString(sig))
}

func (w *Walker) emitMethod(m *types.Selection) {
	sig := m.Type().(*types.Signature)
	recv := sig.Recv().Type()
	// report exported methods with unexported reveiver base type
	if true {
		base := recv
		if p, _ := recv.(*types.Pointer); p != nil {
			base = p.Elem()
		}
		if obj := base.(*types.Named).Obj(); !obj.IsExported() {
			log.Fatalf("exported method with unexported receiver base type: %s", m)
		}
	}
	w.emitf("method (%s) %s%s", w.typeString(recv), m.Obj().Name(), w.signatureString(sig))
}

func (w *Walker) emitf(format string, args ...interface{}) {
	f := strings.Join(w.scope, ", ") + ", " + fmt.Sprintf(format, args...)
	if strings.Contains(f, "\n") {
		panic("feature contains newlines: " + f)
	}

	if _, dup := w.features[f]; dup {
		panic("duplicate feature inserted: " + f)
	}
	w.features[f] = true

	if *verbose {
		log.Printf("feature: %s", f)
	}
}
