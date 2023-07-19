// Copyright 2012 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"fmt"
	"os"
	"runtime"
	"strings"

	"internal/cfg"
)

var cmdEnv = &Command{
	// 	@compatible: 此处变更在 v1.9 版本完成, 因为
	// cmdEnv.Run -> runEnv -> envJson -> cmdEnv
	//  ↑                                   |
	//  └───────────────────────────────────┘
	// 出现了循环引用
	//
	// Run:       runEnv,

	// 	@compatible: 此处变更与`Long`中的`-json`部分, 在 v1.9 版本完成
	// UsageLine: "env [var ...]",
	UsageLine: "env [-json] [var ...]",
	Short:     "print Go environment information",
	Long: `
Env prints Go environment information.

By default env prints information as a shell script
(on Windows, a batch file).  If one or more variable
names is given as arguments,  env prints the value of
each named variable on its own line.

The -json flag prints the environment in JSON format
instead of as a shell script.
	`,
}

type envVar struct {
	name, value string
}

func mkEnv() []envVar {
	var b builder
	b.init()

	env := []envVar{
		{"GOARCH", goarch},
		{"GOBIN", gobin},
		{"GOCHAR", archChar},
		{"GOEXE", exeSuffix},
		{"GOHOSTARCH", runtime.GOARCH},
		{"GOHOSTOS", runtime.GOOS},
		{"GOOS", goos},
		{"GOPATH", os.Getenv("GOPATH")},
		{"GORACE", os.Getenv("GORACE")},
		{"GOROOT", goroot},
		{"GOTOOLDIR", toolDir},

		// disable escape codes in clang errors
		{"TERM", "dumb"},
	}

	if goos != "plan9" {
		cmd := b.gccCmd(".")
		env = append(env, envVar{"CC", cmd[0]})
		env = append(env, envVar{"GOGCCFLAGS", strings.Join(cmd[3:], " ")})
		cmd = b.gxxCmd(".")
		env = append(env, envVar{"CXX", cmd[0]})
	}

	if buildContext.CgoEnabled {
		env = append(env, envVar{"CGO_ENABLED", "1"})
	} else {
		env = append(env, envVar{"CGO_ENABLED", "0"})
	}

	return env
}

func findEnv(env []envVar, name string) string {
	for _, e := range env {
		if e.name == name {
			return e.value
		}
	}
	return ""
}

func runEnv(cmd *Command, args []string) {
	env := mkEnv()
	if len(args) > 0 {
		if *envJson {
			// 	@compatible: 此 if 块在 v1.9 版本添加, 为了实现 -json 参数的能力
			var es []cfg.EnvVar
			for _, name := range args {
				e := cfg.EnvVar{Name: name, Value: findEnv(env, name)}
				es = append(es, e)
			}
			printEnvAsJSON(es)
		} else {
			for _, name := range args {
				fmt.Printf("%s\n", findEnv(env, name))
			}
		}
		return
	}

	// 	@compatible: 此 if 块在 v1.9 版本添加, 为了实现 -json 参数的能力
	if *envJson {
		var cfgEnv []cfg.EnvVar
		for _, e := range env {
			cfgEnv = append(cfgEnv, cfg.EnvVar{
				Name: e.name,
				Value: e.value,
			})
		}
		printEnvAsJSON(cfgEnv)
		return
	}

	switch runtime.GOOS {
	default:
		for _, e := range env {
			fmt.Printf("%s=\"%s\"\n", e.name, e.value)
		}
	case "windows":
		for _, e := range env {
			fmt.Printf("set %s=%s\n", e.name, e.value)
		}
	}
}
