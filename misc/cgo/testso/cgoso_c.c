// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build ignore

extern void goCallback(void);
void setCallback(void *f) { (void)f; }

void sofunc(void)
{
	goCallback();
}
