package main

import (
	"bytes"
	"errors"
	"fmt"
	"os/exec"
	"regexp"
	"strings"
	"time"
)

// caller:
// 	1. builder.run()
// 	2. builder.showcmd()
//
// fmtcmd formats a command in the manner of fmt.Sprintf but also:
//
//	If dir is non-empty and the script is not in dir right now,
//	fmtcmd inserts "cd dir\n" before the command.
//
//	fmtcmd replaces the value of b.work with $WORK.
//	fmtcmd replaces the value of goroot with $GOROOT.
//	fmtcmd replaces the value of b.gobin with $GOBIN.
//
//	fmtcmd replaces the name of the current directory with dot (.)
//	but only when it is at the beginning of a space-separated token.
//
func (b *builder) fmtcmd(dir string, format string, args ...interface{}) string {
	cmd := fmt.Sprintf(format, args...)
	if dir != "" && dir != "/" {
		cmd = strings.Replace(" "+cmd, " "+dir, " .", -1)[1:]
		if b.scriptDir != dir {
			b.scriptDir = dir
			cmd = "cd " + dir + "\n" + cmd
		}
	}
	if b.work != "" {
		cmd = strings.Replace(cmd, b.work, "$WORK", -1)
	}
	return cmd
}

// caller:
// 	1. builder.copyFile()
// 	2. builder.runOut()
// 	3. builder.mkdir()
//
// showcmd prints the given command to standard output
// for the implementation of -n or -x.
func (b *builder) showcmd(dir string, format string, args ...interface{}) {
	b.output.Lock()
	defer b.output.Unlock()
	b.print(b.fmtcmd(dir, format, args...) + "\n")
}

// showOutput prints "# desc" followed by the given output.
// The output is expected to contain references to 'dir', usually
// the source directory for the package that has failed to build.
// showOutput rewrites mentions of dir with a relative path to dir
// when the relative path is shorter.  This is usually more pleasant.
// For example, if fmt doesn't compile and we are in src/pkg/html,
// the output is
//
//	$ go build
//	# fmt
//	../fmt/print.go:1090: undefined: asdf
//	$
//
// instead of
//
//	$ go build
//	# fmt
//	/usr/gopher/go/src/pkg/fmt/print.go:1090: undefined: asdf
//	$
//
// showOutput also replaces references to the work directory with $WORK.
//
func (b *builder) showOutput(dir, desc, out string) {
	prefix := "# " + desc
	suffix := "\n" + out
	if reldir := shortPath(dir); reldir != dir {
		suffix = strings.Replace(suffix, " "+dir, " "+reldir, -1)
		suffix = strings.Replace(suffix, "\n"+dir, "\n"+reldir, -1)
	}
	suffix = strings.Replace(suffix, " "+b.work, " $WORK", -1)

	b.output.Lock()
	defer b.output.Unlock()
	b.print(prefix, suffix)
}

// errPrintedOutput is a special error indicating that a command failed
// but that it generated output as well, and that output has already
// been printed, so there's no point showing 'exit status 1' or whatever
// the wait status was.  The main executor, builder.do, knows not to
// print this error.
var errPrintedOutput = errors.New("already printed output - no need to show error")

var cgoLine = regexp.MustCompile(`\[[^\[\]]+\.cgo1\.go:[0-9]+\]`)

// run 调用 builder.runOut()(底层调用的是 exec)执行目标命令
//
// run runs the command given by cmdline in the directory dir.
// If the command fails, run prints information about the failure
// and returns a non-nil error.
func (b *builder) run(dir string, desc string, env []string, cmdargs ...interface{}) error {
	out, err := b.runOut(dir, desc, env, cmdargs...)
	if len(out) > 0 {
		if desc == "" {
			desc = b.fmtcmd(dir, "%s", strings.Join(stringList(cmdargs...), " "))
		}
		b.showOutput(dir, desc, b.processOutput(out))
		if err != nil {
			err = errPrintedOutput
		}
	}
	return err
}

// processOutput 对 exec 的执行结果做一些额外处理, 同时将 []byte -> string.
//
// caller:
// 	1. builder.build()
// 	2. builder.run()
// 	3. builder.swigOne()
//
// processOutput prepares the output of runOut to be output to the console.
func (b *builder) processOutput(out []byte) string {
	if out[len(out)-1] != '\n' {
		out = append(out, '\n')
	}
	messages := string(out)
	// Fix up output referring to cgo-generated code to be more readable.
	// Replace x.go:19[/tmp/.../x.cgo1.go:18] with x.go:19.
	// Replace _Ctype_foo with C.foo.
	// If we're using -x, assume we're debugging and want the full dump, so disable the rewrite.
	if !buildX && cgoLine.MatchString(messages) {
		messages = cgoLine.ReplaceAllString(messages, "")
		messages = strings.Replace(messages, "type _Ctype_", "type C.", -1)
	}
	return messages
}

// runOut 通过 exec 执行目标命令并返回结果.
//
// 	@param dir: 为目标命令设置的 cwd 值
// 	@param desc:
// 	@param env: 为目标命令设置的环境变量
// 	@param cmdargs: 目标命令, 包含命令名及参数
//
// caller:
// 	1. builder.run()
//
// runOut runs the command given by cmdline in the directory dir.
// It returns the command output and any errors that occurred.
func (b *builder) runOut(dir string, desc string, env []string, cmdargs ...interface{}) ([]byte, error) {
	cmdline := stringList(cmdargs...)
	if buildN || buildX {
		b.showcmd(dir, "%s", joinUnambiguously(cmdline))
		if buildN {
			return nil, nil
		}
	}

	nbusy := 0
	for {
		var buf bytes.Buffer
		cmd := exec.Command(cmdline[0], cmdline[1:]...)
		cmd.Stdout = &buf
		cmd.Stderr = &buf
		cmd.Dir = dir
		cmd.Env = mergeEnvLists(env, envForDir(cmd.Dir))
		err := cmd.Run()

		// cmd.Run will fail on Unix if some other process has the binary
		// we want to run open for writing.  This can happen here because
		// we build and install the cgo command and then run it.
		// If another command was kicked off while we were writing the
		// cgo binary, the child process for that command may be holding
		// a reference to the fd, keeping us from running exec.
		//
		// But, you might reasonably wonder, how can this happen?
		// The cgo fd, like all our fds, is close-on-exec, so that we need
		// not worry about other processes inheriting the fd accidentally.
		// The answer is that running a command is fork and exec.
		// A child forked while the cgo fd is open inherits that fd.
		// Until the child has called exec, it holds the fd open and the
		// kernel will not let us run cgo.  Even if the child were to close
		// the fd explicitly, it would still be open from the time of the fork
		// until the time of the explicit close, and the race would remain.
		//
		// On Unix systems, this results in ETXTBSY, which formats
		// as "text file busy".  Rather than hard-code specific error cases,
		// we just look for that string.  If this happens, sleep a little
		// and try again.  We let this happen three times, with increasing
		// sleep lengths: 100+200+400 ms = 0.7 seconds.
		//
		// An alternate solution might be to split the cmd.Run into
		// separate cmd.Start and cmd.Wait, and then use an RWLock
		// to make sure that copyFile only executes when no cmd.Start
		// call is in progress.  However, cmd.Start (really syscall.forkExec)
		// only guarantees that when it returns, the exec is committed to
		// happen and succeed.  It uses a close-on-exec file descriptor
		// itself to determine this, so we know that when cmd.Start returns,
		// at least one close-on-exec file descriptor has been closed.
		// However, we cannot be sure that all of them have been closed,
		// so the program might still encounter ETXTBSY even with such
		// an RWLock.  The race window would be smaller, perhaps, but not
		// guaranteed to be gone.
		//
		// Sleeping when we observe the race seems to be the most reliable
		// option we have.
		//
		// http://golang.org/issue/3001
		//
		if err != nil && nbusy < 3 && strings.Contains(err.Error(), "text file busy") {
			time.Sleep(100 * time.Millisecond << uint(nbusy))
			nbusy++
			continue
		}

		return buf.Bytes(), err
	}
}
