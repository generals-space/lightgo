// Copyright 2013 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build linux

#include "runtime.h"
#include "defs_GOOS_GOARCH.h"

// 如下函数都是由汇编实现的
int32	runtime·epollcreate(int32 size);
int32	runtime·epollcreate1(int32 flags);
int32	runtime·epollctl(int32 epfd, int32 op, int32 fd, EpollEvent *ev);
// epoll_wait, 等待epoll文件描述符上的I/O事件.
//
// 这是一段汇编代码, 底层直接是 232 号系统调用 epoll_wait (我觉得可以用 c 语言直接写???)
int32	runtime·epollwait(int32 epfd, EpollEvent *ev, int32 nev, int32 timeout);
void	runtime·closeonexec(int32 fd);

// epfd 是全局变量, 难道整个进程只需要1个??? 
// nginx中也是这么用的么???
// epoll descriptor
static int32 epfd = -1;

// caller: 
// 	1. netpoll.goc -> runtime_pollServerInit()
//
// runtime_pollServerInit() 由 net/fd_poll_runtime.go -> Init() 调用
void runtime·netpollinit(void)
{
	epfd = runtime·epollcreate1(EPOLL_CLOEXEC);
	if(epfd >= 0) {
		return;
	}
	epfd = runtime·epollcreate(1024);
	if(epfd >= 0) {
		runtime·closeonexec(epfd);
		return;
	}
	runtime·printf("netpollinit: failed to create descriptor (%d)\n", -epfd);
	runtime·throw("netpollinit: failed to create descriptor");
}

int32 runtime·netpollopen(uintptr fd, PollDesc *pd)
{
	EpollEvent ev;
	int32 res;

	ev.events = EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
	ev.data = (uint64)pd;
	res = runtime·epollctl(epfd, EPOLL_CTL_ADD, (int32)fd, &ev);
	return -res;
}

int32 runtime·netpollclose(uintptr fd)
{
	EpollEvent ev;
	int32 res;

	res = runtime·epollctl(epfd, EPOLL_CTL_DEL, (int32)fd, &ev);
	return -res;
}

// 轮询查找已经准备好的网络连接, 返回状态变为 runnable 的 goroutine 列表(非阻塞).
//
// 注意: 只对 socket 场景中有效, 并不是所有陷入系统调用的 g 都能用 epoll 方法通知.
//
// caller:
// 	1. src/pkg/runtime/proc.c -> sysmon() 该线程会定时调用本函数, 查看是否有因陷入
// 	recv/send 系统调用而休眠的 g 协程是否因为有 IO 事件而被唤醒了, 将ta们加入到全局队列中.
//
// polls for ready network connections
// returns list of goroutines that become runnable
G* runtime·netpoll(bool block)
{
	static int32 lasterr;
	EpollEvent events[128], *ev;
	int32 n, i, waitms, mode;
	G *gp;

	if(epfd == -1) {
		return nil;
	} 
	waitms = -1;
	if(!block) {
		waitms = 0;
	}

retry:
	// epoll_wait, 等待epoll文件描述符上的I/O事件.
	// waitms 为等待的毫秒数, 值为0时立刻返回, 为-1时则无限期等待下去(即阻塞).
	n = runtime·epollwait(epfd, events, nelem(events), waitms);
	if(n < 0) {
		if(n != -EINTR && n != lasterr) {
			lasterr = n;
			runtime·printf("runtime: epollwait on fd %d failed with %d\n", epfd, -n);
		}
		goto retry;
	}
	gp = nil;
	for(i = 0; i < n; i++) {
		ev = &events[i];
		if(ev->events == 0) {
			continue;
		} 

		mode = 0;
		if(ev->events & (EPOLLIN|EPOLLRDHUP|EPOLLHUP|EPOLLERR)) {
			mode += 'r';
		}
		if(ev->events & (EPOLLOUT|EPOLLHUP|EPOLLERR)) {
			mode += 'w';
		}

		if(mode) {
			// 该函数在同目录下的 netpool.goc 文件中定义
			runtime·netpollready(&gp, (void*)ev->data, mode);
		}
	}
	if(block && gp == nil){
		goto retry;
	} 
	return gp;
}
