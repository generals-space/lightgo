// 由于 BufferList{} 使用了"#pragma dataflag NOPTR"注解(来自 textflag),
// 所有引用当前 .h 的 .c, 都需要提前引用 type.h, 否则编译会报错.
#include "../../cmd/ld/textflag.h"

// 常用的对象构造语法: (Obj){p, n, ti}
// p: 该对象实际存储数据的位置
// n: 该对象存储的数据的大小(bytes)
// ti: 该对象的类型信息
typedef struct Obj Obj;
struct Obj
{
	// p: 该对象实际存储数据的位置
	//
	// data pointer
	byte	*p;
	// n: 该对象存储的数据的大小(bytes)
	//
	// size of data in bytes
	uintptr	n;
	// ti: 该对象的类型信息
	//
	// type info
	uintptr	ti;
};

// The size of Workbuf is N*PageSize.
typedef struct Workbuf Workbuf;
struct Workbuf
{
#define SIZE (2*PageSize-sizeof(LFNode)-sizeof(uintptr))
	LFNode  node; // must be first
	// obj 数组中的实际长度
	uintptr nobj;
	// obj 数组
	Obj     obj[SIZE/sizeof(Obj) - 1];
	uint8   _padding[SIZE%sizeof(Obj) + sizeof(Obj)];
#undef SIZE
};

struct WORK {
	uint64	full;  // lock-free list of full blocks
	uint64	empty; // lock-free list of empty blocks

	// prevents false-sharing between full/empty and nproc/nwait
	byte	pad0[CacheLineSize];

	// 参与执行gc的协程数量, 其值由 runtime·gcprocs() 设置.
	// 取值为 min(GOMAXPROCS, ncpu, MaxGcproc), 其中
	// 1. GOMAXPROCS 由开发者通过环境变量设置;
	// 2. ncpu 为物理机实际 cpu 核数(在 runtime 启动时自动探测);
	// 3. MaxGcproc 
	// 参与执行gc的协程要运行的是 runtime·gchelper() 函数.
	uint32	nproc;
	volatile uint32	nwait;
	// 参考 alldone 成员的解释, 每个参与gc的协程在完成自己部分的标记清理工作后,
	// 都会对 ndone 进行加1的原子操作, 然后与 nproc-1 比较(减去的1是gc主协程),
	// 如果相等, 那么表示所以gc协程都已完成, 就唤醒在 alldone 处等待的主协程.
	volatile uint32	ndone;
	volatile uint32 debugmarkdone;
	// Note与Lock一样(...完全一样), 是分别基于futex/sema实现的锁吧
	// 在STW后, 实际执行gc操作的协程可能不只一个,
	// 数量大致在 [1-nproc) 之间, ta们并行执行gc操作.
	// gc主线程将在这个地址休眠, 直到所有gc协程全部完成标记清理工作后, 再继续执行.
	Note	alldone; 
	ParFor	*markfor;
	ParFor	*sweepfor;

	Lock;
	byte	*chunk;
	uintptr	nchunk;

	// 此数组下存储着 nroot 个 Obj 对象, 每个 obj 对象都是 arena 区域中的一个地址.
	// ta们是 gc 的"标记清除"流程中, 真正的目标.
	Obj	*roots; 
	// roots 数组中的成员数量
	uint32	nroot;
	uint32	rootcap; // nroot <= rootcap, 为roots可容纳的最大root数量.
};

// PtrTarget is a structure used by intermediate buffers.
// The intermediate buffers hold GC data before it is moved/flushed to
// the work buffer (Workbuf).
// The size of an intermediate buffer is very small, such as 32 or 64 elements.
typedef struct PtrTarget PtrTarget;
struct PtrTarget
{
	void *p;
	uintptr ti;
};

typedef struct BufferList BufferList;
struct BufferList
{
	PtrTarget 	ptrtarget[IntermediateBufferCapacity];
	Obj 		obj[IntermediateBufferCapacity];
	uint32 		busy;
	byte 		pad[CacheLineSize];
};
#pragma dataflag NOPTR

// 为执行gc操作的协程提供各自的缓存数组.
// 由于执行gc操作的协程不只一个, gc()->runtime·gcprocs() 可以得到执行gc的协程数量.
// 这里 bufferList 数组的容量 MaxGcproc 是绝对够用的.
BufferList bufferList[MaxGcproc];

Type *itabtype;

typedef struct Finalizer Finalizer;
struct Finalizer
{
	FuncVal *fn;
	void *arg;
	uintptr nret;
	Type *fint;
	PtrType *ot;
};

typedef struct FinBlock FinBlock;
struct FinBlock
{
	FinBlock *alllink;
	FinBlock *next;
	int32 cnt;
	int32 cap;
	Finalizer fin[1];
};

// 用来执行 finalizer 的协程 ~~链表~~, 不对, 好像只有一个
G *fing; 
// 等待执行的finalizer链表
//
// list of finalizers that are to be executed
FinBlock *finq; 
FinBlock *finc; // cache of free blocks
FinBlock *allfin; // list of all blocks
Lock finlock;
int32 fingwait;

void addroots(void);
bool markonly(void *obj);
