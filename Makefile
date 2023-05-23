## make默认执行第一个标签的内容

## 构建 go 二进制文件, 默认放在 ./bin 目录下.
build: 
	cd ./src; bash ./make.bash; cd ..;
clean:
	rm -f ./bin/*
