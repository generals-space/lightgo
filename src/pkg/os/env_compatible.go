package os

import "syscall"

// 	@compatible: 该函数在 v1.4 版本初次添加
//
// Unsetenv unsets a single environment variable.
func Unsetenv(key string) error {
	return syscall.Unsetenv(key)
}

// 	@compatible: 该函数在 v1.5 版本初次添加
//
// 	@note: ...跟 Getenv() 是相同的功能, 重复了吧?
//
// LookupEnv retrieves the value of the environment variable named by the key.
// If the variable is present in the environment the value (which may be empty)
// is returned and the boolean is true.
// Otherwise the returned value will be empty and the boolean will be false.
func LookupEnv(key string) (string, bool) {
	return syscall.Getenv(key)
}
