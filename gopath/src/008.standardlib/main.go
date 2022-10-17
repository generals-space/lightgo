package main

import (
	"fmt"
	"strings"
)

func main() {
	str := "hello world"
	strArray := strings.Split(str, " ")
	fmt.Printf("%+v\n", strArray)
}
