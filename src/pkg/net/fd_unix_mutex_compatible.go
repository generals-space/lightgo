// Copyright 2013 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package net

// 	@compatible: 该文件取自
// [golang v1.9 fd_mutex.go](https://github.com/golang/go/tree/go1.9/src/internal/poll/fd_mutex.go)
//

// incref adds a reference to fd.
// It returns an error when fd cannot be used.
func (fd *FD) incref() error {
	// 	@compatible: incref() -> Incref()
	//
	// if !fd.fdmu.incref() {
	if !fd.fdmu.Incref() {
		// return errClosing(fd.isFile)
		return errClosing
	}
	return nil
}

// decref removes a reference from fd.
// It also closes fd when the state of fd is set to closed and there
// is no remaining reference.
func (fd *FD) decref() error {
	// 	@compatible: decref() -> Decref()
	//
	// if fd.fdmu.decref() {
	if fd.fdmu.Decref() {
		return fd.destroy()
	}
	return nil
}

// readLock adds a reference to fd and locks fd for reading.
// It returns an error when fd cannot be used for reading.
func (fd *FD) readLock() error {
	// 	@compatible: rwlock() -> RWLock()
	//
	// if !fd.fdmu.rwlock(true) {
	if !fd.fdmu.RWLock(true) {
		// return errClosing(fd.isFile)
		return errClosing
	}
	return nil
}

// readUnlock removes a reference from fd and unlocks fd for reading.
// It also closes fd when the state of fd is set to closed and there
// is no remaining reference.
func (fd *FD) readUnlock() {
	// 	@compatible: rwunlock() -> RWUnlock()
	//
	// if fd.fdmu.rwunlock(true) {
	if fd.fdmu.RWUnlock(true) {
		fd.destroy()
	}
}

// writeLock adds a reference to fd and locks fd for writing.
// It returns an error when fd cannot be used for writing.
func (fd *FD) writeLock() error {
	// 	@compatible: rwlock() -> RWLock()
	//
	// if !fd.fdmu.rwlock(false) {
	if !fd.fdmu.RWLock(false) {
		// return errClosing(fd.isFile)
		return errClosing
	}
	return nil
}

// writeUnlock removes a reference from fd and unlocks fd for writing.
// It also closes fd when the state of fd is set to closed and there
// is no remaining reference.
func (fd *FD) writeUnlock() {
	// 	@compatible: rwunlock() -> RWUnlock()
	//
	// if fd.fdmu.rwunlock(false) {
	if fd.fdmu.RWUnlock(false) {
		fd.destroy()
	}
}
