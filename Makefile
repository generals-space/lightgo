## make默认执行第一个标签的内容

## 构建 go 二进制文件, 默认放在 ./bin 目录下.
build: 
	cd ./src; bash ./make.bash; cd ..;
clean:
	rm -f ./bin/*
## 将新生成的 go 二进制文件放到 /usr/local/bin 目录下, 就可以使用了
copy:
	cp -f ./bin/* /usr/local/bin/
