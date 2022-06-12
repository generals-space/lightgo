package main

func main() {
	a := new([100]byte)
	_ = a

	b := new([32*1024]byte)
	_ = b
}

// go build -gcflags='-l -N' -o main.gobin main.go
// export GODEBUG=schedtrace=1000,scheddetail=1
