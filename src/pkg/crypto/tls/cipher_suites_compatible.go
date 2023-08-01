package tls

import (
	"crypto"
	"crypto/aes"
	"crypto/cipher"
	"crypto/hmac"
	"crypto/sha256"
	// "golang_org/x/crypto/chacha20poly1305"
	"crypto/tls/chacha20poly1305"
)

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此函数在 v1.8 版本初次添加
//
// macSHA256 returns a SHA-256 based MAC. These are only supported in TLS 1.2
// so the given version is ignored.
func macSHA256(version uint16, key []byte) macFunction {
	return tls10MAC{hmac.New(sha256.New, key)}
}

// 	@compatible: 此函数在 v1.8 版本初次添加
//
func aeadChaCha20Poly1305(key, fixedNonce []byte) cipher.AEAD {
	aead, err := chacha20poly1305.New(key)
	if err != nil {
		panic(err)
	}

	ret := &xorNonceAEAD{aead: aead}
	copy(ret.nonceMask[:], fixedNonce)
	return ret
}

// 	@compatible: 此接口在 v1.8 版本添加
type aead interface {
	cipher.AEAD

	// explicitIVLen returns the number of bytes used by the explicit nonce
	// that is included in the record. This is eight for older AEADs and
	// zero for modern ones.
	explicitNonceLen() int
}

// 	@compatible: 此结构体在 v1.8 版本初次添加
//
// xoredNonceAEAD wraps an AEAD by XORing in a fixed pattern to the nonce
// before each call.
type xorNonceAEAD struct {
	nonceMask [12]byte
	aead      cipher.AEAD
}

func (f *xorNonceAEAD) NonceSize() int        { return 8 }
func (f *xorNonceAEAD) Overhead() int         { return f.aead.Overhead() }
func (f *xorNonceAEAD) explicitNonceLen() int { return 0 }

func (f *xorNonceAEAD) Seal(out, nonce, plaintext, additionalData []byte) []byte {
	for i, b := range nonce {
		f.nonceMask[4+i] ^= b
	}
	result := f.aead.Seal(out, f.nonceMask[:], plaintext, additionalData)
	for i, b := range nonce {
		f.nonceMask[4+i] ^= b
	}

	return result
}

func (f *xorNonceAEAD) Open(out, nonce, plaintext, additionalData []byte) ([]byte, error) {
	for i, b := range nonce {
		f.nonceMask[4+i] ^= b
	}
	result, err := f.aead.Open(out, f.nonceMask[:], plaintext, additionalData)
	for i, b := range nonce {
		f.nonceMask[4+i] ^= b
	}

	return result, err
}

////////////////////////////////////////////////////////////////////////////////

const (
	// 	@compatible: 此变量在 v1.12 版本添加
	aeadNonceLength   = 12
	// 	@compatible: 此变量在 v1.12 版本添加
	noncePrefixLength = 4
)

// 	@compatible: 此接口在 v1.12 版本添加
//
// A cipherSuiteTLS13 defines only the pair of the AEAD algorithm and hash
// algorithm to be used with HKDF. See RFC 8446, Appendix B.4.
type cipherSuiteTLS13 struct {
	id     uint16
	keyLen int
	aead   func(key, fixedNonce []byte) aead
	hash   crypto.Hash
}

// 	@compatible: 此变量在 v1.12 版本添加
//
var cipherSuitesTLS13 = []*cipherSuiteTLS13{
	{TLS_AES_128_GCM_SHA256, 16, aeadAESGCMTLS13, crypto.SHA256},
	// 	@compatibleNote: 暂时不兼容这个
	// {TLS_CHACHA20_POLY1305_SHA256, 32, aeadChaCha20Poly1305, crypto.SHA256},
	{TLS_AES_256_GCM_SHA384, 32, aeadAESGCMTLS13, crypto.SHA384},
}


// 	@compatible: 此函数在 v1.12 版本添加
//
func aeadAESGCMTLS13(key, nonceMask []byte) aead {
	if len(nonceMask) != aeadNonceLength {
		panic("tls: internal error: wrong nonce length")
	}
	aes, err := aes.NewCipher(key)
	if err != nil {
		panic(err)
	}
	aead, err := cipher.NewGCM(aes)
	if err != nil {
		panic(err)
	}

	ret := &xorNonceAEAD{aead: aead}
	copy(ret.nonceMask[:], nonceMask)
	return ret
}
