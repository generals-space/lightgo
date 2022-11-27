package main

import (
	"log"
	"time"
)

func chan01() {
	// 这是一个空 channel, 未初始化
	var schan chan string
	select {
	case schan <- "hello":
		log.Printf("write completed\n")
	default:
		log.Printf("empty\n")
	}
}

func chan02() {
	var schan chan string
	schan = make(chan string)
	schan <- "hello"
	log.Printf("yes\n")
}

func chan03() {
	var schan chan string
	schan = make(chan string)
	// 有下面这个协程在, 就不会出现因为 deadlock 而 panic 的情况了, 而是直接阻塞.
	go func(){
		// 1个小时后才有 receiver 出现
		time.Sleep(time.Second*3600)
		log.Printf("%s\n", <-schan)
	}()
	schan <- "hello"
	log.Printf("yes\n")
}

func main() {
	// chan01()
	// chan02()
	chan03()
}
