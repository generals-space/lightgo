package http

// 	@compatible: 此方法在 v1.13 版本由 clone() -> Clone()
func (h Header) Clone() Header {
	return h.clone()
}
