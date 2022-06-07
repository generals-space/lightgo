package main

import (
	"log"
	"time"
)

func main() {
	channel := make(chan int, 3)
	for i := 0; i < 5; i++ {
		go func(i int) {
			channel <- i
		}(i)
	}

	for i := 0; i < 5; i++ {
		time.Sleep(time.Second * 1)
		log.Printf("%d\n", <-channel)
		if len(channel) == 0 {
			break
		}
	}
}

// go build -gcflags='-l -N' -ldflags=-w -o main.gobin main.go
// export GODEBUG=schedtrace=1000,scheddetail=1
