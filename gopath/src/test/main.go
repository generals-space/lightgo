// 	@example_test: 本类型文件的目的在于提供示例, 且尽量实现将文件直接迁移到某个 main.go 文件,
// 然后可以直接执行的程度(要将 ExampleXXX 改成 main()).

package main

import "net/http"

func main() {
	http.Handle("/tmpfiles", http.FileServer(http.Dir("/tmp")))
	http.ListenAndServe(":8080", nil)
}
