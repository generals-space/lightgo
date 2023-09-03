// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"bufio"
	"encoding/json"
	"io"
	"os"
	"strings"
	"text/template"
)

func init() {
	cmdList.Run = runList // break init cycle
	cmdList.Flag.Var(buildCompiler{}, "compiler", "")
	cmdList.Flag.Var((*stringsFlag)(&buildContext.BuildTags), "tags", "")
}

var listE = cmdList.Flag.Bool("e", false, "")
var listFmt = cmdList.Flag.String("f", "{{.ImportPath}}", "")
var listJson = cmdList.Flag.Bool("json", false, "")
var listRace = cmdList.Flag.Bool("race", false, "")
var nl = []byte{'\n'}

func runList(cmd *Command, args []string) {
	out := newTrackingWriter(os.Stdout)
	defer out.w.Flush()

	if *listRace {
		buildRace = true
	}

	var do func(*Package)
	if *listJson {
		do = func(p *Package) {
			b, err := json.MarshalIndent(p, "", "\t")
			if err != nil {
				out.Flush()
				fatalf("%s", err)
			}
			out.Write(b)
			out.Write(nl)
		}
	} else {
		tmpl, err := template.New("main").Funcs(template.FuncMap{"join": strings.Join}).Parse(*listFmt)
		if err != nil {
			fatalf("%s", err)
		}
		do = func(p *Package) {
			if err := tmpl.Execute(out, p); err != nil {
				out.Flush()
				fatalf("%s", err)
			}
			if out.NeedNL() {
				out.Write([]byte{'\n'})
			}
		}
	}

	load := packages
	if *listE {
		load = packagesAndErrors
	}

	for _, pkg := range load(args) {
		do(pkg)
	}
}

// TrackingWriter tracks the last byte written on every write so
// we can avoid printing a newline if one was already written or
// if there is no output at all.
type TrackingWriter struct {
	w    *bufio.Writer
	last byte
}

func newTrackingWriter(w io.Writer) *TrackingWriter {
	return &TrackingWriter{
		w:    bufio.NewWriter(w),
		last: '\n',
	}
}

func (t *TrackingWriter) Write(p []byte) (n int, err error) {
	n, err = t.w.Write(p)
	if n > 0 {
		t.last = p[n-1]
	}
	return
}

func (t *TrackingWriter) Flush() {
	t.w.Flush()
}

func (t *TrackingWriter) NeedNL() bool {
	return t.last != '\n'
}
