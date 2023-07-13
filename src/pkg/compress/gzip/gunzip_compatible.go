package gzip

import (
	"hash/crc32"
	"io"
)

// 	@compatible: 此方法在 v1.3 版本添加
//
// Reset discards the Reader z's state and makes it equivalent to the
// result of its original state from NewReader, but reading from r instead.
// This permits reusing a Reader rather than allocating a new one.
func (z *Reader) Reset(r io.Reader) error {
	z.r = makeReader(r)
	if z.digest == nil {
		z.digest = crc32.NewIEEE()
	} else {
		z.digest.Reset()
	}
	z.size = 0
	z.err = nil
	return z.readHeader(true)
}
