package rsa


import "crypto"

// 	@compatible: 此方法在 v1.4 版本中添加
//
// Public returns the public key corresponding to priv.
func (priv *PrivateKey) Public() crypto.PublicKey {
	return &priv.PublicKey
}
