package big

// nlz returns the number of leading zeros in x.
func nlz(x Word) uint {
	return uint(_W - bitLen(x))
}

// nlz64 returns the number of leading zeros in x.
func nlz64(x uint64) uint {
	switch _W {
	case 32:
		w := x >> 32
		if w == 0 {
			return 32 + nlz(Word(x))
		}
		return nlz(Word(w))
	case 64:
		return nlz(Word(x))
	}
	panic("unreachable")
}
