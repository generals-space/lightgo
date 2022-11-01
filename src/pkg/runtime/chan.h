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

////////////////////////////////////////////////////////////////////////////////
// 下述3个函数: dequeue, enqueue, racesync, 同时在 chan.c 和 select.c 中使用.
// 虽然在 chan.h 中有ta们的声明, 但是由于是静态类型, 需要在所有用到ta们的 .c 文件中定义.
// 这是C语言的特性规定, 以后可以考虑将这3个函数修改为非静态函数.
// 相关资料见: https://stackoverflow.com/questions/42056160/static-functions-declared-in-c-header-files

static	void	dequeueg(WaitQ*);
static	SudoG*	dequeue(WaitQ*);
static	void	enqueue(WaitQ*, SudoG*);
static	void	destroychan(Hchan*);
static	void	racesync(Hchan*, SudoG*);

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
