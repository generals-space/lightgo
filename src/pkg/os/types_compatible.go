package os

// Type returns type bits in m (m & ModeType).
func (m FileMode) Type() FileMode {
	return m & ModeType
}
