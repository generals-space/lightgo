// DO NOT EDIT; AUTO-GENERATED from sort.go using genzfunc.go

// Copyright 2016 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package sort

// Auto-generated variant of sort.go:insertionSort
func insertionSort_func(data lessSwap, a, b int) {
	for i := a + 1; i < b; i++ {
		for j := i; j > a && data.Less(j, j-1); j-- {
			data.Swap(j, j-1)
		}
	}
}

// Auto-generated variant of sort.go:siftDown
func siftDown_func(data lessSwap, lo, hi, first int) {
	root := lo
	for {
		child := 2*root + 1
		if child >= hi {
			break
		}
		if child+1 < hi && data.Less(first+child, first+child+1) {
			child++
		}
		if !data.Less(first+root, first+child) {
			return
		}
		data.Swap(first+root, first+child)
		root = child
	}
}

// Auto-generated variant of sort.go:heapSort
func heapSort_func(data lessSwap, a, b int) {
	first := a
	lo := 0
	hi := b - a
	for i := (hi - 1) / 2; i >= 0; i-- {
		siftDown_func(data, i, hi, first)
	}
	for i := hi - 1; i >= 0; i-- {
		data.Swap(first, first+i)
		siftDown_func(data, lo, i, first)
	}
}

// Auto-generated variant of sort.go:medianOfThree
func medianOfThree_func(data lessSwap, a, b, c int) {
	m0 := b
	m1 := a
	m2 := c
	if data.Less(m1, m0) {
		data.Swap(m1, m0)
	}
	if data.Less(m2, m1) {
		data.Swap(m2, m1)
	}
	if data.Less(m1, m0) {
		data.Swap(m1, m0)
	}
}

// Auto-generated variant of sort.go:swapRange
func swapRange_func(data lessSwap, a, b, n int) {
	for i := 0; i < n; i++ {
		data.Swap(a+i, b+i)
	}
}

// Auto-generated variant of sort.go:doPivot
func doPivot_func(data lessSwap, lo, hi int) (midlo, midhi int) {
	m := lo + (hi-lo)/2
	if hi-lo > 40 {
		s := (hi - lo) / 8
		medianOfThree_func(data, lo, lo+s, lo+2*s)
		medianOfThree_func(data, m, m-s, m+s)
		medianOfThree_func(data, hi-1, hi-1-s, hi-1-2*s)
	}
	medianOfThree_func(data, lo, m, hi-1)
	pivot := lo
	a, b, c, d := lo+1, lo+1, hi, hi
	for {
		for b < c {
			if data.Less(b, pivot) {
				b++
			} else if !data.Less(pivot, b) {
				data.Swap(a, b)
				a++
				b++
			} else {
				break
			}
		}
		for b < c {
			if data.Less(pivot, c-1) {
				c--
			} else if !data.Less(c-1, pivot) {
				data.Swap(c-1, d-1)
				c--
				d--
			} else {
				break
			}
		}
		if b >= c {
			break
		}
		data.Swap(b, c-1)
		b++
		c--
	}
	n := _min(b-a, a-lo)
	swapRange_func(data, lo, b-n, n)
	n = _min(hi-d, d-c)
	swapRange_func(data, c, hi-n, n)
	return lo + b - a, hi - (d - c)
}

// Auto-generated variant of sort.go:quickSort
func quickSort_func(data lessSwap, a, b, maxDepth int) {
	for b-a > 7 {
		if maxDepth == 0 {
			heapSort_func(data, a, b)
			return
		}
		maxDepth--
		mlo, mhi := doPivot_func(data, a, b)
		if mlo-a < b-mhi {
			quickSort_func(data, a, mlo, maxDepth)
			a = mhi
		} else {
			quickSort_func(data, mhi, b, maxDepth)
			b = mlo
		}
	}
	if b-a > 1 {
		insertionSort_func(data, a, b)
	}
}

// Auto-generated variant of sort.go:stable
func stable_func(data lessSwap, n int) {
	blockSize := 20
	a, b := 0, blockSize
	for b <= n {
		insertionSort_func(data, a, b)
		a = b
		b += blockSize
	}
	insertionSort_func(data, a, n)
	for blockSize < n {
		a, b = 0, 2*blockSize
		for b <= n {
			symMerge_func(data, a, a+blockSize, b)
			a = b
			b += 2 * blockSize
		}
		symMerge_func(data, a, a+blockSize, n)
		blockSize *= 2
	}
}

// Auto-generated variant of sort.go:symMerge
func symMerge_func(data lessSwap, a, m, b int) {
	if a >= m || m >= b {
		return
	}
	mid := a + (b-a)/2
	n := mid + m
	start := 0
	if m > mid {
		start = n - b
		r, p := mid, n-1
		for start < r {
			c := start + (r-start)/2
			if !data.Less(p-c, c) {
				start = c + 1
			} else {
				r = c
			}
		}
	} else {
		start = a
		r, p := m, n-1
		for start < r {
			c := start + (r-start)/2
			if !data.Less(p-c, c) {
				start = c + 1
			} else {
				r = c
			}
		}
	}
	end := n - start
	rotate_func(data, start, m, end)
	symMerge_func(data, a, start, mid)
	symMerge_func(data, mid, end, b)
}

// Auto-generated variant of sort.go:rotate
func rotate_func(data lessSwap, a, m, b int) {
	i := m - a
	if i == 0 {
		return
	}
	j := b - m
	if j == 0 {
		return
	}
	if i == j {
		swapRange_func(data, a, m, i)
		return
	}
	p := a + i
	for i != j {
		if i > j {
			swapRange_func(data, p-i, p, j)
			i -= j
		} else {
			swapRange_func(data, p-i, p+j-i, i)
			j -= i
		}
	}
	swapRange_func(data, p-i, p, i)
}
