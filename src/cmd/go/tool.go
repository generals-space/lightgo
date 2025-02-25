// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"fmt"
	"go/build"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"sort"
	"strings"
)

var cmdTool = &Command{
	Run:       runTool,
	UsageLine: "tool [-n] command [args...]",
	Short:     "run specified go tool",
	Long: `
Tool runs the go tool command identified by the arguments.
With no arguments it prints the list of known tools.

The -n flag causes tool to print the command that would be
executed but not execute it.

For more about each tool command, see 'go tool command -h'.
`,
}

var (
	toolGOOS      = runtime.GOOS
	toolGOARCH    = runtime.GOARCH
	toolIsWindows = toolGOOS == "windows"
	toolDir       = build.ToolDir

	toolN bool
)

func init() {
	cmdTool.Flag.BoolVar(&toolN, "n", false, "")
}

const toolWindowsExtension = ".exe"

// tool 获取名为 toolName 的编译工具所在的路径并返回, 如 6g, 6l 等.
//
// 	@param toolName: 编译工具名称, 如 6g, 6l 等
//
// caller:
// 	1. src/cmd/go/build.go -> gcToolchain.gc() 由 go build 命令调用的 6g 进行编译时,
// 	通过调用该函数先获取工具的绝对路径, 然后由主调函数再次调用.
func tool(toolName string) string {
	toolPath := filepath.Join(toolDir, toolName)
	if toolIsWindows && toolName != "pprof" {
		toolPath += toolWindowsExtension
	}
	// Give a nice message if there is no tool with that name.
	if _, err := os.Stat(toolPath); err != nil {
		if isInGoToolsRepo(toolName) {
			fmt.Fprintf(os.Stderr, "go tool: no such tool %q; to install:\n\tgo get code.google.com/p/go.tools/cmd/%s\n", toolName, toolName)
		} else {
			fmt.Fprintf(os.Stderr, "go tool: no such tool %q\n", toolName)
		}
		setExitStatus(3)
		exit()
	}
	return toolPath
}

func isInGoToolsRepo(toolName string) bool {
	switch toolName {
	case "cover", "vet":
		return true
	}
	return false
}

func runTool(cmd *Command, args []string) {
	if len(args) == 0 {
		listTools()
		return
	}
	toolName := args[0]
	// The tool name must be lower-case letters, numbers or underscores.
	for _, c := range toolName {
		switch {
		case 'a' <= c && c <= 'z', '0' <= c && c <= '9', c == '_':
		default:
			fmt.Fprintf(os.Stderr, "go tool: bad tool name %q\n", toolName)
			setExitStatus(2)
			return
		}
	}
	toolPath := tool(toolName)
	if toolPath == "" {
		return
	}
	if toolIsWindows && toolName == "pprof" {
		args = append([]string{"perl", toolPath}, args[1:]...)
		var err error
		toolPath, err = exec.LookPath("perl")
		if err != nil {
			fmt.Fprintf(os.Stderr, "go tool: perl not found\n")
			setExitStatus(3)
			return
		}
	}
	if toolN {
		fmt.Printf("%s %s\n", toolPath, strings.Join(args[1:], " "))
		return
	}
	toolCmd := &exec.Cmd{
		Path:   toolPath,
		Args:   args,
		Stdin:  os.Stdin,
		Stdout: os.Stdout,
		Stderr: os.Stderr,
	}
	err := toolCmd.Run()
	if err != nil {
		// Only print about the exit status if the command
		// didn't even run (not an ExitError) or it didn't exit cleanly
		// or we're printing command lines too (-x mode).
		// Assume if command exited cleanly (even with non-zero status)
		// it printed any messages it wanted to print.
		if e, ok := err.(*exec.ExitError); !ok || !e.Exited() || buildX {
			fmt.Fprintf(os.Stderr, "go tool %s: %s\n", toolName, err)
		}
		setExitStatus(1)
		return
	}
}

// listTools prints a list of the available tools in the tools directory.
func listTools() {
	f, err := os.Open(toolDir)
	if err != nil {
		fmt.Fprintf(os.Stderr, "go tool: no tool directory: %s\n", err)
		setExitStatus(2)
		return
	}
	defer f.Close()
	names, err := f.Readdirnames(-1)
	if err != nil {
		fmt.Fprintf(os.Stderr, "go tool: can't read directory: %s\n", err)
		setExitStatus(2)
		return
	}

	sort.Strings(names)
	for _, name := range names {
		// Unify presentation by going to lower case.
		name = strings.ToLower(name)
		// If it's windows, don't show the .exe suffix.
		if toolIsWindows && strings.HasSuffix(name, toolWindowsExtension) {
			name = name[:len(name)-len(toolWindowsExtension)]
		}
		fmt.Println(name)
	}
}
