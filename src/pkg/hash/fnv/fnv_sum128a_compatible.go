package fnv

import "hash"

type sum128a [2]uint64

const (
	offset128Lower  = 0x62b821756295c58d
	offset128Higher = 0x6c62272e07bb0142

	prime128Lower = 0x13b
	prime128Shift = 24
)

func (s *sum128a) Reset() { s[0] = offset128Higher; s[1] = offset128Lower }

func (s *sum128a) Write(data []byte) (int, error) {
	for _, c := range data {
		s[1] ^= uint64(c)
		// Compute the multiplication in 4 parts to simplify carrying
		s1l := (s[1] & 0xffffffff) * prime128Lower
		s1h := (s[1] >> 32) * prime128Lower
		s0l := (s[0]&0xffffffff)*prime128Lower + (s[1]&0xffffffff)<<prime128Shift
		s0h := (s[0]>>32)*prime128Lower + (s[1]>>32)<<prime128Shift
		// Carries
		s1h += s1l >> 32
		s0l += s1h >> 32
		s0h += s0l >> 32
		// Update the values
		s[1] = (s1l & 0xffffffff) + (s1h << 32)
		s[0] = (s0l & 0xffffffff) + (s0h << 32)
	}
	return len(data), nil
}

func (s *sum128a) Size() int      { return 16 }
func (s *sum128a) BlockSize() int { return 1 }

func (s *sum128a) Sum(in []byte) []byte {
	return append(in,
		byte(s[0]>>56), byte(s[0]>>48), byte(s[0]>>40), byte(s[0]>>32), byte(s[0]>>24), byte(s[0]>>16), byte(s[0]>>8), byte(s[0]),
		byte(s[1]>>56), byte(s[1]>>48), byte(s[1]>>40), byte(s[1]>>32), byte(s[1]>>24), byte(s[1]>>16), byte(s[1]>>8), byte(s[1]),
	)
}

// New128a returns a new 128-bit FNV-1a hash.Hash.
// Its Sum method will lay the value out in big-endian byte order.
func New128a() hash.Hash {
	var s sum128a
	s[0] = offset128Higher
	s[1] = offset128Lower
	return &s
}
