package main

import (
    "unsafe"
)

type Person struct {
    Name string
    Age int8
    Gender bool
    Title string
}

func main() {
    p := &Person {
        Name: "general",
        Age: 21,
        Gender: true,
        Title: "president",
    }
    println(unsafe.Sizeof(p.Name)) // 16
    println(unsafe.Sizeof(p.Age))  // 1
    println(unsafe.Sizeof(p.Gender)) // 1
    println(unsafe.Sizeof(p.Title)) // 16
    println()
    println(unsafe.Offsetof(p.Name)) // 0
    println(unsafe.Offsetof(p.Age))  // 16
    println(unsafe.Offsetof(p.Gender)) // 17
    println(unsafe.Offsetof(p.Title)) // 24
    println()
    println(unsafe.Alignof(p.Name)) // 8
    println(unsafe.Alignof(p.Age))  // 1
    println(unsafe.Alignof(p.Gender)) // 1
    println(unsafe.Alignof(p.Title)) // 8
}
