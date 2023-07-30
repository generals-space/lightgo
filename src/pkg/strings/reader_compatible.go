package strings

// 	@compatible: addAt v1.7
//
// Size returns the original length of the underlying string.
// Size is the number of bytes available for reading via ReadAt.
// The returned value is always the same and is not affected by calls
// to any other method.
func (r *Reader) Size() int64 { return int64(len(r.s)) }

// 	@compatible: addAt v1.7
//
// Reset resets the Reader to be reading from s.
func (r *Reader) Reset(s string) { *r = Reader{s, 0, -1} }
