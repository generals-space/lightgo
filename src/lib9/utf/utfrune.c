/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include <string.h>
#include "utf.h"
#include "utfdef.h"

/* const - removed for go code */

// utfrune 应该是寻找 s 字符串中, 首次出现字符 c 的位置
// 不是索引, 而是起始地址, 所以有可能返回 nil
//
// 	@return: 0 应该就是 nil(非法地址), 没找到.
char* utfrune(const char *s, Rune c)
{
	long c1;
	Rune r;
	int n;

	/* not part of utf sequence */
	if(c < Runesync) {
		return strchr(s, (char)c);
	}

	for(;;) {
		c1 = *(uchar*)s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0) {
				return 0;
			}
			if(c1 == c) {
				return (char*)s;
			}
			s++;
			continue;
		}
		n = chartorune(&r, s);
		if(r == c) {
			return (char*)s;
		}
		s += n;
	}
	return 0;
}
