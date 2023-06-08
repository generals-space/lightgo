#!/usr/bin/env bash
# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Environment variables that control make.bash:
#
# GOROOT_FINAL: The expected final Go root, baked into binaries.
# The default is the location of the Go tree during the build.
#
# GOHOSTARCH: The architecture for host tools (compilers and binaries). 
# Binaries of this type must be executable on the current system,
# so the only common reason to set this is to set
# GOHOSTARCH=386 on an amd64 machine.
#
# GOARCH: The target architecture for installed packages and tools.
#
# GOOS: The target operating system for installed packages and tools.
#
# GO_GCFLAGS: Additional 5g/6g/8g arguments to use when
# building the packages and commands.
#
# GO_LDFLAGS: Additional 5l/6l/8l arguments to use when
# building the commands.
#
# GO_CCFLAGS: Additional 5c/6c/8c arguments to use when
# building.
#
# CGO_ENABLED: Controls cgo usage during the build. Set it to 1
# to include all cgo related files, .c and .go file with "cgo"
# build directive, in the build. Set it to 0 to ignore them.
#
# GO_EXTLINK_ENABLED: Set to 1 to invoke the host linker when building
# packages that use cgo.  Set to 0 to do all linking internally.  This
# controls the default behavior of the linker's -linkmode option.  The
# default value depends on the system.
#
# CC: Command line to run to get at host C compiler.
# Default is "gcc". Also supported: "clang".
# CXX: Command line to run to get at host C++ compiler, only recorded
# for cgo use. Default is "g++". Also supported: "clang++".
#
# GO_DISTFLAGS: extra flags to provide to "dist bootstrap". Use "-s"
# to build a statically linked toolchain.

set -e

################################################################################
## 环境检测

if [ ! -f run.bash ]; then
	echo 'make.bash must be run from $GOROOT/src' 1>&2
	exit 1
fi

# Test for bad ld.
if ld --version 2>&1 | grep 'gold.* 2\.20' >/dev/null; then
	echo 'ERROR: Your system has gold 2.20 installed.'
	echo 'This version is shipped by Ubuntu even though'
	echo 'it is known not to work on Ubuntu.'
	echo 'Binaries built with this linker are likely to fail in mysterious ways.'
	echo
	echo 'Run sudo apt-get remove binutils-gold.'
	echo
	exit 1
fi

# Test for bad SELinux.
# On Fedora 16 the selinux filesystem is mounted at /sys/fs/selinux,
# so loop through the possible selinux mount points.
for se_mount in /selinux /sys/fs/selinux
do
	if [ -d $se_mount -a -f $se_mount/booleans/allow_execstack -a -x /usr/sbin/selinuxenabled ] && /usr/sbin/selinuxenabled; then
		if ! cat $se_mount/booleans/allow_execstack | grep -c '^1 1$' >> /dev/null ; then
			echo "WARNING: the default SELinux policy on, at least, Fedora 12 breaks "
			echo "Go. You can enable the features that Go needs via the following "
			echo "command (as root):"
			echo "  # setsebool -P allow_execstack 1"
			echo
			echo "Note that this affects your system globally! "
			echo
			echo "The build will continue in five seconds in case we "
			echo "misdiagnosed the issue..."

			sleep 5
		fi
	fi
done

# Clean old generated file that will cause problems in the build.
rm -f ./pkg/runtime/runtime_defs.go

# Finally! Run the build.

################################################################################
## 环境检测完成, 开始构建

echo '# Building C bootstrap tool.'
echo cmd/dist
export GOROOT="$(cd .. && pwd)"
GOROOT_FINAL="${GOROOT_FINAL:-$GOROOT}"
DEFGOROOT='-DGOROOT_FINAL="'"$GOROOT_FINAL"'"'

mflag=""
case "$GOHOSTARCH" in
386) mflag=-m32;;
amd64) mflag=-m64;;
esac

################################################################################
## Step 1: 先编译生成 dist 工具

## gcc 选项解释:
##
## -I: include 的缩写, 表示头文件路径.
## -D: 在编译期间向 .c 程序中传入宏选项, 在 .c 程序中可以直接获取 GOROOT_FINAL 的值.
##
## 实际的命令应该为
## gcc -m64 -O0 -Wall -Werror -o cmd/dist/dist -Icmd/dist '-DGOROOT_FINAL="/usr/local/go"' cmd/dist/*.c
##
## ${CC:-gcc} $mflag -Wall -Werror -g -o cmd/dist/dist -Icmd/dist "$DEFGOROOT" cmd/dist/*.c
${CC:-gcc} $mflag -O0 -Wall -Werror -o cmd/dist/dist -Icmd/dist "$DEFGOROOT" cmd/dist/*.c

## dist env 打印出的内容与 go env 差不多
# -e doesn't propagate out of eval, so check success by hand.
eval $(./cmd/dist/dist env -p || echo FAIL=true)
if [ "$FAIL" = true ]; then
	exit 1
fi

echo

## 如果命令类似于`./make.bash --dist-tool`, 说明是只想构建 dist 工具, 执行到这里即可退出.
if [ "$1" = "--dist-tool" ]; then
	# Stop after building dist tool.
	mkdir -p "$GOTOOLDIR"
	if [ "$2" != "" ]; then
		cp cmd/dist/dist "$2"
	fi
	mv cmd/dist/dist "$GOTOOLDIR"/dist
	exit 0
fi

################################################################################
## Step 2: 使用 dist 构建 6c,6g,6l,go_bootstrap 工具及核心标准库.
## 不过 pkg/linux_amd64/ 目录下并没有生成 .a 文件.
##
## go_bootstrap 可以说是一个精简版的 go 命令, ta也可以编译一些简单的 .go 文件,
## 这些文件只能引用"核心标准库", 否则会报错 can't find import: "strings"
##
## go_bootstrap 入口函数在 src/cmd/go/main.go -> main()

echo "# Building compilers and Go bootstrap tool for host, $GOHOSTOS/$GOHOSTARCH."
buildall="-a"
if [ "$1" = "--no-clean" ]; then
	buildall=""
fi

## 实际的命令应该为
## ./cmd/dist/dist bootstrap -a -v
##
## 这一行将生成 pkg/tool/linux_amd64/{6a,6c,6g,6l,go_bootstrap} 等工具
./cmd/dist/dist bootstrap $buildall $GO_DISTFLAGS -v
mv cmd/dist/dist "$GOTOOLDIR"/dist
"$GOTOOLDIR"/go_bootstrap clean -i std
echo

################################################################################
## Step 3: 使用 go_bootstrap 构建标准库及 go,gofmt 二进制文件

echo "# Building packages and commands for $GOOS/$GOARCH."
## 实际命令应该为
## go_bootstrap install -ccflags '' -gcflags '' -ldflags '' -v std
##
## 将在 pkg/linux_amd64/ 目录下生成所有标准库的 .a 文件, 以及 bin/go 二进制可执行文件.
##
## 实际上, go_bootstrap install 就类似于 go install, 即安装(加载)库文件,
## std 即为"标准库"的统称, 对该别名的处理可见如下函数.
## src/cmd/go/main.go -> importPathsNoDotExpansion()
"$GOTOOLDIR"/go_bootstrap install $GO_FLAGS -ccflags "$GO_CCFLAGS" -gcflags "$GO_GCFLAGS" -ldflags "$GO_LDFLAGS" -v std
echo

rm -f "$GOTOOLDIR"/go_bootstrap

## 打印 Successful 信息
if [ "$1" != "--no-banner" ]; then
	"$GOTOOLDIR"/dist banner
fi
