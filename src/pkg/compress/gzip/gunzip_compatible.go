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

// 	@compatible: 此方法在 v1.4 版本添加
//
// Multistream controls whether the reader supports multistream files.
//
// If enabled (the default), the Reader expects the input to be a sequence
// of individually gzipped data streams, each with its own header and
// trailer, ending at EOF. The effect is that the concatenation of a sequence
// of gzipped files is treated as equivalent to the gzip of the concatenation
// of the sequence. This is standard behavior for gzip readers.
//
// Calling Multistream(false) disables this behavior; disabling the behavior
// can be useful when reading file formats that distinguish individual gzip
// data streams or mix gzip data streams with other data streams.
// In this mode, when the Reader reaches the end of the data stream,
// Read returns io.EOF. If the underlying reader implements io.ByteReader,
// it will be left positioned just after the gzip stream.
// To start the next stream, call z.Reset(r) followed by z.Multistream(false).
// If there is no next stream, z.Reset(r) will return io.EOF.
func (z *Reader) Multistream(ok bool) {
	z.multistream = ok
}
