package main

import (
	"lib"
)

func init() {
	println("test.init")
}
func test() {
	println(lib.Sum(1, 2, 3))
}