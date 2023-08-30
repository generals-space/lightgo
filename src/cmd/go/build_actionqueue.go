package main

// actionQueue 优先级队列.
//
// 其实就是一个有序数组, 借助堆排序实现升序, 即 actionQueue[0]最小, actionQueue[n]最大.
//
// Push() 和 Pop() 操作的都是数组的末尾元素.
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
func (q *actionQueue) Pop() interface{} {
	n := len(*q) - 1
	x := (*q)[n]
	*q = (*q)[:n]
	return x
}

func (q *actionQueue) push(a *action) {
	q.Push(a)
	q.up(q.Len() - 1)
}

func (q *actionQueue) pop() *action {
	n := q.Len() - 1
	q.Swap(0, n)
	q.down(0, n)
	return q.Pop().(*action)
}

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
