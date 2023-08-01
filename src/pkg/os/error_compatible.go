package os

// 	@compatible 该错误在 v1.15 版本添加
//
// errDeadlineExceeded returns the value for os.ErrDeadlineExceeded.
// This error comes from the internal/poll package, which is also
// used by package net. Doing this this way ensures that the net
// package will return os.ErrDeadlineExceeded for an exceeded deadline,
// as documented by net.Conn.SetDeadline, without requiring any extra
// work in the net package and without requiring the internal/poll
// package to import os (which it can't, because that would be circular).
func errDeadlineExceeded() error { 
	// 	@compatibleNote
	// return poll.ErrDeadlineExceeded 
	return poll_ErrDeadlineExceeded 
}

// 	@compatible: 此结构在 v1.15 版本添加
// 	@compatibleNote: 因为 poll.ErrDeadlineExceeded 位于 internal/poll,
// 需要在编译时放在 os 包前编译, 这里将该结构体直接移动到这里.
//
// ErrDeadlineExceeded is returned for an expired deadline.
// This is exported by the os package as os.ErrDeadlineExceeded.
// var ErrDeadlineExceeded error = &DeadlineExceededError{}
var poll_ErrDeadlineExceeded error = &DeadlineExceededError{}

// DeadlineExceededError is returned for an expired deadline.
type DeadlineExceededError struct{}

// Implement the net.Error interface.
// The string is "i/o timeout" because that is what was returned
// by earlier Go versions. Changing it may break programs that
// match on error strings.
func (e *DeadlineExceededError) Error() string   { return "i/o timeout" }
func (e *DeadlineExceededError) Timeout() bool   { return true }
func (e *DeadlineExceededError) Temporary() bool { return true }
