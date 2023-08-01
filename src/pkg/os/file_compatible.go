package os

import (
	"internal/errors"
	"io"
	"syscall"
)

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此方法在在 v1.12 版本添加
//
// SyscallConn returns a raw file.
// This implements the syscall.Conn interface.
func (f *File) SyscallConn() (syscall.RawConn, error) {
	if err := f.checkValid("SyscallConn"); err != nil {
		return nil, err
	}
	return newRawConn(f)
}

// 	@compatible: 此方法在 v1.12 版本添加
//
// checkValid checks whether f is valid for use.
// If not, it returns an appropriate error, perhaps incorporating the operation name op.
func (f *File) checkValid(op string) error {
	if f == nil {
		return ErrInvalid
	}
	return nil
}

// 	@compatible: 此函数在 v1.12 版本添加
// 	@compatibleNote: 移除了平台相关的代码, 只保留 linux 的.
//
// UserHomeDir returns the current user's home directory.
//
// On Unix, including macOS, it returns the $HOME environment variable.
// On Windows, it returns %USERPROFILE%.
// On Plan 9, it returns the $home environment variable.
func UserHomeDir() (string, error) {
	env, enverr := "HOME", "$HOME"

	if v := Getenv(env); v != "" {
		return v, nil
	}
	return "", errors.New(enverr + " is not defined")
}

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此函数在在 v1.16 版本添加
//
// ReadFile reads the named file and returns the contents.
// A successful call returns err == nil, not err == EOF.
// Because ReadFile reads the whole file, it does not treat an EOF from Read
// as an error to be reported.
func ReadFile(name string) ([]byte, error) {
	f, err := Open(name)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var size int
	if info, err := f.Stat(); err == nil {
		size64 := info.Size()
		if int64(int(size64)) == size64 {
			size = int(size64)
		}
	}
	size++ // one byte for final read at EOF

	// If a file claims a small size, read at least 512 bytes.
	// In particular, files in Linux's /proc claim size 0 but
	// then do not work right if read in small pieces,
	// so an initial read of 1 byte would not work correctly.
	if size < 512 {
		size = 512
	}

	data := make([]byte, 0, size)
	for {
		if len(data) >= cap(data) {
			d := append(data[:cap(data)], 0)
			data = d[:len(data)]
		}
		n, err := f.Read(data[len(data):cap(data)])
		data = data[:len(data)+n]
		if err != nil {
			if err == io.EOF {
				err = nil
			}
			return data, err
		}
	}
}

// 	@compatible: 此函数在在 v1.16 版本添加
//
// WriteFile writes data to the named file, creating it if necessary.
// If the file does not exist, WriteFile creates it with permissions perm (before umask);
// otherwise WriteFile truncates it before writing, without changing permissions.
func WriteFile(name string, data []byte, perm FileMode) error {
	f, err := OpenFile(name, O_WRONLY|O_CREATE|O_TRUNC, perm)
	if err != nil {
		return err
	}
	_, err = f.Write(data)
	if err1 := f.Close(); err1 != nil && err == nil {
		err = err1
	}
	return err
}
