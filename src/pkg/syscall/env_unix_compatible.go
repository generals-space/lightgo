package syscall

// 暂时不考虑 cgo 的场景.
// func unsetenv_c(k string)

// 	@compatible: 该函数在 v1.4 版本初次添加
//
func Unsetenv(key string) error {
	envOnce.Do(copyenv)

	envLock.Lock()
	defer envLock.Unlock()

	if i, ok := env[key]; ok {
		envs[i] = ""
		delete(env, key)
	}
	// 暂时不考虑 cgo 的场景.
	// unsetenv_c(key)
	return nil
}
