// chan.h 文件是从 chan.c 中拆分出来, 与 select.c 共享的头文件部分

#include "runtime.h"
#include "race.h"
#include "type.h"

#define	MAXALIGN	8
#define	NOSELGEN	1

typedef	struct	WaitQ	WaitQ;
typedef	struct	SudoG	SudoG;

typedef	struct	Select	Select;
typedef	struct	Scase	Scase;

// chanbuf 应该是 chan 中实际存储数据的地方
//
// 	@param c: 目标 channel 的地址
// 	@param i: 目标 channel 中缓冲区的索引(即数据要写入哪个位置, 或是从哪个位置读出)
//
// Buffer follows Hchan immediately in memory.
// chanbuf(c, i) is pointer to the i'th slot in the buffer.
#define chanbuf(c, i) ((byte*)((c)+1)+(uintptr)(c)->elemsize*(i))

#define debug 0

// static 函数只能用于源文件自身, 要实现跨文件的函数相互调用, 则需要将被调函数的 static 标记移除.
// 下面3个函数在 chan.c 和 select.c 2个文件中都有使用, 所以需要移除`static`标记.

SudoG*	dequeue(WaitQ*);
void	enqueue(WaitQ*, SudoG*);
void	racesync(Hchan*, SudoG*);

static	void	dequeueg(WaitQ*);
static	void	destroychan(Hchan*);

struct	SudoG
{
	G*	g;		// g and selgen constitute
	uint32	selgen;		// a weak pointer to g
	// link 表示队列中的下一个 g 对象
	SudoG*	link;
	int64	releasetime;
	byte*	elem;		// data element
};

struct	WaitQ
{
	SudoG*	first;
	SudoG*	last;
};

// The garbage collector is assuming that Hchan can only contain pointers into the stack
// and cannot contain pointers into the heap.
struct	Hchan
{
	// channel 队列中的数据总量
	//
	// total data in the q
	uintgo	qcount;
	// dataqsiz 表示该 channel 可缓冲的元素数量, 与 qcount 相比, 应该是cap与len的区别.
	// 如果 dataqsiz > 0 则表示为异步队列, 否则为同步队列.
	// 注意, channel对象无法像slice一样扩容, 所以初始化后这个值是无法改变的.
	//
	// size of the circular q
	uintgo	dataqsiz;

	// 注意: channel 对象中没有保存实际存储数据的地方(即缓冲区), 
	// 缓冲区其实就在 channel 对象的后面, 在 make() 的时候为其就申请了空间.
	// 见 chanbuf() 函数.

	// 队列中的元素类型占用的字节数(即单个元素的大小)
	uint16	elemsize;
	// ensures proper alignment of the buffer that follows Hchan in memory
	uint16	pad;
	// closed 表示该 channel 是否被关闭了.
	bool	closed;
	Alg*	elemalg;		// interface for element type
	uintgo	sendx;			// send index
	uintgo	recvx;			// receive index
	// recvq/sendq 表示向该 channel 中等待接收/发送的 goroutine 队列.
	// 每有一处 <- chan, 或是 chan <-, 都会将该 goroutine 加到这两个队列中.
	// list of recv waiters
	WaitQ	recvq;
	// list of send waiters
	WaitQ	sendq;
	Lock;
};
