package sort_test

import (
	"testing"
	"math/rand"
	"strconv"

	. "sort"
)

func TestSlice(t *testing.T) {
	data := strings
	Slice(data[:], func(i, j int) bool {
		return data[i] < data[j]
	})
	if !SliceIsSorted(data[:], func(i, j int) bool { return data[i] < data[j] }) {
		t.Errorf("sorted %v", strings)
		t.Errorf("   got %v", data)
	}
}

type nonDeterministicTestingData struct {
	r *rand.Rand
}

func (t *nonDeterministicTestingData) Len() int {
	return 500
}
func (t *nonDeterministicTestingData) Less(i, j int) bool {
	if i < 0 || j < 0 || i >= t.Len() || j >= t.Len() {
		panic("nondeterministic comparison out of bounds")
	}
	return t.r.Float32() < 0.5
}
func (t *nonDeterministicTestingData) Swap(i, j int) {
	if i < 0 || j < 0 || i >= t.Len() || j >= t.Len() {
		panic("nondeterministic comparison out of bounds")
	}
}

func TestNonDeterministicComparison(t *testing.T) {
	// Ensure that sort.Sort does not panic when Less returns inconsistent results.
	// See https://golang.org/issue/14377.
	defer func() {
		if r := recover(); r != nil {
			t.Error(r)
		}
	}()

	td := &nonDeterministicTestingData{
		r: rand.New(rand.NewSource(0)),
	}

	for i := 0; i < 10; i++ {
		Sort(td)
	}
}

func BenchmarkSortString1K_Slice(b *testing.B) {
	b.StopTimer()
	unsorted := make([]string, 1<<10)
	for i := range unsorted {
		unsorted[i] = strconv.Itoa(i ^ 0x2cc)
	}
	data := make([]string, len(unsorted))

	for i := 0; i < b.N; i++ {
		copy(data, unsorted)
		b.StartTimer()
		Slice(data, func(i, j int) bool { return data[i] < data[j] })
		b.StopTimer()
	}
}

func BenchmarkStableInt1K_Slice(b *testing.B) {
	b.StopTimer()
	unsorted := make([]int, 1<<10)
	for i := range unsorted {
		unsorted[i] = i ^ 0x2cc
	}
	data := make([]int, len(unsorted))
	for i := 0; i < b.N; i++ {
		copy(data, unsorted)
		b.StartTimer()
		SliceStable(data, func(i, j int) bool { return data[i] < data[j] })
		b.StopTimer()
	}
}

func BenchmarkSortInt64K_Slice(b *testing.B) {
	b.StopTimer()
	for i := 0; i < b.N; i++ {
		data := make([]int, 1<<16)
		for i := 0; i < len(data); i++ {
			data[i] = i ^ 0xcccc
		}
		b.StartTimer()
		Slice(data, func(i, j int) bool { return data[i] < data[j] })
		b.StopTimer()
	}
}
