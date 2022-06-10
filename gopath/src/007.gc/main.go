package main

import (
	"log"
)

func main() {
	str := "hello world"
	log.Printf("%s", str)

	list := make([]int, 1, 5)
	list[0] = 1
	log.Printf("%+v", list)
}

// go build -gcflags='-l -N' -o main.gobin main.go
// export GODEBUG=schedtrace=1000,scheddetail=1
