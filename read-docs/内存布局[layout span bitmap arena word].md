# 内存布局[layout span bitmap arena word]

<!--
<!key!>: {32a3d702-70db-4cae-852b-5c12ce491afc}
-->

常用常量值

```c++
sizeof(void*) = 8
word = 4Bits
```

## 基础布局

全局变量`runtime·mheap`初始化流程, 见: [runtime·mallocinit()](/src/pkg/runtime/malloc.goc)

```
                        arena_used(初始arena的使用为0, 所以其地址与 arena_start 在同一位置)
 span     bitmap        arena_start                arena_end
   ↓        ↓                ↓                         ↓
   +--------|----------------+-------------------------+
   |  span  |     bitmap     |          arena          |
   +--------|----------------+-------------------------+
```

在 64-bit 系统中，实际保留地址 256MB spans + 8GB bitmap + 128GB arena

## bitmap 与 arena 区域的映射.1

```
                arena_start
                    ↓
         <- bitmap  |  arena ->
+----+----+----+----|--------------------------+--------------------------+------------------+
| x4 | x4 | x4 | x4 |            x64           |            x64           |        ...       |
+----+----+----+----|--------------------------+--------------------------+------------------+
             |   └────────────────┘                          ↑
             └───────────────────────────────────────────────┘
```

每个 word 为 4Bits, 上图中`x4`表示1个 word(字), x64 则表示一个指针(4Bits * 16, 正是64位主机的指针大小, 8Bits * 8).

所以说, **arena 区域中的每个指针, 都可以用 bitmap 区域中的一个 word 来描述**.

这种映射方式, 使得在 bitmap 中对 arena 进行描述的同时, 不需要额外保存 arena 中的指针地址, 直接通过公式就能计算出来.

另外, 注意ta们的映射方向, 以 arena_start 为分界, arena 中**向前**每增加一个指针, 则需要**向后**减去一个字的大小, 即**bitmap 增长的方向与 arena 相反**.

## bitmap 与 arena 区域的映射.2

**但是**, 上述布局还不够严谨.

由于在编程层面, 实际还是以 pointer(指针) 为操作对象, 没有一种变量类型是 word 的. 所以实际上在 bitmap 区域, 信息仍然是按 pointer 进行计算与存储的. 

所以正常来说, 应该是 bitmap 区域中, 每个 pointer 对应 arena 区域中的 16个 pointer. 

然而对于 arena 中的某个指针, bitmap 中对应的 word 却并不是连续的 4 Bits, 而是被分在4个"bit块"中, 每个"bit块"占 2Bytes(8Bits * 2).

- bitSpecial(高地址)
- bitMarked
- bitNoScan/bitBlockBoundary
- bitAllocated(低地址)

下图 bitmap 区域中为单个指针内容的详细表示, 每个"bit块"从低到高为`0`到`F`, 分别对应 arena 区域中的第`0`到第`F`个指针.

```
                                      bitBlockBoundary
    bitSpecial    |    bitMarked    |    bitNoScan    |   bitAllocated
|F*******|****3210|F*******|****3210|F*******|****3210|F*******|****3210|                                    bitmap
                 |                 |        ┌────────┘                 |
                 |                 |        ┌──────────────────────────┘
                 |                 └────────┐
                 └──────────────────────────┐
                                            |
 F                 F                 F      |          F
 |                 |                 |      |          └──────────────────────────────────────┐
 |                 |                 └──────|─────────────────────────────────────────────────┐
 |                 └────────────────────────|─────────────────────────────────────────────────┐
 └──────────────────────────────────────────|─────────────────────────────────────────────────┐
                                            ↓                                                 ↓
                                            0         1         2         3         4   ...   F
                                            +---------+---------+---------+---------+---------+---------+
                                            |   x64   |   x64   |   x64   |   x64   |   ...   |   x64   |    arena
                                            +---------+---------+---------+---------+---------+---------+
                                            |
                                       arena_start
```

以从 arena_start 开始的第0个指针为例. 描述ta的 word 是由 bitmap 区域中, 从 arena_start 向后的第0个指针中, 每个"bit块"的第`0`位组成的. 而从 arena_start 中的第15个指针(F), 则由该指针中的第`F`位组成.

## 映射的计算方法

好了, 有了这个映射的概念, 接下来就是计算方法. 我们找到源码中常见的遍历方法, 可以在如下文件中找到ta们的应用

1. [Bits in per-word bitmap](/src/pkg/runtime/mgc0.c): 英文注释中的计算方法;
2. [markonly marks an object](/src/pkg/runtime/mgc0.c): gc 流程中, "标记"过程的实现;

```c++
// `p`为某个对象在 arena 区域所占空间的起始地址.
// `off`为偏移量, 是一个整型, 可以理解为`p`到`arena_start`间隔的指针数量.
//
// 注意: 指针-指针, 得到的数值应该乘以8, 才能真正表示两个指针间的地址差.
off = p - (uintptr*)mheap.arena_start;

// `b` 是计算映射在 bitmap 区域中的位置, 是一个指针.
//
// wordsPerBitmapWord 为 16, 由于 bitmap 中一个指针可以描述 arena 中的 16 个指针,
// arena 中的 0-F 个指针, 都通过 bitmap 中的第0个指针的不同 bit 位描述.
// 那么ta们的 off/wordsPerBitmapWord 都只能得到 0
b = (uintptr*)mheap.arena_start - off/wordsPerBitmapWord - 1;

// shift 值的范围为 [0,F], 表示 p 在ta所属的16个指针数组中的索引.
shift = off % wordsPerBitmapWord

// 将 bitmap 中相应指针指表示的内容, 向右移动, 最终得到
// |********|*******0|********|*******0|********|*******0|********|*******0|
// 即, 将 bitmap 指针中, 描述 p 指针的bit位, 都移动后4个"bit块"中最低位, 
// 这样, 就可以与 bitAllocated, bitNoScan 等bit位进行与操作比较了.
bits = *b >> shift;
```

好了, 这下一切都说得通了.
