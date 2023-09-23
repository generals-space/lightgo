package build

import (
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
)

// joinPath calls ctxt.JoinPath (if not nil) or else filepath.Join.
func (ctxt *Context) joinPath(elem ...string) string {
	if f := ctxt.JoinPath; f != nil {
		return f(elem...)
	}
	return filepath.Join(elem...)
}

// splitPathList calls ctxt.SplitPathList (if not nil) or else filepath.SplitList.
func (ctxt *Context) splitPathList(s string) []string {
	if f := ctxt.SplitPathList; f != nil {
		return f(s)
	}
	return filepath.SplitList(s)
}

// isAbsPath calls ctxt.IsAbsSPath (if not nil) or else filepath.IsAbs.
func (ctxt *Context) isAbsPath(path string) bool {
	if f := ctxt.IsAbsPath; f != nil {
		return f(path)
	}
	return filepath.IsAbs(path)
}

// isDir 判断目标 path 是否为目录类型
//
// isDir calls ctxt.IsDir (if not nil) or else uses os.Stat.
func (ctxt *Context) isDir(path string) bool {
	if f := ctxt.IsDir; f != nil {
		return f(path)
	}
	fi, err := os.Stat(path)
	return err == nil && fi.IsDir()
}

// hasSubdir calls ctxt.HasSubdir (if not nil) or else uses
// the local file system to answer the question.
func (ctxt *Context) hasSubdir(root, dir string) (rel string, ok bool) {
	if f := ctxt.HasSubdir; f != nil {
		return f(root, dir)
	}

	// Try using paths we received.
	if rel, ok = hasSubdir(root, dir); ok {
		return
	}

	// Try expanding symlinks and comparing
	// expanded against unexpanded and
	// expanded against expanded.
	rootSym, _ := filepath.EvalSymlinks(root)
	dirSym, _ := filepath.EvalSymlinks(dir)

	if rel, ok = hasSubdir(rootSym, dir); ok {
		return
	}
	if rel, ok = hasSubdir(root, dirSym); ok {
		return
	}
	return hasSubdir(rootSym, dirSym)
}

func hasSubdir(root, dir string) (rel string, ok bool) {
	const sep = string(filepath.Separator)
	root = filepath.Clean(root)
	if !strings.HasSuffix(root, sep) {
		root += sep
	}
	dir = filepath.Clean(dir)
	if !strings.HasPrefix(dir, root) {
		return "", false
	}
	return filepath.ToSlash(dir[len(root):]), true
}

// readDir 读取指定目录中的所有目录和文件（不包括子目录下的文件）并返回.
//
// 	@param path: 必须是一个目录路径, 否则会出错.
// 
// readDir calls ctxt.ReadDir (if not nil) or else ioutil.ReadDir.
func (ctxt *Context) readDir(path string) ([]os.FileInfo, error) {
	if f := ctxt.ReadDir; f != nil {
		return f(path)
	}
	return ioutil.ReadDir(path)
}

// openFile calls ctxt.OpenFile (if not nil) or else os.Open.
func (ctxt *Context) openFile(path string) (io.ReadCloser, error) {
	if fn := ctxt.OpenFile; fn != nil {
		return fn(path)
	}

	f, err := os.Open(path)
	if err != nil {
		return nil, err // nil interface
	}
	return f, nil
}

// isFile 判断目标 path 是否为文件类型
//
// isFile determines whether path is a file by trying to open it.
// It reuses openFile instead of adding another function to the
// list in Context.
func (ctxt *Context) isFile(path string) bool {
	f, err := ctxt.openFile(path)
	if err != nil {
		return false
	}
	f.Close()
	return true
}
