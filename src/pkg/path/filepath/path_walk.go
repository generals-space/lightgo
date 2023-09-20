package filepath

import (
	"errors"
	"os"
	"sort"
)

// SkipDir is used as a return value from WalkFuncs to indicate that
// the directory named in the call is to be skipped. It is not returned
// as an error by any function.
var SkipDir = errors.New("skip this directory")

// WalkFunc is the type of the function called for each file or directory
// visited by Walk. The path argument contains the argument to Walk as a
// prefix; that is, if Walk is called with "dir", which is a directory
// containing the file "a", the walk function will be called with argument
// "dir/a". The info argument is the os.FileInfo for the named path.
//
// If there was a problem walking to the file or directory named by path, the
// incoming error will describe the problem and the function can decide how
// to handle that error (and Walk will not descend into that directory). If
// an error is returned, processing stops. The sole exception is that if path
// is a directory and the function returns the special value SkipDir, the
// contents of the directory are skipped and processing continues as usual on
// the next file.
type WalkFunc func(path string, info os.FileInfo, err error) error

// Walk 遍历目标目录 root 下的所有文件与子目录, 对每个成员执行目标函数 walkFn.
//
// 	@alg: 递归遍历(深度优先)
// 
// walk recursively descends path, calling w.
func walk(path string, info os.FileInfo, walkFn WalkFunc) error {
	err := walkFn(path, info, nil)
	if err != nil {
		if info.IsDir() && err == SkipDir {
			return nil
		}
		return err
	}

	// 如果当前对象是"文件", 则直接返回
	if !info.IsDir() {
		return nil
	}

	// 如果当前对象是"目录", 则获取该目录下的子级内容.
	list, err := readDir(path)
	if err != nil {
		return walkFn(path, info, err)
	}

	for _, fileInfo := range list {
		err = walk(Join(path, fileInfo.Name()), fileInfo, walkFn)
		if err != nil {
			if !fileInfo.IsDir() || err != SkipDir {
				return err
			}
		}
	}
	return nil
}

// Walk 遍历目标目录 root 下的所有文件与子目录, 对每个成员执行目标函数 walkFn.
//
// 	@alg: 递归遍历(深度优先)
//
// Walk walks the file tree rooted at root, calling walkFn for each file or
// directory in the tree, including root.
// All errors that arise visiting files and directories are filtered by walkFn.
// The files are walked in lexical order, which makes the output deterministic
// but means that for very large directories Walk can be inefficient.
// Walk does not follow symbolic links.
func Walk(root string, walkFn WalkFunc) error {
	info, err := os.Lstat(root)
	if err != nil {
		// 即使有 err, 也要对 root 调用一次 walkFn(), 并把 err 传入.
		return walkFn(root, nil, err)
	}
	return walk(root, info, walkFn)
}

// readDir 获取目标目录下所有子级内容并返回(按照文件名排序)
//
// 	@param dirname: 由主调函数保证该字符串一定是"目录"类型的路径.
//
// caller:
// 	1. walk()
//
// readDir reads the directory named by dirname and returns
// a sorted list of directory entries.
// Copied from io/ioutil to avoid the circular import.
func readDir(dirname string) ([]os.FileInfo, error) {
	f, err := os.Open(dirname)
	if err != nil {
		return nil, err
	}
	list, err := f.Readdir(-1)
	f.Close()
	if err != nil {
		return nil, err
	}
	sort.Sort(byName(list))
	return list, nil
}

// byName implements sort.Interface.
type byName []os.FileInfo

func (f byName) Len() int           { return len(f) }
func (f byName) Less(i, j int) bool { return f[i].Name() < f[j].Name() }
func (f byName) Swap(i, j int)      { f[i], f[j] = f[j], f[i] }
