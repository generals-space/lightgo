// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Malloc small size classes.
//
// See malloc.h for overview.
//
// The size classes are chosen so that rounding an allocation
// request up to the next size class wastes at most 12.5% (1.125x).
//
// Each size class has its own page count that gets allocated
// and chopped up when new objects of the size class are needed.
// That page count is chosen so that chopping up the run of
// pages into objects of the given size wastes at most 12.5% (1.125x)
// of the memory.  It is not necessary that the cutoff here be
// the same as above.
//
// The two sources of waste multiply, so the worst possible case
// for the above constraints would be that allocations of some
// size might have a 26.6% (1.266x) overhead.
// In practice, only one of the wastes comes into play for a
// given size (sizes < 512 waste mainly on the round-up,
// sizes > 512 waste mainly on the page chopping).
//
// TODO(rsc): Compute max waste for any given size.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "malloc.h"

int32 runtime·class_to_size[NumSizeClasses];
int32 runtime·class_to_allocnpages[NumSizeClasses];

// The SizeToClass lookup is implemented using two arrays,
// one mapping sizes <= 1024 to their class and one mapping sizes >= 1024 and <= MaxSmallSize to their class.
// All objects are 8-aligned, so the first array is indexed by the size divided by 8 (rounded up). 
// Objects >= 1024 bytes are 128-aligned, 
// so the second array is indexed by the size divided by 128 (rounded up). 
// The arrays are filled in by InitSizes.
// size_to_classXXX相对于class_to_size, 用于反查.
int8 runtime·size_to_class8[1024/8 + 1];
int8 runtime·size_to_class128[(MaxSmallSize-1024)/128 + 1];

static int32
SizeToClass(int32 size)
{
	if(size > MaxSmallSize)
		runtime·throw("SizeToClass - invalid size");
	if(size > 1024-8)
		return runtime·size_to_class128[(size-1024+127) >> 7];
	return runtime·size_to_class8[(size+7)>>3];
}

void
runtime·InitSizes(void)
{
	int32 align, sizeclass, size, nextsize, n;
	uint32 i;
	uintptr allocsize, npages;

	// Initialize the runtime·class_to_size table (and choose class sizes in the process).
	// 构建runtime·class_to_size映射表, 键为1到61的等级值, 值为不同等级的span中存储的对象的大小.
	// 在malloc.h中定义了NumSizeClasses = 61, 表示span的size分级有61级, 也定义了从8 byte到最大的小对象size为32k,
	// 但是从8B -> 32KB划分为61份要怎么分...? 平均分肯定不行的
	// 这里的for循环代表了划分的流程, 可以看到ta是按照size大小+align步长一点一点加上去的. 
	// ...这个流程分析起来有点复杂, 不如打印一下日志更清晰.
	runtime·class_to_size[0] = 0;
	sizeclass = 1;	// 0 means no class 从第1级开始
	align = 8; 
	// runtime·printf("+++ init runtime·class_to_size\n");
	for(size = align; size <= MaxSmallSize; size += align) {
		if((size&(size-1)) == 0) {	// bump alignment once in a while
			if(size >= 2048) 		align = 256;
			else if(size >= 128) 	align = size / 8;
			else if(size >= 16)		align = 16;	// required for x86 SSE instructions, if we want to use them
		}
		if((align&(align-1)) != 0) runtime·throw("InitSizes - bug");

		// Make the allocnpages big enough that the leftover is less than 1/8 of the total,
		// so wasted space is at most 12.5%.
		// 因为span的空间大小是按page(1 page = 4k)算的, 但是这个for循环中对于对象的size并不完全按2的指数倍增长(8, 16, 32, 64...)
		// 对象的size可以是48, 80等, 有可能无法被整除, 所以会存在剩余的空间浪费.
		// 下面的语句就是为了尽量减少这种浪费, 或者说是为尽量减少浪费空间在当前size等级所分配的总空间的占比.
		// while的循环条件就是, 所有对象占满时的剩余空间(allocsize % size)大于分配总空间的8分之一(allocsize / 8), 即可满足, 这样最多浪费12.5%.
		// ...md这个值也不小啊.
		// 以目标对象size=1152为例(sizeclass为30), 
		// 如果这个等级分配1页, 那么这个span只能容纳3个对象, 剩余640 byte的空间浪费, 比例占15.625%;
		// 如果分配2页, 那么这个span可以容纳7个1152 byte的对象, 然后剩余128 byte, 比例为1.5625%.
		// ...我call, 那么就是有时span只能容纳很少的对象, 不够了怎么办? 重新创建span吗?
		allocsize = PageSize;
		while(allocsize%size > allocsize/8) allocsize += PageSize;
		npages = allocsize >> PageShift;

		// 打印size与align, 会在运行目标程序时输出(编译时也会输出), 需要重新编译
		// runtime·printf("+++ sizeclass: %d, size: %d, align: %d, pages: %d\n", sizeclass, size, align, (int32)npages);

		// If the previous sizeclass chose the same allocation size and fit the same number of objects into the page, 
		// we might as well use just this size instead of having two different sizes.
		// 如果在当前循环中, 与前面的size等级拥有同样span大小(npages的值相同, 表示页的个数相同), 且其中可以容纳的对象的个数(即allocsize/size值)也相同,
		// 就没有必要保留当前的size等级了.
		// 比如, sizeclass 22, size对象大小为416 byte, 分配的总空间为4k, 可容纳9个对象, 空间浪费8.59%; 
		// 而当时align的为32(因为 16 <= size <= 128, 当前for循环起始的if块中有提到), 下一个size对象大小为448, 分配的总空间为4k, 也可容纳9个对象, 空间浪费1.5625%.
		// 那么sizeclass 23就可以不用保留size 448的记录了, 分配对象时直接使用size 22的.
		if(sizeclass > 1
		&& npages == runtime·class_to_allocnpages[sizeclass-1]
		&& allocsize/size == allocsize/runtime·class_to_size[sizeclass-1]) {
			runtime·class_to_size[sizeclass-1] = size;
			// 在这里, npages可以正常输出, 但allocsize的值为0, 日志信息中表示其类型为VULONG, 不太明白为什么.
			// runtime·printf("++++++ npages: %d, allocsize: %d\n", npages, (int64)allocsize);
			continue;
		}

		runtime·class_to_allocnpages[sizeclass] = npages;
		runtime·class_to_size[sizeclass] = size;
		sizeclass++;
	}
	// 核对一下sizeclass的等级值是否与预设的等级相同...
	// 我觉得NumSizeClasses应该是先执行上面的算法, 得到61后再确定的吧...
	// 因为算法流程中对align步长的划分看起来很随机, 无法预估等级数量.
	if(sizeclass != NumSizeClasses) {
		runtime·printf("sizeclass=%d NumSizeClasses=%d\n", sizeclass, NumSizeClasses);
		runtime·throw("InitSizes - bad NumSizeClasses");
	}

	// Initialize the size_to_class tables.
	// 初始化size_to_class反查表, 这应该才是runtime真正需要的吧, 
	// 毕竟肯定是先知道目标对象的大小, 才能确定需要将其分配到哪个等级的span中.
	// 这个for循环流程就简单很多了, 反查表有两个, size_to_class8和size_to_class128.
	// 对小于1024 byte的对象, size -> class的映射比较细, 每8 bytes分一段.
	// 比如class 7中存放的对象size为96 bytes, class 8的为112 bytes.
	// 那么100, 108 bytes的对象应该放在哪呢? 肯定也得是class 8, 因为class 7放不下, class 9会造成浪费.
	// 对于大于1024的对象, nextsize步进长度变成了128, 看一下span等级表会发现, 大于1024 bytes的class等级之间,
	// size的差值都是大于128的, 这样可以减少第2个for循环的次数...我猜.
	nextsize = 0;
	for (sizeclass = 1; sizeclass < NumSizeClasses; sizeclass++) {
		// 在nextsize < 1024时, 步进长度为8
		for(; nextsize < 1024 && nextsize <= runtime·class_to_size[sizeclass]; nextsize+=8)
			runtime·size_to_class8[nextsize/8] = sizeclass;
		// 在nextsize >= 1024时, 步进长度为128
		if(nextsize >= 1024)
			for(; nextsize <= runtime·class_to_size[sizeclass]; nextsize += 128)
				runtime·size_to_class128[(nextsize-1024)/128] = sizeclass;
	}

	// Double-check SizeToClass.
	if(0) {
		// ...call, n的步进长度就是1, 从0加到32k吗
		for(n=0; n < MaxSmallSize; n++) {
			sizeclass = SizeToClass(n);
			// runtime·class_to_size[sizeclass] < n表示通过SizeToClass得到的class等级中的size无法容纳大小为n的对象, 
			// 这肯定是不行的
			if(sizeclass < 1 || sizeclass >= NumSizeClasses || runtime·class_to_size[sizeclass] < n) {
				runtime·printf("size=%d sizeclass=%d runtime·class_to_size=%d\n", n, sizeclass, runtime·class_to_size[sizeclass]);
				runtime·printf("incorrect SizeToClass");
				goto dump;
			}
			//
			// runtime·class_to_size[sizeclass-1] >= n则表示SizeToClass得到的class等级可以容纳大小为n的对象,
			// 但是明明前一个class等级也可以容纳, 这也是不允许的.
			// 比如class 61容纳的对象是32k的, 虽然可以存储8 bytes的对象, 但是可能造成很大的空间浪费, 不合理.
			if(sizeclass > 1 && runtime·class_to_size[sizeclass-1] >= n) {
				runtime·printf("size=%d sizeclass=%d runtime·class_to_size=%d\n", n, sizeclass, runtime·class_to_size[sizeclass]);
				runtime·printf("SizeToClass too big");
				goto dump;
			}
		}
	}

	// Copy out for statistics table.
	for(i=0; i<nelem(runtime·class_to_size); i++)
		mstats.by_size[i].size = runtime·class_to_size[i];
	return;

dump:
	if(1){
		runtime·printf("NumSizeClasses=%d\n", NumSizeClasses);
		runtime·printf("runtime·class_to_size:");
		for(sizeclass=0; sizeclass<NumSizeClasses; sizeclass++)
			runtime·printf(" %d", runtime·class_to_size[sizeclass]);
		runtime·printf("\n\n");
		runtime·printf("size_to_class8:");
		for(i=0; i<nelem(runtime·size_to_class8); i++)
			runtime·printf(" %d=>%d(%d)\n", i*8, runtime·size_to_class8[i],
				runtime·class_to_size[runtime·size_to_class8[i]]);
		runtime·printf("\n");
		runtime·printf("size_to_class128:");
		for(i=0; i<nelem(runtime·size_to_class128); i++)
			runtime·printf(" %d=>%d(%d)\n", i*128, runtime·size_to_class128[i],
				runtime·class_to_size[runtime·size_to_class128[i]]);
		runtime·printf("\n");
	}
	runtime·throw("InitSizes failed");
}
