package build

import (
	"bytes"
	"fmt"
	"strings"
	"unicode"
)

// caller: 貌似没有显式调用的地方.
//
// MatchFile reports whether the file with the given name in the given directory
// matches the context and would be included in a Package created by ImportDir
// of that directory.
//
// MatchFile considers the name of the file and may use ctxt.OpenFile to
// read some or all of the file's content.
func (ctxt *Context) MatchFile(dir, name string) (match bool, err error) {
	match, _, _, err = ctxt.matchFile(dir, name, false, nil)
	return
}

// matchFile 判断目标文件是否参与 go build 构建.
//
// 判断条件是文件名中的"CPU架构/OS系统", 以及文件内容中的'// +build'语句.
//
// 	@param dir: import 语句中的某个 package 所在的目录(绝对路径)
// 	@param name: dir 目录下的某个文件的名称(一定是文件, 不可能是子目录, 且包含后缀名)
//
// 	@return match: 是否参与构建
// 	@return data: 文件开头部分的内容, 一般是"[注释] <package> [注释] [import]"
// 	@return filename: 目标文件的绝对路径
//
// caller:
// 	1. Context.Import()
// 	2. Context.MatchFile() 貌似没有显式调用的地方, 所以只用关注第1个主调函数即可.
//
// matchFile determines whether the file with the given name in the given directory
// should be included in the package being constructed.
// It returns the data read from the file.
// If returnImports is true and name denotes a Go program, matchFile reads
// until the end of the imports (and returns that data) even though it only
// considers text until the first non-comment.
// If allTags is non-nil, matchFile records any encountered build tag
// by setting allTags[tag] = true.
func (ctxt *Context) matchFile(
	dir, name string, returnImports bool, allTags map[string]bool,
) (match bool, data []byte, filename string, err error) {
	// 1. 如果文件名以下划线或点号开头, 则不参与构建.
	if strings.HasPrefix(name, "_") || strings.HasPrefix(name, ".") {
		return
	}

	i := strings.LastIndex(name, ".")
	if i < 0 {
		i = len(name)
	}
	// 如果文件名中不包含点号(即 i = -1), 则 ext 将为空
	ext := name[i:]

	// 2. 如果文件名中包含不兼容的CPU架构/OS系统, 则不参与构建.
	if !ctxt.goodOSArchFile(name, allTags) && !ctxt.UseAllFiles {
		return
	}

	// 3. 非 .go, .c 等类型的文件, 不参与构建.
	switch ext {
	case ".go", ".c", ".cc", ".cxx", ".cpp", ".s", ".h",
		".hh", ".hpp", ".hxx", ".S", ".swig", ".swigcxx":
		// tentatively okay - read to make sure
	case ".syso":
		// binary, no reading
		match = true
		return
	default:
		// skip
		return
	}

	// 4. 打开并读取文件内容, '// +build' 语句中不符合 -tag 选项的, 不参与构建
	filename = ctxt.joinPath(dir, name)
	f, err := ctxt.openFile(filename)
	if err != nil {
		return
	}

	if strings.HasSuffix(filename, ".go") {
		data, err = readImports(f, false)
	} else {
		data, err = readComments(f)
	}
	f.Close()
	if err != nil {
		err = fmt.Errorf("read %s: %v", filename, err)
		return
	}

	// Look for +build comments to accept or reject the file.
	if !ctxt.shouldBuild(data, allTags) && !ctxt.UseAllFiles {
		return
	}

	match = true
	return
}

var slashslash = []byte("//")

// shouldBuild 从文件内容中读取'// +build'语句, 不符合 -tag 选项的, 不参与构建
//
// 	@param content: .go, .c 等源码文件开头部分的内容, 一般是"[注释] <package> [注释] [import]"
//
// caller:
// 	1. Context.matchFile() 只有这一处
//
// shouldBuild reports whether it is okay to use this file,
// The rule is that in the file's leading run of // comments and blank lines,
// which must be followed by a blank line
// (to avoid including a Go package clause doc comment),
// lines beginning with '// +build' are taken as build directives.
//
// The file is accepted only if each such line lists something matching the file.
// For example:
//
//	// +build windows linux
//
// marks the file as applicable only on Windows and Linux.
//
func (ctxt *Context) shouldBuild(content []byte, allTags map[string]bool) bool {
	// Pass 1. Identify leading run of // comments and blank lines,
	// which must be followed by a blank line.
	end := 0
	p := content
	for len(p) > 0 {
		line := p
		if i := bytes.IndexByte(line, '\n'); i >= 0 {
			line, p = line[:i], p[i+1:]
		} else {
			p = p[len(p):]
		}
		line = bytes.TrimSpace(line)
		if len(line) == 0 { // Blank line
			end = len(content) - len(p)
			continue
		}
		if !bytes.HasPrefix(line, slashslash) { // Not comment line
			break
		}
	}
	content = content[:end]

	// Pass 2.  Process each line in the run.
	p = content
	allok := true
	for len(p) > 0 {
		line := p
		if i := bytes.IndexByte(line, '\n'); i >= 0 {
			line, p = line[:i], p[i+1:]
		} else {
			p = p[len(p):]
		}
		line = bytes.TrimSpace(line)
		if bytes.HasPrefix(line, slashslash) {
			line = bytes.TrimSpace(line[len(slashslash):])
			if len(line) > 0 && line[0] == '+' {
				// Looks like a comment +line.
				f := strings.Fields(string(line))
				if f[0] == "+build" {
					ok := false
					for _, tok := range f[1:] {
						if ctxt.match(tok, allTags) {
							ok = true
						}
					}
					if !ok {
						allok = false
					}
				}
			}
		}
	}

	return allok
}

// caller:
// 	1. Context.shouldBuild()
// 	2. Context.match() 递归调用
//
// match returns true if the name is one of:
//
//	$GOOS
//	$GOARCH
//	cgo (if cgo is enabled)
//	!cgo (if cgo is disabled)
//	ctxt.Compiler
//	!ctxt.Compiler
//	tag (if tag is listed in ctxt.BuildTags or ctxt.ReleaseTags)
//	!tag (if tag is not listed in ctxt.BuildTags or ctxt.ReleaseTags)
//	a comma-separated list of any of these
//
func (ctxt *Context) match(name string, allTags map[string]bool) bool {
	if name == "" {
		if allTags != nil {
			allTags[name] = true
		}
		return false
	}
	if i := strings.Index(name, ","); i >= 0 {
		// comma-separated list
		ok1 := ctxt.match(name[:i], allTags)
		ok2 := ctxt.match(name[i+1:], allTags)
		return ok1 && ok2
	}
	if strings.HasPrefix(name, "!!") { // bad syntax, reject always
		return false
	}
	if strings.HasPrefix(name, "!") { // negation
		return len(name) > 1 && !ctxt.match(name[1:], allTags)
	}

	if allTags != nil {
		allTags[name] = true
	}

	// Tags must be letters, digits, underscores or dots.
	// Unlike in Go identifiers, all digits are fine (e.g., "386").
	for _, c := range name {
		if !unicode.IsLetter(c) && !unicode.IsDigit(c) && c != '_' && c != '.' {
			return false
		}
	}

	// special tags
	if ctxt.CgoEnabled && name == "cgo" {
		return true
	}
	if name == ctxt.GOOS || name == ctxt.GOARCH || name == ctxt.Compiler {
		return true
	}

	// other tags
	for _, tag := range ctxt.BuildTags {
		if tag == name {
			return true
		}
	}
	for _, tag := range ctxt.ReleaseTags {
		if tag == name {
			return true
		}
	}

	return false
}

// 判断目标文件是否需要编译, 如果目标文件中包含已知的 os/arch 名称(如 sys_plan9_arm64.go),
// 但与当前所在主机不符, 则不编译.
//
// 	@name: 待编译的文件名(无路径, 有后缀)
//
// goodOSArchFile returns false if the name contains a $GOOS or $GOARCH
// suffix which does not match the current system.
// The recognized name formats are:
//
//     name_$(GOOS).*
//     name_$(GOARCH).*
//     name_$(GOOS)_$(GOARCH).*
//     name_$(GOOS)_test.*
//     name_$(GOARCH)_test.*
//     name_$(GOOS)_$(GOARCH)_test.*
//
func (ctxt *Context) goodOSArchFile(name string, allTags map[string]bool) bool {
	// 移除后缀名
	if dot := strings.Index(name, "."); dot != -1 {
		name = name[:dot]
	}

	////////////////////////////////////////////////////////////////////////////
	// 	@compatible: src/pkg/go/build/syslist.go -> [goosList, goarchList] 中,
	// 扩大了 os/arch 范围, 但源文件中有可能出现 linux.go, amd64.go 这种,
	// 包含 os/arch 名称, 但又明显不是用来做多系统兼容的,
	// 我们要的是 linux_arm64.go 这种, 必须是用下划线分割 os/arch 的才算.
	//
	// Before Go 1.4, a file called "linux.go" would be equivalent to having a
	// build tag "linux" in that file. For Go 1.4 and beyond, we require this
	// auto-tagging to apply only to files with a non-empty prefix, so
	// "foo_linux.go" is tagged but "linux.go" is not. This allows new operating
	// systems, such as android, to arrive without breaking existing code with
	// innocuous source code in "android.go". The easiest fix: cut everything
	// in the name before the initial _.
	i := strings.Index(name, "_")
	if i < 0 {
		return true
	}
	name = name[i:] // ignore everything before first _
	////////////////////////////////////////////////////////////////////////////

	// _test.go 文件也有可能包含 os/arch 信息
	l := strings.Split(name, "_")
	if n := len(l); n > 0 && l[n-1] == "test" {
		l = l[:n-1]
	}
	n := len(l)
	if n >= 2 && knownOS[l[n-2]] && knownArch[l[n-1]] {
		if allTags != nil {
			allTags[l[n-2]] = true
			allTags[l[n-1]] = true
		}
		return l[n-2] == ctxt.GOOS && l[n-1] == ctxt.GOARCH
	}
	if n >= 1 && knownOS[l[n-1]] {
		if allTags != nil {
			allTags[l[n-1]] = true
		}
		return l[n-1] == ctxt.GOOS
	}
	if n >= 1 && knownArch[l[n-1]] {
		if allTags != nil {
			allTags[l[n-1]] = true
		}
		return l[n-1] == ctxt.GOARCH
	}
	return true
}

var knownOS = make(map[string]bool)
var knownArch = make(map[string]bool)

func init() {
	for _, v := range strings.Fields(goosList) {
		knownOS[v] = true
	}
	for _, v := range strings.Fields(goarchList) {
		knownArch[v] = true
	}
}
