package ecdsa

import (
	"crypto"
	"encoding/asn1"
	"io"
	"math/big"
)

// 	@compatible: 此方法在 v1.4 版本中添加
//
// Public returns the public key corresponding to priv.
func (priv *PrivateKey) Public() crypto.PublicKey {
	return &priv.PublicKey
}

// 	@compatible: 此结构在 v1.4 版本中添加
//
type ecdsaSignature struct {
	R, S *big.Int
}

// 	@compatible: 此方法在 v1.4 版本中添加
//
// Sign signs msg with priv, reading randomness from rand. This method is
// intended to support keys where the private part is kept in, for example, a
// hardware module. Common uses should use the Sign function in this package
// directly.
func (priv *PrivateKey) Sign(rand io.Reader, msg []byte, opts crypto.SignerOpts) ([]byte, error) {
	r, s, err := Sign(rand, priv, msg)
	if err != nil {
		return nil, err
	}

	return asn1.Marshal(ecdsaSignature{r, s})
}
