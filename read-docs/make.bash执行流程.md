[root@8fa68def7f37 go-cbuild-NnkpPv]# /root/go/pkg/tool/linux_amd64/6g -o /var/tmp/go-cbuild-NnkpPv/_go_.6 -p main /root/go/src/cmd/go/bootstrap.go /root/go/src/cmd/go/build.go /root/go/src/cmd/go/clean.go /root/go/src/cmd/go/env.go /root/go/src/cmd/go/fix.go /root/go/src/cmd/go/fmt.go /root/go/src/cmd/go/get.go /root/go/src/cmd/go/go11.go /root/go/src/cmd/go/help.go /root/go/src/cmd/go/list.go /root/go/src/cmd/go/main.go /root/go/src/cmd/go/pkg.go /root/go/src/cmd/go/run.go /root/go/src/cmd/go/signal.go /root/go/src/cmd/go/signal_unix.go /root/go/src/cmd/go/test.go /root/go/src/cmd/go/testflag.go /root/go/src/cmd/go/tool.go /root/go/src/cmd/go/vcs.go /root/go/src/cmd/go/version.go /root/go/src/cmd/go/vet.go /root/go/src/cmd/go/zdefaultcc.go
[root@8fa68def7f37 go-cbuild-NnkpPv]# ls
arch_amd64.h  defs_GOOS_GOARCH.h  _go_.6  os_GOOS.h  proc.acid  runtimedefs  signal_GOOS_GOARCH.h  signals_GOOS.h  zasm_GOOS_GOARCH.h
[root@8fa68def7f37 go-cbuild-NnkpPv]# ll
total 2376
-rw-r--r-- 1 root root     378 Nov  1 11:03 arch_amd64.h
-rw-r--r-- 1 root root    4130 Nov  1 11:03 defs_GOOS_GOARCH.h
-rw-r--r-- 1 root root 2332948 Nov  1 11:51 _go_.6
-rw-r--r-- 1 root root    1067 Nov  1 11:03 os_GOOS.h
-rw-r--r-- 1 root root   42273 Nov  1 11:03 proc.acid
-rw-r--r-- 1 root root   16653 Nov  1 11:03 runtimedefs
-rw-r--r-- 1 root root    1399 Nov  1 11:03 signal_GOOS_GOARCH.h
-rw-r--r-- 1 root root    2574 Nov  1 11:03 signals_GOOS.h
-rw-r--r-- 1 root root    7237 Nov  1 11:03 zasm_GOOS_GOARCH.h
[root@8fa68def7f37 go-cbuild-NnkpPv]# ll
total 2376
-rw-r--r-- 1 root root     378 Nov  1 11:03 arch_amd64.h
-rw-r--r-- 1 root root    4130 Nov  1 11:03 defs_GOOS_GOARCH.h
-rw-r--r-- 1 root root 2332948 Nov  1 11:52 _go_.6
-rw-r--r-- 1 root root    1067 Nov  1 11:03 os_GOOS.h
-rw-r--r-- 1 root root   42273 Nov  1 11:03 proc.acid
-rw-r--r-- 1 root root   16653 Nov  1 11:03 runtimedefs
-rw-r--r-- 1 root root    1399 Nov  1 11:03 signal_GOOS_GOARCH.h
-rw-r--r-- 1 root root    2574 Nov  1 11:03 signals_GOOS.h
-rw-r--r-- 1 root root    7237 Nov  1 11:03 zasm_GOOS_GOARCH.h
[root@8fa68def7f37 go-cbuild-NnkpPv]# pwd
/var/tmp/go-cbuild-NnkpPv
[root@8fa68def7f37 go-cbuild-NnkpPv]# /root/go/pkg/tool/linux_amd64/6l -o /root/go/pkg/tool/linux_amd64/go_bootstrap /var/tmp/go-cbuild-NnkpPv/_go_.6
