// 	@compatible: 此文件在 v1.5 版本添加

package big

import "io"

// 	@compatible: 此函数在 v1.5 版本添加
//
func scanSign(r io.ByteScanner) (neg bool, err error) {
	var ch byte
	if ch, err = r.ReadByte(); err != nil {
		return false, err
	}
	switch ch {
	case '-':
		neg = true
	case '+':
		// nothing to do
	default:
		r.UnreadByte()
	}
	return
}
