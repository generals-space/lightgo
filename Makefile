## make默认执行第一个标签的内容

## 构建 go 二进制文件, 默认放在 ./bin 目录下.
build: 
	@## "@" 可以不打印执行命令及输出信息
	@## "|| true" 是因为 mv -f 只能避免目标文件已存在的场景, 对于源文件不存在的情况仍然会报错退出.
	@mv -f src/pkg/sort/sort_compatible.go src/pkg/sort/sort_compatible.go.1 > /dev/null 2>&1 || true
	@mv -f src/pkg/sort/zfuncversion_compatible.go src/pkg/sort/zfuncversion_compatible.go.1 > /dev/null 2>&1 || true
	@mv -f src/pkg/os/dir_compatible.go src/pkg/os/dir_compatible.go.1 > /dev/null 2>&1 || true
	cd ./src; bash ./make.bash; cd ..;
	@mv -f src/pkg/sort/sort_compatible.go.1 src/pkg/sort/sort_compatible.go > /dev/null 2>&1 || true
	@mv -f src/pkg/sort/zfuncversion_compatible.go.1 src/pkg/sort/zfuncversion_compatible.go > /dev/null 2>&1 || true
	@mv -f src/pkg/os/dir_compatible.go.1 src/pkg/os/dir_compatible.go > /dev/null 2>&1 || true

## 只执行 make.bash 的第1阶段, 即只构建 dist 命令, 不再构建 6c,6g,6l,及 go 命令和标准库.
dist:
	cd ./src; bash ./make.bash --dist-tool; cd ..;

install:
	cp -f ./bin/* /usr/local/bin/

clean:
	rm -rf ./bin/*;
	rm -rf ./pkg/*;
