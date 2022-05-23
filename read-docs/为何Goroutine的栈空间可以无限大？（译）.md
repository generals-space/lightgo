# 为何Goroutine的栈空间可以无限大？（译）

[原文链接](http://blog.xiayf.cn/2014/01/17/goroutine-stack-infinite/)

原文：[Why is a Goroutine's stack infinite?](http://dave.cheney.net/2013/06/02/why-is-a-goroutines-stack-infinite)

译者：[youngsterxyf](https://github.com/youngsterxyf)

Go编程新手可能会偶然发现Go语言---与一个Goroutine可用栈空间大小相关---的一个古怪特性。这通常是由于程序员 无意间构造了一个无限递归函数调用而产生的。为了阐明这个特性，以如下代码（有点刻意设计的）为例。

```go
package main

import "fmt"

type S struct {
    a, b int
}

// String implements the fmt.Stringer interface
func (s *S) String() string {
    return fmt.Sprintf("%s", s)     // Sprintf will call s.String()
}

func main() {
    s := &S{a: 1, b: 2}
    fmt.Println(s)
}
```

如果你运行这个程序（**我不建议你这样做**），你会发现你的机器开始频繁地swap（译注：不了解swap的，可以简单理解为“内存与硬盘之间数据的导出导入”）， 并且可能不再响应操作事件，除非你在一切无法挽回之前及时地按下^C。我知道所有人都会先在Go官网的playground中尝试运行这个程序， [所以我已经为你准备好了](http://dave.cheney.net/2013/06/02/why-is-a-goroutines-stack-infinite)。

**大多数程序员应该都遇到过无限递归导致的问题，但这都只是对于他们的程序来说是致命的，对于他们的机器通常来说并不是。那么，为何Go程序会不同呢？**

Goroutine的主要特征之一是其开销---在内存占用初始化方面，创建一个Goroutine的开销非常小（相比于一个传统POSIX线程的1-8M字节），并且 Goroutine的栈空间是按需扩大和缩小的。这就允许一个Goroutine以单个4096字节的栈空间开始，然后按需扩容缩容，也不用担心栈空间耗尽的风险。

为了实现这一特性，链接器（5l，6l，8l）在每个函数的开头都插入一小段前导代码<sup>1</sup>，这段代码会检测该函数需要的栈空间大小是否小于当前可用的栈空间。 若大于，则调用`runtime.morestack`分配一个新的栈页（stack page）<sup>2</sup>，拷贝函数调用方传递来的参数，然后将控制权返回给原来要调用的函数， 这样这个函数就可以安全运行了。当这个函数退出时，再撤销操作，将函数返回值拷贝回函数调用方的栈帧（stack frame），不再需要的栈空间也被释放。

通过这个过程，栈空间就好像无限大一样，若假设不会持续地跨越两个栈的大小边界-通常称为栈切分（stack splitting）（译注：不太理解这句话，应该是指：程序执行 到函数调用方，正好将近耗尽预分配的栈空间，而函数调用方中又不断地调用其他函数，这样每次函数调用就需要分配新的栈空间，函数调用结束后又需要 释放新分配的栈空间，所以开销积累起来就比较大），这种栈空间分配方式的开销也会很小。

然而，直到现在我都还未披露一个细节---粗心地使用递归函数导致内存耗尽的话，当需要新的栈页时，就会从堆上分配（译注：这句话可能有点问题。应该是Goroutine耗尽操作系统为Go程序分配的栈大小的话， 就从堆上分配）。

由于无限递归函数持续地调用自己，新的栈页最后就需要从堆上分配。堆的大小很快就会超过机器的可用物理内存空间，到那时，swapping会很快导致你的机器不可用。

Go程序可用的堆大小依赖于很多东西，包括机器的CPU架构和操作系统，但这通常是一个超出机器物理内存的值，因此机器很可能在程序耗尽它的堆空间之前就会频繁地swap。

对于Go 1.1，曾有强烈要求增大32位、64位平台上堆的最大值，但这在某种程度上恶化了这一问题，比如，你的机器不太可能有128GB<sup>3</sup>的物理内存。

最后提一下，关于这个问题有几个未解决的issue（[链接](https://code.google.com/p/go/issues/detail?id=4692)，[链接](https://code.google.com/p/go/issues/detail?id=2556)）, 目前还没找到一个解决方案能够不影响按常规编写的程序的性能。

注释

1. 也适用于方法（method），虽然方法是作为第一个参数为方法接受者（the method receiver）的函数来实现的，但在讨论Go语言中分段的（segmented）栈如何工作之时，并没有实际的区别。
2. 使用单词page并不意味着仅按固定的4096字节来分配，如果需要，runtime.morestack会分配一个更大的，倍数于一个页大小的空间。
3. 由于Go 1.1发布周期中一个后来的改变，64位的Windows平台仅允许32Gb大小的堆。

译者补充相关文章

1. [A trip down the (split) rabbithole](http://blog.nella.org/a-trip-down-the-split-rabbithole/)
2. [go语言中split stack(上)](http://runtime.diandian.com/post/2011-12-24/11488238)
3. [go语言中split stack(下)](http://runtime.diandian.com/post/2011-12-26/10119542)
4. [go在stack上干了神马？](http://mikespook.com/2011/03/go%E5%9C%A8stack%E4%B8%8A%E5%B9%B2%E4%BA%86%E7%A5%9E%E9%A9%AC%EF%BC%9F/)
