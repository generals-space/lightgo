package asn1

import "strconv"

// 	@compatible: 该方法在 v1.3 版本初次添加.
func (oi ObjectIdentifier) String() string {
	var s string

	for i, v := range oi {
		if i > 0 {
			s += "."
		}
		s += strconv.Itoa(v)
	}

	return s
}
