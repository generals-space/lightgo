// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package filepath_test

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"reflect"
	"runtime"
	"strings"
	"testing"
)

type PathTest struct {
	path, result string
}

const sep = filepath.Separator

var slashtests = []PathTest{
	{"", ""},
	{"/", string(sep)},
	{"/a/b", string([]byte{sep, 'a', sep, 'b'})},
	{"a//b", string([]byte{'a', sep, sep, 'b'})},
}

func TestFromAndToSlash(t *testing.T) {
	for _, test := range slashtests {
		if s := filepath.FromSlash(test.path); s != test.result {
			t.Errorf("FromSlash(%q) = %q, want %q", test.path, s, test.result)
		}
		if s := filepath.ToSlash(test.result); s != test.path {
			t.Errorf("ToSlash(%q) = %q, want %q", test.result, s, test.path)
		}
	}
}

type SplitListTest struct {
	list   string
	result []string
}

const lsep = filepath.ListSeparator

var splitlisttests = []SplitListTest{
	{"", []string{}},
	{string([]byte{'a', lsep, 'b'}), []string{"a", "b"}},
	{string([]byte{lsep, 'a', lsep, 'b'}), []string{"", "a", "b"}},
}

var winsplitlisttests = []SplitListTest{
	// quoted
	{`"a"`, []string{`a`}},

	// semicolon
	{`";"`, []string{`;`}},
	{`"a;b"`, []string{`a;b`}},
	{`";";`, []string{`;`, ``}},
	{`;";"`, []string{``, `;`}},

	// partially quoted
	{`a";"b`, []string{`a;b`}},
	{`a; ""b`, []string{`a`, ` b`}},
	{`"a;b`, []string{`a;b`}},
	{`""a;b`, []string{`a`, `b`}},
	{`"""a;b`, []string{`a;b`}},
	{`""""a;b`, []string{`a`, `b`}},
	{`a";b`, []string{`a;b`}},
	{`a;b";c`, []string{`a`, `b;c`}},
	{`"a";b";c`, []string{`a`, `b;c`}},
}

func TestSplitList(t *testing.T) {
	tests := splitlisttests
	if runtime.GOOS == "windows" {
		tests = append(tests, winsplitlisttests...)
	}
	for _, test := range tests {
		if l := filepath.SplitList(test.list); !reflect.DeepEqual(l, test.result) {
			t.Errorf("SplitList(%#q) = %#q, want %#q", test.list, l, test.result)
		}
	}
}

type SplitTest struct {
	path, dir, file string
}

var unixsplittests = []SplitTest{
	{"a/b", "a/", "b"},
	{"a/b/", "a/b/", ""},
	{"a/", "a/", ""},
	{"a", "", "a"},
	{"/", "/", ""},
}

var winsplittests = []SplitTest{
	{`c:`, `c:`, ``},
	{`c:/`, `c:/`, ``},
	{`c:/foo`, `c:/`, `foo`},
	{`c:/foo/bar`, `c:/foo/`, `bar`},
	{`//host/share`, `//host/share`, ``},
	{`//host/share/`, `//host/share/`, ``},
	{`//host/share/foo`, `//host/share/`, `foo`},
	{`\\host\share`, `\\host\share`, ``},
	{`\\host\share\`, `\\host\share\`, ``},
	{`\\host\share\foo`, `\\host\share\`, `foo`},
}

func TestSplit(t *testing.T) {
	var splittests []SplitTest
	splittests = unixsplittests
	if runtime.GOOS == "windows" {
		splittests = append(splittests, winsplittests...)
	}
	for _, test := range splittests {
		if d, f := filepath.Split(test.path); d != test.dir || f != test.file {
			t.Errorf("Split(%q) = %q, %q, want %q, %q", test.path, d, f, test.dir, test.file)
		}
	}
}

type JoinTest struct {
	elem []string
	path string
}

var jointests = []JoinTest{
	// zero parameters
	{[]string{}, ""},

	// one parameter
	{[]string{""}, ""},
	{[]string{"a"}, "a"},

	// two parameters
	{[]string{"a", "b"}, "a/b"},
	{[]string{"a", ""}, "a"},
	{[]string{"", "b"}, "b"},
	{[]string{"/", "a"}, "/a"},
	{[]string{"/", ""}, "/"},
	{[]string{"a/", "b"}, "a/b"},
	{[]string{"a/", ""}, "a"},
	{[]string{"", ""}, ""},
}

var winjointests = []JoinTest{
	{[]string{`directory`, `file`}, `directory\file`},
	{[]string{`C:\Windows\`, `System32`}, `C:\Windows\System32`},
	{[]string{`C:\Windows\`, ``}, `C:\Windows`},
	{[]string{`C:\`, `Windows`}, `C:\Windows`},
	{[]string{`C:`, `Windows`}, `C:\Windows`},
	{[]string{`\\host\share`, `foo`}, `\\host\share\foo`},
	{[]string{`//host/share`, `foo/bar`}, `\\host\share\foo\bar`},
}

// join takes a []string and passes it to Join.
func join(elem []string, args ...string) string {
	args = elem
	return filepath.Join(args...)
}

func TestJoin(t *testing.T) {
	if runtime.GOOS == "windows" {
		jointests = append(jointests, winjointests...)
	}
	for _, test := range jointests {
		if p := join(test.elem); p != filepath.FromSlash(test.path) {
			t.Errorf("join(%q) = %q, want %q", test.elem, p, test.path)
		}
	}
}

type ExtTest struct {
	path, ext string
}

var exttests = []ExtTest{
	{"path.go", ".go"},
	{"path.pb.go", ".go"},
	{"a.dir/b", ""},
	{"a.dir/b.go", ".go"},
	{"a.dir/", ""},
}

func TestExt(t *testing.T) {
	for _, test := range exttests {
		if x := filepath.Ext(test.path); x != test.ext {
			t.Errorf("Ext(%q) = %q, want %q", test.path, x, test.ext)
		}
	}
}

var basetests = []PathTest{
	{"", "."},
	{".", "."},
	{"/.", "."},
	{"/", "/"},
	{"////", "/"},
	{"x/", "x"},
	{"abc", "abc"},
	{"abc/def", "def"},
	{"a/b/.x", ".x"},
	{"a/b/c.", "c."},
	{"a/b/c.x", "c.x"},
}

var winbasetests = []PathTest{
	{`c:\`, `\`},
	{`c:.`, `.`},
	{`c:\a\b`, `b`},
	{`c:a\b`, `b`},
	{`c:a\b\c`, `c`},
	{`\\host\share\`, `\`},
	{`\\host\share\a`, `a`},
	{`\\host\share\a\b`, `b`},
}

func TestBase(t *testing.T) {
	tests := basetests
	if runtime.GOOS == "windows" {
		// make unix tests work on windows
		for i := range tests {
			tests[i].result = filepath.Clean(tests[i].result)
		}
		// add windows specific tests
		tests = append(tests, winbasetests...)
	}
	for _, test := range tests {
		if s := filepath.Base(test.path); s != test.result {
			t.Errorf("Base(%q) = %q, want %q", test.path, s, test.result)
		}
	}
}

var dirtests = []PathTest{
	{"", "."},
	{".", "."},
	{"/.", "/"},
	{"/", "/"},
	{"////", "/"},
	{"/foo", "/"},
	{"x/", "x"},
	{"abc", "."},
	{"abc/def", "abc"},
	{"a/b/.x", "a/b"},
	{"a/b/c.", "a/b"},
	{"a/b/c.x", "a/b"},
}

var windirtests = []PathTest{
	{`c:\`, `c:\`},
	{`c:.`, `c:.`},
	{`c:\a\b`, `c:\a`},
	{`c:a\b`, `c:a`},
	{`c:a\b\c`, `c:a\b`},
	{`\\host\share\`, `\\host\share\`},
	{`\\host\share\a`, `\\host\share\`},
	{`\\host\share\a\b`, `\\host\share\a`},
}

func TestDir(t *testing.T) {
	tests := dirtests
	if runtime.GOOS == "windows" {
		// make unix tests work on windows
		for i := range tests {
			tests[i].result = filepath.Clean(tests[i].result)
		}
		// add windows specific tests
		tests = append(tests, windirtests...)
	}
	for _, test := range tests {
		if s := filepath.Dir(test.path); s != test.result {
			t.Errorf("Dir(%q) = %q, want %q", test.path, s, test.result)
		}
	}
}

type IsAbsTest struct {
	path  string
	isAbs bool
}

var isabstests = []IsAbsTest{
	{"", false},
	{"/", true},
	{"/usr/bin/gcc", true},
	{"..", false},
	{"/a/../bb", true},
	{".", false},
	{"./", false},
	{"lala", false},
}

var winisabstests = []IsAbsTest{
	{`C:\`, true},
	{`c\`, false},
	{`c::`, false},
	{`c:`, false},
	{`/`, false},
	{`\`, false},
	{`\Windows`, false},
	{`c:a\b`, false},
	{`\\host\share\foo`, true},
	{`//host/share/foo/bar`, true},
}

func TestIsAbs(t *testing.T) {
	var tests []IsAbsTest
	if runtime.GOOS == "windows" {
		tests = append(tests, winisabstests...)
		// All non-windows tests should fail, because they have no volume letter.
		for _, test := range isabstests {
			tests = append(tests, IsAbsTest{test.path, false})
		}
		// All non-windows test should work as intended if prefixed with volume letter.
		for _, test := range isabstests {
			tests = append(tests, IsAbsTest{"c:" + test.path, test.isAbs})
		}
	} else {
		tests = isabstests
	}

	for _, test := range tests {
		if r := filepath.IsAbs(test.path); r != test.isAbs {
			t.Errorf("IsAbs(%q) = %v, want %v", test.path, r, test.isAbs)
		}
	}
}

type EvalSymlinksTest struct {
	// If dest is empty, the path is created; otherwise the dest is symlinked to the path.
	path, dest string
}

var EvalSymlinksTestDirs = []EvalSymlinksTest{
	{"test", ""},
	{"test/dir", ""},
	{"test/dir/link3", "../../"},
	{"test/link1", "../test"},
	{"test/link2", "dir"},
	{"test/linkabs", "/"},
}

var EvalSymlinksTests = []EvalSymlinksTest{
	{"test", "test"},
	{"test/dir", "test/dir"},
	{"test/dir/../..", "."},
	{"test/link1", "test"},
	{"test/link2", "test/dir"},
	{"test/link1/dir", "test/dir"},
	{"test/link2/..", "test"},
	{"test/dir/link3", "."},
	{"test/link2/link3/test", "test"},
	{"test/linkabs", "/"},
}

var EvalSymlinksAbsWindowsTests = []EvalSymlinksTest{
	{`c:\`, `c:\`},
}

// simpleJoin builds a file name from the directory and path.
// It does not use Join because we don't want ".." to be evaluated.
func simpleJoin(dir, path string) string {
	return dir + string(filepath.Separator) + path
}

func TestEvalSymlinks(t *testing.T) {
	if runtime.GOOS == "plan9" {
		t.Skip("Skipping test: symlinks don't exist under Plan 9")
	}

	tmpDir, err := ioutil.TempDir("", "evalsymlink")
	if err != nil {
		t.Fatal("creating temp dir:", err)
	}
	defer os.RemoveAll(tmpDir)

	// /tmp may itself be a symlink! Avoid the confusion, although
	// it means trusting the thing we're testing.
	tmpDir, err = filepath.EvalSymlinks(tmpDir)
	if err != nil {
		t.Fatal("eval symlink for tmp dir:", err)
	}

	// Create the symlink farm using relative paths.
	for _, d := range EvalSymlinksTestDirs {
		var err error
		path := simpleJoin(tmpDir, d.path)
		if d.dest == "" {
			err = os.Mkdir(path, 0755)
		} else {
			if runtime.GOOS != "windows" {
				err = os.Symlink(d.dest, path)
			}
		}
		if err != nil {
			t.Fatal(err)
		}
	}

	var tests []EvalSymlinksTest
	if runtime.GOOS == "windows" {
		for _, d := range EvalSymlinksTests {
			if d.path == d.dest {
				// will test only real files and directories
				tests = append(tests, d)
				// test "canonical" names
				d2 := EvalSymlinksTest{
					path: strings.ToUpper(d.path),
					dest: d.dest,
				}
				tests = append(tests, d2)
			}
		}
	} else {
		tests = EvalSymlinksTests
	}

	// Evaluate the symlink farm.
	for _, d := range tests {
		path := simpleJoin(tmpDir, d.path)
		dest := simpleJoin(tmpDir, d.dest)
		if filepath.IsAbs(d.dest) {
			dest = d.dest
		}
		if p, err := filepath.EvalSymlinks(path); err != nil {
			t.Errorf("EvalSymlinks(%q) error: %v", d.path, err)
		} else if filepath.Clean(p) != filepath.Clean(dest) {
			t.Errorf("Clean(%q)=%q, want %q", path, p, dest)
		}
	}
}

// Test directories relative to temporary directory.
// The tests are run in absTestDirs[0].
var absTestDirs = []string{
	"a",
	"a/b",
	"a/b/c",
}

// Test paths relative to temporary directory. $ expands to the directory.
// The tests are run in absTestDirs[0].
// We create absTestDirs first.
var absTests = []string{
	".",
	"b",
	"../a",
	"../a/b",
	"../a/b/./c/../../.././a",
	"$",
	"$/.",
	"$/a/../a/b",
	"$/a/b/c/../../.././a",
}

func TestAbs(t *testing.T) {
	oldwd, err := os.Getwd()
	if err != nil {
		t.Fatal("Getwd failed: ", err)
	}
	defer os.Chdir(oldwd)

	root, err := ioutil.TempDir("", "TestAbs")
	if err != nil {
		t.Fatal("TempDir failed: ", err)
	}
	defer os.RemoveAll(root)

	wd, err := os.Getwd()
	if err != nil {
		t.Fatal("getwd failed: ", err)
	}
	err = os.Chdir(root)
	if err != nil {
		t.Fatal("chdir failed: ", err)
	}
	defer os.Chdir(wd)

	for _, dir := range absTestDirs {
		err = os.Mkdir(dir, 0777)
		if err != nil {
			t.Fatal("Mkdir failed: ", err)
		}
	}

	err = os.Chdir(absTestDirs[0])
	if err != nil {
		t.Fatal("chdir failed: ", err)
	}

	for _, path := range absTests {
		path = strings.Replace(path, "$", root, -1)
		info, err := os.Stat(path)
		if err != nil {
			t.Errorf("%s: %s", path, err)
			continue
		}

		abspath, err := filepath.Abs(path)
		if err != nil {
			t.Errorf("Abs(%q) error: %v", path, err)
			continue
		}
		absinfo, err := os.Stat(abspath)
		if err != nil || !os.SameFile(absinfo, info) {
			t.Errorf("Abs(%q)=%q, not the same file", path, abspath)
		}
		if !filepath.IsAbs(abspath) {
			t.Errorf("Abs(%q)=%q, not an absolute path", path, abspath)
		}
		if filepath.IsAbs(path) && abspath != filepath.Clean(path) {
			t.Errorf("Abs(%q)=%q, isn't clean", path, abspath)
		}
	}
}

type RelTests struct {
	root, path, want string
}

var reltests = []RelTests{
	{"a/b", "a/b", "."},
	{"a/b/.", "a/b", "."},
	{"a/b", "a/b/.", "."},
	{"./a/b", "a/b", "."},
	{"a/b", "./a/b", "."},
	{"ab/cd", "ab/cde", "../cde"},
	{"ab/cd", "ab/c", "../c"},
	{"a/b", "a/b/c/d", "c/d"},
	{"a/b", "a/b/../c", "../c"},
	{"a/b/../c", "a/b", "../b"},
	{"a/b/c", "a/c/d", "../../c/d"},
	{"a/b", "c/d", "../../c/d"},
	{"a/b/c/d", "a/b", "../.."},
	{"a/b/c/d", "a/b/", "../.."},
	{"a/b/c/d/", "a/b", "../.."},
	{"a/b/c/d/", "a/b/", "../.."},
	{"../../a/b", "../../a/b/c/d", "c/d"},
	{"/a/b", "/a/b", "."},
	{"/a/b/.", "/a/b", "."},
	{"/a/b", "/a/b/.", "."},
	{"/ab/cd", "/ab/cde", "../cde"},
	{"/ab/cd", "/ab/c", "../c"},
	{"/a/b", "/a/b/c/d", "c/d"},
	{"/a/b", "/a/b/../c", "../c"},
	{"/a/b/../c", "/a/b", "../b"},
	{"/a/b/c", "/a/c/d", "../../c/d"},
	{"/a/b", "/c/d", "../../c/d"},
	{"/a/b/c/d", "/a/b", "../.."},
	{"/a/b/c/d", "/a/b/", "../.."},
	{"/a/b/c/d/", "/a/b", "../.."},
	{"/a/b/c/d/", "/a/b/", "../.."},
	{"/../../a/b", "/../../a/b/c/d", "c/d"},
	{".", "a/b", "a/b"},
	{".", "..", ".."},

	// can't do purely lexically
	{"..", ".", "err"},
	{"..", "a", "err"},
	{"../..", "..", "err"},
	{"a", "/a", "err"},
	{"/a", "a", "err"},
}

var winreltests = []RelTests{
	{`C:a\b\c`, `C:a/b/d`, `..\d`},
	{`C:\`, `D:\`, `err`},
	{`C:`, `D:`, `err`},
}

func TestRel(t *testing.T) {
	tests := append([]RelTests{}, reltests...)
	if runtime.GOOS == "windows" {
		for i := range tests {
			tests[i].want = filepath.FromSlash(tests[i].want)
		}
		tests = append(tests, winreltests...)
	}
	for _, test := range tests {
		got, err := filepath.Rel(test.root, test.path)
		if test.want == "err" {
			if err == nil {
				t.Errorf("Rel(%q, %q)=%q, want error", test.root, test.path, got)
			}
			continue
		}
		if err != nil {
			t.Errorf("Rel(%q, %q): want %q, got error: %s", test.root, test.path, test.want, err)
		}
		if got != test.want {
			t.Errorf("Rel(%q, %q)=%q, want %q", test.root, test.path, got, test.want)
		}
	}
}

type VolumeNameTest struct {
	path string
	vol  string
}

var volumenametests = []VolumeNameTest{
	{`c:/foo/bar`, `c:`},
	{`c:`, `c:`},
	{`2:`, ``},
	{``, ``},
	{`\\\host`, ``},
	{`\\\host\`, ``},
	{`\\\host\share`, ``},
	{`\\\host\\share`, ``},
	{`\\host`, ``},
	{`//host`, ``},
	{`\\host\`, ``},
	{`//host/`, ``},
	{`\\host\share`, `\\host\share`},
	{`//host/share`, `//host/share`},
	{`\\host\share\`, `\\host\share`},
	{`//host/share/`, `//host/share`},
	{`\\host\share\foo`, `\\host\share`},
	{`//host/share/foo`, `//host/share`},
	{`\\host\share\\foo\\\bar\\\\baz`, `\\host\share`},
	{`//host/share//foo///bar////baz`, `//host/share`},
	{`\\host\share\foo\..\bar`, `\\host\share`},
	{`//host/share/foo/../bar`, `//host/share`},
}

func TestVolumeName(t *testing.T) {
	if runtime.GOOS != "windows" {
		return
	}
	for _, v := range volumenametests {
		if vol := filepath.VolumeName(v.path); vol != v.vol {
			t.Errorf("VolumeName(%q)=%q, want %q", v.path, vol, v.vol)
		}
	}
}

func TestDriveLetterInEvalSymlinks(t *testing.T) {
	if runtime.GOOS != "windows" {
		return
	}
	wd, _ := os.Getwd()
	if len(wd) < 3 {
		t.Errorf("Current directory path %q is too short", wd)
	}
	lp := strings.ToLower(wd)
	up := strings.ToUpper(wd)
	flp, err := filepath.EvalSymlinks(lp)
	if err != nil {
		t.Fatalf("EvalSymlinks(%q) failed: %q", lp, err)
	}
	fup, err := filepath.EvalSymlinks(up)
	if err != nil {
		t.Fatalf("EvalSymlinks(%q) failed: %q", up, err)
	}
	if flp != fup {
		t.Errorf("Results of EvalSymlinks do not match: %q and %q", flp, fup)
	}
}

func TestBug3486(t *testing.T) { // http://code.google.com/p/go/issues/detail?id=3486
	root, err := filepath.EvalSymlinks(runtime.GOROOT() + "/test")
	if err != nil {
		t.Fatal(err)
	}
	bugs := filepath.Join(root, "bugs")
	ken := filepath.Join(root, "ken")
	seenBugs := false
	seenKen := false
	filepath.Walk(root, func(pth string, info os.FileInfo, err error) error {
		if err != nil {
			t.Fatal(err)
		}

		switch pth {
		case bugs:
			seenBugs = true
			return filepath.SkipDir
		case ken:
			if !seenBugs {
				t.Fatal("filepath.Walk out of order - ken before bugs")
			}
			seenKen = true
		}
		return nil
	})
	if !seenKen {
		t.Fatalf("%q not seen", ken)
	}
}
