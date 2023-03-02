package main

import (
	"time"
	"log"
)

func task(){
	slice := []string{}
	for {
		slice = append(slice, "hello kitty")
		if len(slice) > 1024 {
			slice = slice[1024:]
			time.Sleep(time.Second * 1)
			log.Printf("hello kitty")
		}
	}
	log.Printf("exit")
}
func main() {
	// task()
	slice := []string{}
	for {
		slice = append(slice, "hello world")
		if len(slice) > 1024 {
			slice = slice[1024:]
			time.Sleep(time.Second * 1)
			log.Printf("hello world")
		}
	}
	log.Printf("exit")
}
