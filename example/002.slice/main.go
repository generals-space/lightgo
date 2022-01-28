package main

import (
	"fmt"
)

func main(){
	// src/pkg/runtime/slice.c -> runtime.makeslice()
	list1 := make([]string, 5, 10)
	list1[0] = "hello world"
	fmt.Println("%+v", list1)
	
	// src/pkg/runtime/malloc.goc -> runtime.new()
	list2 := []string{"hello world"}
	fmt.Println("%+v", list2)
}

// go build -gcflags='-l -N' -o main.gobin main.go 
