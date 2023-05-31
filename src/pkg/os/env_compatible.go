package os

import "syscall"

// 	@compatible: 该函数在 v1.4 版本初次添加
//
// Unsetenv unsets a single environment variable.
func Unsetenv(key string) error {
	return syscall.Unsetenv(key)
}
