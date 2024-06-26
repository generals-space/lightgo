package main

// actionQueue 优先级队列.
//
// 其实就是一个有序数组, 借助堆排序实现升序, 即 actionQueue[0]最小, actionQueue[n]最大.
//
// 	@implementOf: container/heap/heap.go -> Interface{}
// 	不过, heap 包移到 compatible 目录后, 双方就没有直接关系了, 只能说是一个使用示例.
//
// An actionQueue is a priority queue of actions.
type actionQueue []*action

func (q *actionQueue) Len() int           { return len(*q) }
func (q *actionQueue) Swap(i, j int)      { (*q)[i], (*q)[j] = (*q)[j], (*q)[i] }
func (q *actionQueue) Less(i, j int) bool { return (*q)[i].priority < (*q)[j].priority }
func (q *actionQueue) Push(x interface{}) { *q = append(*q, x.(*action)) }

// Pop 弹出数组的末尾元素并返回
func (q *actionQueue) Pop() interface{} {
	n := len(*q) - 1 // 数组长度
	x := (*q)[n]     // 末尾元素
	*q = (*q)[:n]    // 弹出末尾元素
	return x
}

func (q *actionQueue) push(a *action) {
	q.Push(a)
	q.up(q.Len() - 1)
}

// pop() 弹出的是数组的第0个元素, 也是堆顶元素, 是整个数组中 priority 最小的元素.
//
// 在主调函数构建依赖树的时候, 是按序号作为优先级的, 越底层的包, 优先级越小.
//
// caller:
// 	1. main()
func (q *actionQueue) pop() *action {
	n := q.Len() - 1
	q.Swap(0, n)
	q.down(0, n)
	return q.Pop().(*action)
}

// 从下往上调整, 将完全二叉树重新调整为小顶堆.
//
// 	@compatibleNote: 此方法取自 container/heap -> up()
func (q *actionQueue) up(j int) {
	for {
		i := (j - 1) / 2 // parent
		if i == j || !q.Less(j, i) {
			break
		}
		q.Swap(i, j)
		j = i
	}
}

// 从上往下调整, 将完全二叉树重新调整为小顶堆
//
// 	@compatibleNote: 此方法取自 container/heap -> down()
func (q *actionQueue) down(i, n int) {
	for {
		j1 := 2*i + 1
		if j1 >= n || j1 < 0 { // j1 < 0 after int overflow
			break
		}
		j := j1 // left child
		if j2 := j1 + 1; j2 < n && !q.Less(j1, j2) {
			j = j2 // = 2*i + 2  // right child
		}
		if !q.Less(j, i) {
			break
		}
		q.Swap(i, j)
		i = j
	}
}
