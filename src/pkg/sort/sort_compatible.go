package sort

import (
	"reflect"
)

/*

// 示例

*/

// 	@compatible: 该结构在 v1.8 版本初次出现.
// 
// lessSwap is a pair of Less and Swap function for use with the
// auto-generated func-optimized variant of sort.go in zfuncversion.go.
type lessSwap struct {
	Less func(i, j int) bool
	Swap func(i, j int)
}

// maxDepth returns a threshold at which quicksort should switch to heapsort.
// It returns 2*ceil(lg(n+1)).
func maxDepth(n int) int {
	var depth int
	for i := n; i > 0; i >>= 1 {
		depth++
	}
	return depth * 2
}

// Slice 借助 less 参数方法, 实现了通用排序能力.
//
// 	@param less: 用于判断两个数组成员排序先后的方法, 需要根据不同的 slice 类型编写不同的方法,
// 	也可以按自身要求, 编写逆序排序的方法.
//
// [anchor] 使用示例, 请见 016.sort 中的 main() 函数.
//
// Slice sorts the provided slice given the provided less function.
//
// The sort is not guaranteed to be stable. For a stable sort, use SliceStable.
//
// The function panics if the provided interface is not a slice.
func Slice(slice interface{}, less func(i, j int) bool) {
	rv := reflect.ValueOf(slice)
	swap := reflect.Swapper(slice)
	length := rv.Len()
	quickSort_func(lessSwap{less, swap}, 0, length, maxDepth(length))
}

// SliceStable sorts the provided slice given the provided less
// function while keeping the original order of equal elements.
//
// The function panics if the provided interface is not a slice.
func SliceStable(slice interface{}, less func(i, j int) bool) {
	rv := reflect.ValueOf(slice)
	swap := reflect.Swapper(slice)
	stable_func(lessSwap{less, swap}, rv.Len())
}

// SliceIsSorted tests whether a slice is sorted.
//
// The function panics if the provided interface is not a slice.
func SliceIsSorted(slice interface{}, less func(i, j int) bool) bool {
	rv := reflect.ValueOf(slice)
	n := rv.Len()
	for i := n - 1; i > 0; i-- {
		if less(i, i-1) {
			return false
		}
	}
	return true
}
