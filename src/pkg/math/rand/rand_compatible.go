package rand

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
// 	@compatible: 此接口在 v1.8 版本添加
//
// A Source64 is a Source that can also generate
// uniformly-distributed pseudo-random uint64 values in
// the range [0, 1<<64) directly.
// If a Rand r's underlying Source s implements Source64,
// then r.Uint64 returns the result of one call to s.Uint64
// instead of making two calls to s.Int63.
type Source64 interface {
	Source
	Uint64() uint64
}

// 	@compatible: 此方法在 v1.8 版本添加
//
// Uint64 returns a pseudo-random 64-bit value as a uint64.
func (r *Rand) Uint64() uint64 {
	if r.s64 != nil {
		return r.s64.Uint64()
	}
	return uint64(r.Int63())>>31 | uint64(r.Int63())<<32
}
////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此方法在 v1.10 版本添加
//
// int31n returns, as an int32, a non-negative pseudo-random number in [0,n).
// n must be > 0, but int31n does not check this; the caller must ensure it.
// int31n exists because Int31n is inefficient, but Go 1 compatibility
// requires that the stream of values produced by math/rand remain unchanged.
// int31n can thus only be used internally, by newly introduced APIs.
//
// For implementation details, see:
// https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
// https://lemire.me/blog/2016/06/30/fast-random-shuffling
func (r *Rand) int31n(n int32) int32 {
	v := r.Uint32()
	prod := uint64(v) * uint64(n)
	low := uint32(prod)
	if low < uint32(n) {
		thresh := uint32(-n) % uint32(n)
		for low < thresh {
			v = r.Uint32()
			prod = uint64(v) * uint64(n)
			low = uint32(prod)
		}
	}
	return int32(prod >> 32)
}

// 	@compatible: 此方法在 v1.10 版本添加
//
// Shuffle pseudo-randomizes the order of elements.
// n is the number of elements. Shuffle panics if n < 0.
// swap swaps the elements with indexes i and j.
func (r *Rand) Shuffle(n int, swap func(i, j int)) {
	if n < 0 {
		panic("invalid argument to Shuffle")
	}

	// Fisher-Yates shuffle: https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
	// Shuffle really ought not be called with n that doesn't fit in 32 bits.
	// Not only will it take a very long time, but with 2³¹! possible permutations,
	// there's no way that any PRNG can have a big enough internal state to
	// generate even a minuscule percentage of the possible permutations.
	// Nevertheless, the right API signature accepts an int n, so handle it as best we can.
	i := n - 1
	for ; i > 1<<31-1-1; i-- {
		j := int(r.Int63n(int64(i + 1)))
		swap(i, j)
	}
	for ; i > 0; i-- {
		j := int(r.int31n(int32(i + 1)))
		swap(i, j)
	}
}

// 	@compatible: 此方法在 v1.10 版本添加
//
// Shuffle pseudo-randomizes the order of elements using the default Source.
// n is the number of elements. Shuffle panics if n < 0.
// swap swaps the elements with indexes i and j.
func Shuffle(n int, swap func(i, j int)) { globalRand.Shuffle(n, swap) }
