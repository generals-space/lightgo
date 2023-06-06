package testing

import (
	"runtime"
	"sync"
	"sync/atomic"
)

// 	@compatible: 该结构在 v1.3 版本初次添加
//
// A PB is used by RunParallel for running parallel benchmarks.
type PB struct {
	globalN *uint64 // shared between all worker goroutines iteration counter
	grain   uint64  // acquire that many iterations from globalN at once
	cache   uint64  // local cache of acquired iterations
	bN      uint64  // total number of iterations to execute (b.N)
}

// Next reports whether there are more iterations to execute.
func (pb *PB) Next() bool {
	if pb.cache == 0 {
		n := atomic.AddUint64(pb.globalN, pb.grain)
		if n <= pb.bN {
			pb.cache = pb.grain
		} else if n < pb.bN+pb.grain {
			pb.cache = pb.bN + pb.grain - n
		} else {
			return false
		}
	}
	pb.cache--
	return true
}

// 	@compatible: 该方法在 v1.3 版本初次添加
//
// RunParallel runs a benchmark in parallel.
// It creates multiple goroutines and distributes b.N iterations among them.
// The number of goroutines defaults to GOMAXPROCS. To increase parallelism for
// non-CPU-bound benchmarks, call SetParallelism before RunParallel.
// RunParallel is usually used with the go test -cpu flag.
//
// The body function will be run in each goroutine. It should set up any
// goroutine-local state and then iterate until pb.Next returns false.
// It should not use the StartTimer, StopTimer, or ResetTimer functions,
// because they have global effect.
func (b *B) RunParallel(body func(*PB)) {
	// Calculate grain size as number of iterations that take ~100µs.
	// 100µs is enough to amortize the overhead and provide sufficient
	// dynamic load balancing.
	grain := uint64(0)
	if b.previousN > 0 && b.previousDuration > 0 {
		grain = 1e5 * uint64(b.previousN) / uint64(b.previousDuration)
	}
	if grain < 1 {
		grain = 1
	}
	// We expect the inner loop and function call to take at least 10ns,
	// so do not do more than 100µs/10ns=1e4 iterations.
	if grain > 1e4 {
		grain = 1e4
	}

	n := uint64(0)
	numProcs := b.parallelism * runtime.GOMAXPROCS(0)
	var wg sync.WaitGroup
	wg.Add(numProcs)
	for p := 0; p < numProcs; p++ {
		go func() {
			defer wg.Done()
			pb := &PB{
				globalN: &n,
				grain:   grain,
				bN:      uint64(b.N),
			}
			body(pb)
		}()
	}
	wg.Wait()
	if n <= uint64(b.N) && !b.Failed() {
		b.Fatal("RunParallel: body exited without pb.Next() == false")
	}
}

// 	@compatible: 该方法在 v1.3 版本初次添加
//
// SetParallelism sets the number of goroutines used by RunParallel to p*GOMAXPROCS.
// There is usually no need to call SetParallelism for CPU-bound benchmarks.
// If p is less than 1, this call will have no effect.
func (b *B) SetParallelism(p int) {
	if p >= 1 {
		b.parallelism = p
	}
}
