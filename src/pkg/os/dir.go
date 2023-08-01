// 	@compatible: 此文件在 v1.8 版本添加

package os

type readdirMode int

const (
	readdirName readdirMode = iota
	readdirDirEntry
	readdirFileInfo
)

// 	@compatible: 此类型在 v1.16 版本添加
//
// testingForceReadDirLstat forces ReadDir to call Lstat, for testing that code path.
// This can be difficult to provoke on some Unix systems otherwise.
var testingForceReadDirLstat bool

// 	@compatible: 此类型在 v1.16 版本添加
//
// A DirEntry is an entry read from a directory
// (using the ReadDir function or a File's ReadDir method).
// type DirEntry = fs.DirEntry

// 	@compatible: 此接口在 v1.16 版本添加
// 	@compatibleNote: 从 io/fs 中, 将 DirEntry 接口移到这里
// 	@implementAt: src/pkg/os/file_compatible.go -> unixDirent{}
//
// A DirEntry is an entry read from a directory
// (using the ReadDir function or a ReadDirFile's ReadDir method).
type DirEntry interface {
	// Name returns the name of the file (or subdirectory) described by the entry.
	// This name is only the final element of the path (the base name), not the entire path.
	// For example, Name would return "hello.go" not "/home/gopher/hello.go".
	Name() string

	// IsDir reports whether the entry describes a directory.
	IsDir() bool

	// Type returns the type bits for the entry.
	// The type bits are a subset of the usual FileMode bits, those returned by the FileMode.Type method.
	Type() FileMode

	// Info returns the FileInfo for the file or subdirectory described by the entry.
	// The returned FileInfo may be from the time of the original directory read
	// or from the time of the call to Info. If the file has been removed or renamed
	// since the directory read, Info may return an error satisfying errors.Is(err, ErrNotExist).
	// If the entry denotes a symbolic link, Info reports the information about the link itself,
	// not the link's target.
	Info() (FileInfo, error)
}
