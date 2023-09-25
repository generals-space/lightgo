package sort_test

import (
	"fmt"
	"sort"
)
func ExampleSlice() {
	// 通用排序能力, 自定义排序方式.
	// 以切片的第1个成员为依据排序.
	slice := [][]int{{1, 2}, {7, 3}, {3, 4}, {1, 3}}
	sort.Slice(slice, func(i, j int) bool {
		if slice[i][0] < slice[j][0] {
			return true
		}
		return false
	})
	fmt.Printf("%+v", slice) 

	// Output:
	// [[1 2] [1 3] [3 4] [7 3]]
}
