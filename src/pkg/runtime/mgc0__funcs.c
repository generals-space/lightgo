#include "runtime.h"
#include "mgc0__funcs.h"

// 获取 GOGC 环境变量, 用于主调函数设置 gcpercent 全局变量.
//
// caller: 
// 	1. runtime·gc()
// 	2. runtime∕debug·setGCPercent()
//
int32 readgogc(void)
{
	byte *p;

	p = runtime·getenv("GOGC");
	if(p == nil || p[0] == '\0'){
		return 100;
	} 
	if(runtime·strcmp(p, (byte*)"off") == 0) {
		return -1;
	}
	return runtime·atoi(p);
}
