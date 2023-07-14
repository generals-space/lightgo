package rand

// 	@compatible: 该方法在 v1.6 版本初次添加
//
// Read generates len(p) random bytes from the default Source and
// writes them into p. It always returns len(p) and a nil error.
func Read(p []byte) (n int, err error) { return globalRand.Read(p) }

// 	@compatible: 该方法在 v1.6 版本初次添加
//
// Read generates len(p) random bytes and writes them into p. It
// always returns len(p) and a nil error.
func (r *Rand) Read(p []byte) (n int, err error) {
	for i := 0; i < len(p); i += 7 {
		val := r.src.Int63()
		for j := 0; i+j < len(p) && j < 7; j++ {
			p[i+j] = byte(val)
			val >>= 8
		}
	}
	return len(p), nil
}
