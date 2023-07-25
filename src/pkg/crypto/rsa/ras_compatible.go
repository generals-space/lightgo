package rsa

import (
	"crypto"
	"io"
)

// 	@compatible: 此方法在 v1.4 版本中添加
//
// Public returns the public key corresponding to priv.
func (priv *PrivateKey) Public() crypto.PublicKey {
	return &priv.PublicKey
}

// 	@compatible: 此方法在 v1.4 版本中添加
//
// Sign signs msg with priv, reading randomness from rand. If opts is a
// *PSSOptions then the PSS algorithm will be used, otherwise PKCS#1 v1.5 will
// be used. This method is intended to support keys where the private part is
// kept in, for example, a hardware module. Common uses should use the Sign*
// functions in this package.
func (priv *PrivateKey) Sign(rand io.Reader, msg []byte, opts crypto.SignerOpts) ([]byte, error) {
	if pssOpts, ok := opts.(*PSSOptions); ok {
		return SignPSS(rand, priv, pssOpts.Hash, msg, pssOpts)
	}

	return SignPKCS1v15(rand, priv, opts.HashFunc(), msg)
}

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此方法在 v1.11 版本添加
//
// Size returns the modulus size in bytes. Raw signatures and ciphertexts
// for or by this public key will have the same size.
func (pub *PublicKey) Size() int {
	return (pub.N.BitLen() + 7) / 8
}
