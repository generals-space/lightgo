// Copyright 2010 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build darwin dragonfly freebsd linux netbsd openbsd

// Unix environment variables.

package syscall

import "sync"

var (
	// envOnce guards initialization by copyenv, which populates env.
	envOnce sync.Once

	// envLock guards env and envs.
	envLock sync.RWMutex

	// env 是 envs 数组中所有环境变量的 map 索引.
	// 该 map 中的 value 是其 key 在 envs 数组中的索引值.
	// 以下面的 envs 注释为例, env["key01"] 为 0, env["key02"] 为 1.
	//
	// env maps from an environment variable to its first occurrence in envs.
	env map[string]int

	// 实际值为 src/pkg/runtime/env_posix.c -> syscall·envs
	// 其中的成员为 ["key01=val01", "key02=val02", ...]
	//
	// 在 src/pkg/runtime/runtime.c -> runtime·goenvs_unix() 中完成初始化,
	// 加载程序所在终端中的所有环境变量.
	//
	// envs is provided by the runtime.
	// elements are expected to be of the form "key=value".
	envs []string
)

// 在 src/pkg/runtime/env_posix.c -> syscall·setenv_c() 中定义
//
// setenv_c is provided by the runtime, but is a no-op if cgo isn't loaded.
func setenv_c(k, v string)

// 给 envs 数组中的所有环境变量, 建立 map 索引(每次调用都要重新生成).
func copyenv() {
	env = make(map[string]int)
	for i, s := range envs {
		for j := 0; j < len(s); j++ {
			if s[j] == '=' {
				key := s[:j]
				if _, ok := env[key]; !ok {
					env[key] = i
				}
				break
			}
		}
	}
}

// Getenv 直接从 envs 数组中寻找目标 key, 而不是从宿主机的实际环境变量中查找.
func Getenv(key string) (value string, found bool) {
	envOnce.Do(copyenv)
	if len(key) == 0 {
		return "", false
	}

	envLock.RLock()
	defer envLock.RUnlock()
	// 先从 map 中找到目标 key, i 是该 key 在 envs 中的索引.
	i, ok := env[key]
	if !ok {
		return "", false
	}
	// 根据索引在 envs 数组中找到对应成员.
	s := envs[i]
	for i := 0; i < len(s); i++ {
		if s[i] == '=' {
			return s[i+1:], true
		}
	}
	return "", false
}

// Setenv 需要注意, Setenv 只是将 kv 保存在 envs 数组, 并没有真正写入宿主机的环境变量.
// 所以, 在程序运行期间调用的 Setenv 其实只是在内存中保存了一个数组, 
// Getenv() 也只是从这个数组中查找.
//
// envs 在 src/pkg/runtime/runtime.c -> runtime·goenvs_unix() 中初始化,
// 在程序初始启动时加载所在终端的所有环境变量.
//
// 另外, 在调用 exec, 或是 fork 这种新建子线程时, 要将 envs 传给子线程, 见 Environ() 函数
//
func Setenv(key, value string) error {
	envOnce.Do(copyenv)
	if len(key) == 0 {
		return EINVAL
	}
	// 检测 key, value 中是否存在非法字符: '=' 或是 \0 
	for i := 0; i < len(key); i++ {
		if key[i] == '=' || key[i] == 0 {
			return EINVAL
		}
	}
	for i := 0; i < len(value); i++ {
		if value[i] == 0 {
			return EINVAL
		}
	}

	envLock.Lock()
	defer envLock.Unlock()

	i, ok := env[key]
	kv := key + "=" + value
	if ok {
		envs[i] = kv
	} else {
		i = len(envs)
		envs = append(envs, kv)
	}
	env[key] = i
	// 实际上, 只要没有开启 cgo, 该函数根本没啥作用.
	setenv_c(key, value)
	return nil
}

func Clearenv() {
	// prevent copyenv in Getenv/Setenv
	envOnce.Do(copyenv)

	envLock.Lock()
	defer envLock.Unlock()

	env = make(map[string]int)
	envs = []string{}
	// TODO(bradfitz): pass through to C
}

// 	@return: ["key01=val01", "key02=val02", ...]
//
// caller:
// 	1. src/pkg/os/exec_posix.go -> startProcess()
// 	在调用 exec, 或是 fork 这种新建子线程时, 将全部 envs 传给子线程
func Environ() []string {
	envOnce.Do(copyenv)
	envLock.RLock()
	defer envLock.RUnlock()
	a := make([]string, len(envs))
	copy(a, envs)
	return a
}
