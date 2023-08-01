package net

import "io"

// 	@compatible: 此函数在 v1.8 版本添加
//
// buffersWriter is the interface implemented by Conns that support a
// "writev"-like batch write optimization.
// writeBuffers should fully consume and write all chunks from the
// provided Buffers, else it should report a non-nil error.
type buffersWriter interface {
	writeBuffers(*Buffers) (int64, error)
}

var testHookDidWritev = func(wrote int) {}

// 	@compatible: 此类型在 v1.8 版本添加
//
// Buffers contains zero or more runs of bytes to write.
//
// On certain machines, for certain types of connections, this is
// optimized into an OS-specific batch write operation (such as
// "writev").
type Buffers [][]byte

var (
	_ io.WriterTo = (*Buffers)(nil)
	_ io.Reader   = (*Buffers)(nil)
)

// 	@compatible: 此方法在 v1.8 版本添加
//
func (v *Buffers) WriteTo(w io.Writer) (n int64, err error) {
	if wv, ok := w.(buffersWriter); ok {
		return wv.writeBuffers(v)
	}
	for _, b := range *v {
		nb, err := w.Write(b)
		n += int64(nb)
		if err != nil {
			v.consume(n)
			return n, err
		}
	}
	v.consume(n)
	return n, nil
}

// 	@compatible: 此方法在 v1.8 版本添加
//
func (v *Buffers) Read(p []byte) (n int, err error) {
	for len(p) > 0 && len(*v) > 0 {
		n0 := copy(p, (*v)[0])
		v.consume(int64(n0))
		p = p[n0:]
		n += n0
	}
	if len(*v) == 0 {
		err = io.EOF
	}
	return
}

// 	@compatible: 此方法在 v1.8 版本添加
//
func (v *Buffers) consume(n int64) {
	for len(*v) > 0 {
		ln0 := int64(len((*v)[0]))
		if ln0 > n {
			(*v)[0] = (*v)[0][n:]
			return
		}
		n -= ln0
		*v = (*v)[1:]
	}
}
