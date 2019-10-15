// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

enum {
	thechar = '6',
	BigEndian = 0,
	// CPU 高速缓存线路
	CacheLineSize = 64,
	RuntimeGogoBytes = 64,
	// PCQuantum 应该是每个 pc 变量占用空间的大小吧
	// 这里是1个指针.
	PCQuantum = 1
};
