// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*

这里的 heap 指的是小根堆(也叫小顶堆, 最小堆), 单从树的形状上看, 为完全二叉树.
即某个节点的值小于其左右子树中任意节点的值, 这就意味着, 根节点是整棵树中最小的.
(但对于左右节点之间的大小没有限制, 可以是左>右, 也可以是左<右)

堆排序就是借助小根堆/大根堆实现的, 这两种堆可以选择出数组中最小/最大值, 放在堆顶.
堆排序可以通过把堆顶元素放在数组末尾, 作为已排序区, 然后将剩余元素恢复成小根堆/大根堆.
不断重复上述流程, 则可以得到有序数组.

由于小根堆可以选出数组中最小值, 因此最终得到的数组是降序的.

heap 是实现优先级队列的一种常用方式(其实就是按照元素的 priority 优先级字段排序, 得到有序数组)

目前只有 src/cmd/go/build.go -> actionQueue{} 实现了 heap 接口,
就是一个普通数组, 借助堆排序, 实现成员的有序排列.
而且目前我也无法想像其他实现, 所以本文的解析中, 只会将 heap 的承载对象看作常规数组.

*/

// Package heap provides heap operations for any type that implements
// heap.Interface.
// A heap is a tree with the property that
// each node is the minimum-valued node in its subtree.
//
// The minimum element in the tree is the root, at index 0.
//
// A heap is a common way to implement a priority queue.
// To build a priority queue, implement the Heap interface with the
// (negative) priority as the ordering for the Less method,
// so Push adds items while Pop removes the highest-priority item from the queue.
// The Examples include such an implementation;
// the file example_pq_test.go has the complete source.
//
package heap

import "sort"

// 	@implementAt: src/cmd/go/build.go -> actionQueue{}
// 	不过, heap 包移到 compatible 目录后, 双方就没有直接关系了, 只能说是一个使用示例.
//
// Any type that implements heap.Interface may be used as a
// min-heap with the following invariants (established after
// Init has been called or if the data is empty or sorted):
//
//	!h.Less(j, i) for 0 <= i < h.Len() and j = 2*i+1 or 2*i+2 and j < h.Len()
//
// Note that Push and Pop in this interface are for package heap's
// implementation to call.
// To add and remove things from the heap, use heap.Push and heap.Pop.
type Interface interface {
	sort.Interface
	Push(x interface{}) // add x as element Len()
	Pop() interface{}   // remove and return element Len() - 1.
}

// 初始化小根堆
//
// A heap must be initialized before any of the heap operations can be used.
// Init is idempotent(幂等的) with respect to the heap invariants(不变性, 非变量)
// and may be called whenever the heap invariants may have been invalidated.
// Its complexity is O(n) where n = h.Len().
//
func Init(h Interface) {
	// heapify
	n := h.Len()
	// n/2 - 1 是二叉树中最后一个非叶子节点的索引.
	for i := n/2 - 1; i >= 0; i-- {
		down(h, i, n)
	}
}

// 向堆中添加一个节点, 并再次调整, 使堆保持为小顶堆.
//
// 注意: heap 只是一个接口, 也许实现不同, 插入时新元素的位置也会不一样.
// 但是按照函数中调用的 up(h, h.Len()-1), 基本可以确定, 新增的元素**只能加到末尾**
//
// Push pushes the element x onto the heap.
// The complexity is O(log(n)) where n = h.Len().
//
func Push(h Interface, x interface{}) {
	h.Push(x)
	up(h, h.Len()-1)
}

// 从堆中找一个最小的弹出, 并再次调整, 使堆保持为小顶堆.
//
// 由于小顶堆的特性, 排序后的数组中是升序数组, 最小的就是第0个成员.
//
// Pop removes the minimum element (according to Less) from the heap and returns it.
// The complexity is O(log(n)) where n = h.Len().
// It is equivalent to Remove(h, 0).
//
func Pop(h Interface) interface{} {
	n := h.Len() - 1
	// 交换首尾两个元素的位置
	h.Swap(0, n)
	down(h, 0, n)
	// 弹出末尾元素并返回
	return h.Pop()
}

// Remove removes the element at index i from the heap.
// The complexity is O(log(n)) where n = h.Len().
//
func Remove(h Interface, i int) interface{} {
	n := h.Len() - 1
	if n != i {
		h.Swap(i, n)
		down(h, i, n)
		up(h, i)
	}
	return h.Pop()
}

// Fix reestablishes the heap ordering after the element at index i has changed its value.
// Changing the value of the element at index i and then calling Fix is equivalent to,
// but less expensive than, calling Remove(h, i) followed by a Push of the new value.
// The complexity is O(log(n)) where n = h.Len().
func Fix(h Interface, i int) {
	down(h, i, h.Len())
	up(h, i)
}

// 从下往上调整, 将完全二叉树重新调整为小顶堆.
//
// 	@param j: 完全二叉树中某个节点的索引值.
//
// caller:
// 	1. Push()
//
// 从唯一一个主调函数来看, 由于Push()的元素只会加在结构的末尾, 
// 调整时就会先从末尾元素(即新增的元素)所在的子树开始, 调整节点与其父结点的结构, 
// 依次向上调整其所在的子树, 直到根节点.
//
//       1                     1        
//      / \                   / \       
//   2       3      =>     2       3    
//  / \     / \           / \     / \   
// 4    5 6     7        4    5 6     7 
//                     /                
//                    8                 
//
// 那么 up() 方法调整的顺序应该是按照 8 -> 4 -> 2 -> 1 这一路径完成的.
//
func up(h Interface, j int) {
	for {
		// i: j节点所属的父节点的索引值
		i := (j - 1) / 2
		if i == j || !h.Less(j, i) {
			break
		}
		// 如果 节点j的值 >= 节点i的值, 就不满足小顶堆的条件, 需要从下往上调整.
		h.Swap(i, j)
		j = i
	}
}

// 从上往下调整, 将完全二叉树重新调整为小顶堆
//
// 	@param i: 0, 数组的第0个元素, 即堆顶元素;
// 	@param i: h.Len() - 1, 数组的末尾元素;
//
// caller:
// 	1. Pop() 只有这一处
//
// 从唯一一个主调函数来看, 在调用 down() 时会先将堆顶元素与数组末尾元素调换位置.
// 之后要将末尾元素弹出, 即弹出原本的堆顶元素, 也就是数组中最小的元素.
//
// Pop()调用时, 参数中的 i 虽然为 0, 但已经是数组中最大的元素了.
// 这个调换行为破坏了小顶堆的结构, 因此需要从上向下调整.
//
//        1                      7       
//       / \                    / \      
//    2       3      =>      2       3   
//   / \     / \            / \     / \  
// 4     5 6     7        4     5 6     1
//
// 接下来就是 down() 函数的处理过程了(记得要把末尾元素1排除).
//
//        7                    2                    2       
//       / \      down()      / \      down()      / \      
//    2       3     =>     7       3     =>     4       3   
//   / \     / \          / \     / \          / \     / \  
// 4     5 6     1      4     5 6     1      7     5 6     1
//
func down(h Interface, i, n int) {
	for {
		// j1 表示 i 元素的左节点索引
		j1 := 2*i + 1
		// j1 < 0 after int overflow
		if j1 >= n || j1 < 0 {
			break
		}
		// j 表示左右节点中更小的那个
		j := j1
		// j1 + 1, 即为 i 元素的右节点
		// 如果 i 有右节点(右节点不能是末尾节点, 因为之后末尾节点要被弹出的),
		// 那么找到左右节点中, 比较小的那个
		if j2 := j1 + 1; j2 < n && !h.Less(j1, j2) {
			j = j2 // = 2*i + 2  // right child
		}
		// 如果 j(i的左右节点), 都小于 i 自身, 说明此时已满足小顶堆,
		// 不必继续向下处理了.
		if !h.Less(j, i) {
			break
		}
		// 根节点与左右节点较小的那个做交换, 然后到对应的子树递归处理.
		h.Swap(i, j)
		i = j
	}
}
