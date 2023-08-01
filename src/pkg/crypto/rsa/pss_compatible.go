package rsa

import "crypto"

// 	@compatible: 此方法在 v1.4 版本添加
//
// HashFunc returns pssOpts.Hash so that PSSOptions implements
// crypto.SignerOpts.
func (pssOpts *PSSOptions) HashFunc() crypto.Hash {
	return pssOpts.Hash
}
