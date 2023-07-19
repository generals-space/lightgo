package main

import (
	"encoding/json"
	"os"
	"log"
	"internal/cfg"
)

// 	@compatible: 此函数在 v1.9 版本添加
// 	@note: 新增此函数的原因是为了解除循环引用, 具体可见
// 	src/cmd/go/env.go -> cmdEnv.Run
//
func init() {
	// 	@note: CmdEnv -> cmdEnv
	// CmdEnv.Run = runEnv // break init cycle
	cmdEnv.Run = runEnv // break init cycle
}

// 	@compatible: 此参数(go env -json)在 v1.9 版本添加
// var envJson = CmdEnv.Flag.Bool("json", false, "")
var envJson = cmdEnv.Flag.Bool("json", false, "")

// 	@compatible: 此方法在 v1.9 版本添加
func printEnvAsJSON(env []cfg.EnvVar) {
	m := make(map[string]string)
	for _, e := range env {
		if e.Name == "TERM" {
			continue
		}
		m[e.Name] = e.Value
	}
	enc := json.NewEncoder(os.Stdout)
	enc.SetIndent("", "\t")
	if err := enc.Encode(m); err != nil {
		// base.Fatalf("%s", err)
		log.Fatalf("%s", err)
	}
}
