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
// Buffer follows Hchan immediately in memory.
// chanbuf(c, i) is pointer to the i'th slot in the buffer.
#define chanbuf(c, i) ((byte*)((c)+1)+(uintptr)(c)->elemsize*(i))

// static 函数只能用于源文件自身, 要实现跨文件的函数相互调用, 则需要将被调函数的 static 标记移除.

SudoG*	dequeue(WaitQ*);
void	enqueue(WaitQ*, SudoG*);
void	racesync(Hchan*, SudoG*);

static	void	dequeueg(WaitQ*);
static	void	destroychan(Hchan*);

enum
{
	debug = 0,

	// Scase.kind
	CaseRecv,
	CaseSend,
	CaseDefault,
};

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
	// dataqsiz 表示该 channel 可缓冲的元素数量, 与qcount相比, 应该是cap与len的区别.
	// 如果 dataqsiz > 0 则表示为异步队列, 否则为同步队列.
	// 注意, channel对象无法像slice一样扩容, 所以初始化后这个值是无法改变的.
	//
	// size of the circular q
	uintgo	dataqsiz;
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
	WaitQ	recvq;			// list of recv waiters
	WaitQ	sendq;			// list of send waiters
	Lock;
};
