参考文章

1. [C语言--指针加减](https://blog.csdn.net/zouluhendiao/article/details/121050230)
    - 指针不适合做加法运算，一般用于减法运算
    - 指针做减法运算时，**相减结果是两个指针之间的元素的数目，而不是两个指针之间相差的字节数**
2. [C语言：指针的加减](https://blog.csdn.net/yy_0733/article/details/81008286)
    - 指针+指针：非法

arena_start:    0xc210000000
arena_start +1: 0xc210000008
arena_start -1: 0xc20ffffff8

```c++
	// off 是起始地址 v 距 arena_start 处的偏移量, 是一个整数
	// word offset
	off = (uintptr*)v - (uintptr*)runtime·mheap.arena_start;
	// 这是计算映射在bitmap区域中的位置, b 是一个指针.
	//
	// 注意: 指针-指针, 得到的数值应该乘以8, 才能真正表示两个指针间的差值.
	// 如果将 arena_start 和 b 将 0xXXX 形式的16进制指针, 转换成10进制,
	// 那下面的公式其实应该为 arena_start - b = (off/wordsPerBitmapWord + 1) * 8
	// 假设 off 为 15360, 则 arena_start - b = (15360 / 16 + 1) * 8
	b = (uintptr*)runtime·mheap.arena_start - off/wordsPerBitmapWord - 1;
	// 注意: shift 是取模结果, 用来测整除的.
	shift = off % wordsPerBitmapWord;
```

指针相减一般是用来计算数组元素的间隔的, 比如一个数组`array = int[10]`, 则`&array[3] - &array[1] = 2`, 表示第3个元素与第1个元素之间隔了2个元素.

而至于实际上`&array[3]`与`&array[1]`的地址究竟相差多少, 则需要看这是一个什么类型的数组, 每个元素的大小是多少. 

如果是int数组, 则两者相差为 2 * 32; 如果是 char 数组, 则两者相差 2 * 8;
