package x509

import (
	"crypto"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rsa"
	"crypto/x509/pkix"
	"encoding/asn1"
	"errors"
	"io"
	"math/big"
	"net"
	"strconv"
)

// 	@compatible: 此结构体在 v1.3 版本初次添加.
//
// CertificateRequest represents a PKCS #10, certificate signature request.
type CertificateRequest struct {
	Raw                      []byte // Complete ASN.1 DER content (CSR, signature algorithm and signature).
	RawTBSCertificateRequest []byte // Certificate request info part of raw ASN.1 DER content.
	RawSubjectPublicKeyInfo  []byte // DER encoded SubjectPublicKeyInfo.
	RawSubject               []byte // DER encoded Subject.

	Version            int
	Signature          []byte
	SignatureAlgorithm SignatureAlgorithm

	PublicKeyAlgorithm PublicKeyAlgorithm
	PublicKey          interface{}

	Subject pkix.Name

	// Attributes is a collection of attributes providing
	// additional information about the subject of the certificate.
	// See RFC 2986 section 4.1.
	Attributes []pkix.AttributeTypeAndValueSET

	// Extensions contains raw X.509 extensions. When parsing CSRs, this
	// can be used to extract extensions that are not parsed by this
	// package.
	Extensions []pkix.Extension

	// ExtraExtensions contains extensions to be copied, raw, into any
	// marshaled CSR. Values override any extensions that would otherwise
	// be produced based on the other fields but are overridden by any
	// extensions specified in Attributes.
	//
	// The ExtraExtensions field is not populated when parsing CSRs, see
	// Extensions.
	ExtraExtensions []pkix.Extension

	// Subject Alternate Name values.
	DNSNames       []string
	EmailAddresses []string
	IPAddresses    []net.IP
}

// These structures reflect the ASN.1 structure of X.509 certificate
// signature requests (see RFC 2986):

// 	@compatible: 此结构体在 v1.3 版本初次添加.
//
type tbsCertificateRequest struct {
	Raw        asn1.RawContent
	Version    int
	Subject    asn1.RawValue
	PublicKey  publicKeyInfo
	Attributes []pkix.AttributeTypeAndValueSET `asn1:"tag:0"`
}

// 	@compatible: 此结构体在 v1.3 版本初次添加.
//
type certificateRequest struct {
	Raw                asn1.RawContent
	TBSCSR             tbsCertificateRequest
	SignatureAlgorithm pkix.AlgorithmIdentifier
	SignatureValue     asn1.BitString
}

// oidExtensionRequest is a PKCS#9 OBJECT IDENTIFIER that indicates requested
// extensions in a CSR.
var oidExtensionRequest = asn1.ObjectIdentifier{1, 2, 840, 113549, 1, 9, 14}

// 	@compatible: 此方法在 v1.3 版本初次添加.
//
// CreateCertificateRequest creates a new certificate based on a template. The
// following members of template are used: Subject, Attributes,
// SignatureAlgorithm, Extension, DNSNames, EmailAddresses, and IPAddresses.
// The private key is the private key of the signer.
//
// The returned slice is the certificate request in DER encoding.
//
// The only supported key types are RSA (*rsa.PrivateKey) and ECDSA
// (*ecdsa.PrivateKey).
func CreateCertificateRequest(rand io.Reader, template *CertificateRequest, priv interface{}) (csr []byte, err error) {
	hashFunc, sigAlgo, err := signingParamsForPrivateKey(priv, template.SignatureAlgorithm)
	if err != nil {
		return nil, err
	}

	var publicKeyBytes []byte
	var publicKeyAlgorithm pkix.AlgorithmIdentifier

	switch priv := priv.(type) {
	case *rsa.PrivateKey:
		publicKeyBytes, publicKeyAlgorithm, err = marshalPublicKey(&priv.PublicKey)
	case *ecdsa.PrivateKey:
		publicKeyBytes, publicKeyAlgorithm, err = marshalPublicKey(&priv.PublicKey)
	default:
		panic("internal error")
	}

	if err != nil {
		return nil, err
	}

	var extensions []pkix.Extension

	if (len(template.DNSNames) > 0 || len(template.EmailAddresses) > 0 || len(template.IPAddresses) > 0) &&
		!oidInExtensions(oidExtensionSubjectAltName, template.ExtraExtensions) {
		sanBytes, err := marshalSANs(template.DNSNames, template.EmailAddresses, template.IPAddresses)
		if err != nil {
			return nil, err
		}

		extensions = append(extensions, pkix.Extension{
			Id:    oidExtensionSubjectAltName,
			Value: sanBytes,
		})
	}

	extensions = append(extensions, template.ExtraExtensions...)

	var attributes []pkix.AttributeTypeAndValueSET
	attributes = append(attributes, template.Attributes...)

	if len(extensions) > 0 {
		// specifiedExtensions contains all the extensions that we
		// found specified via template.Attributes.
		specifiedExtensions := make(map[string]bool)

		for _, atvSet := range template.Attributes {
			if !atvSet.Type.Equal(oidExtensionRequest) {
				continue
			}

			for _, atvs := range atvSet.Value {
				for _, atv := range atvs {
					specifiedExtensions[atv.Type.String()] = true
				}
			}
		}

		atvs := make([]pkix.AttributeTypeAndValue, 0, len(extensions))
		for _, e := range extensions {
			if specifiedExtensions[e.Id.String()] {
				// Attributes already contained a value for
				// this extension and it takes priority.
				continue
			}

			atvs = append(atvs, pkix.AttributeTypeAndValue{
				// There is no place for the critical flag in a CSR.
				Type:  e.Id,
				Value: e.Value,
			})
		}

		// Append the extensions to an existing attribute if possible.
		appended := false
		for _, atvSet := range attributes {
			if !atvSet.Type.Equal(oidExtensionRequest) || len(atvSet.Value) == 0 {
				continue
			}

			atvSet.Value[0] = append(atvSet.Value[0], atvs...)
			appended = true
			break
		}

		// Otherwise, add a new attribute for the extensions.
		if !appended {
			attributes = append(attributes, pkix.AttributeTypeAndValueSET{
				Type: oidExtensionRequest,
				Value: [][]pkix.AttributeTypeAndValue{
					atvs,
				},
			})
		}
	}

	asn1Subject := template.RawSubject
	if len(asn1Subject) == 0 {
		asn1Subject, err = asn1.Marshal(template.Subject.ToRDNSequence())
		if err != nil {
			return
		}
	}

	tbsCSR := tbsCertificateRequest{
		Version: 0, // PKCS #10, RFC 2986
		Subject: asn1.RawValue{FullBytes: asn1Subject},
		PublicKey: publicKeyInfo{
			Algorithm: publicKeyAlgorithm,
			PublicKey: asn1.BitString{
				Bytes:     publicKeyBytes,
				BitLength: len(publicKeyBytes) * 8,
			},
		},
		Attributes: attributes,
	}

	tbsCSRContents, err := asn1.Marshal(tbsCSR)
	if err != nil {
		return
	}
	tbsCSR.Raw = tbsCSRContents

	h := hashFunc.New()
	h.Write(tbsCSRContents)
	digest := h.Sum(nil)

	var signature []byte
	switch priv := priv.(type) {
	case *rsa.PrivateKey:
		signature, err = rsa.SignPKCS1v15(rand, priv, hashFunc, digest)
	case *ecdsa.PrivateKey:
		var r, s *big.Int
		if r, s, err = ecdsa.Sign(rand, priv, digest); err == nil {
			signature, err = asn1.Marshal(ecdsaSignature{r, s})
		}
	default:
		panic("internal error")
	}

	if err != nil {
		return
	}

	return asn1.Marshal(certificateRequest{
		TBSCSR:             tbsCSR,
		SignatureAlgorithm: sigAlgo,
		SignatureValue: asn1.BitString{
			Bytes:     signature,
			BitLength: len(signature) * 8,
		},
	})
}

// 	@compatible: 此方法在 v1.3 版本初次添加.
//
// ParseCertificateRequest parses a single certificate request from the
// given ASN.1 DER data.
func ParseCertificateRequest(asn1Data []byte) (*CertificateRequest, error) {
	var csr certificateRequest

	rest, err := asn1.Unmarshal(asn1Data, &csr)
	if err != nil {
		return nil, err
	} else if len(rest) != 0 {
		return nil, asn1.SyntaxError{Msg: "trailing data"}
	}

	return parseCertificateRequest(&csr)
}

// 	@compatible: 此方法在 v1.3 版本初次添加.
//
func parseCertificateRequest(in *certificateRequest) (*CertificateRequest, error) {
	out := &CertificateRequest{
		Raw: in.Raw,
		RawTBSCertificateRequest: in.TBSCSR.Raw,
		RawSubjectPublicKeyInfo:  in.TBSCSR.PublicKey.Raw,
		RawSubject:               in.TBSCSR.Subject.FullBytes,

		Signature:          in.SignatureValue.RightAlign(),
		SignatureAlgorithm: getSignatureAlgorithmFromOID(in.SignatureAlgorithm.Algorithm),

		PublicKeyAlgorithm: getPublicKeyAlgorithmFromOID(in.TBSCSR.PublicKey.Algorithm.Algorithm),

		Version:    in.TBSCSR.Version,
		Attributes: in.TBSCSR.Attributes,
	}

	var err error
	out.PublicKey, err = parsePublicKey(out.PublicKeyAlgorithm, &in.TBSCSR.PublicKey)
	if err != nil {
		return nil, err
	}

	var subject pkix.RDNSequence
	if _, err := asn1.Unmarshal(in.TBSCSR.Subject.FullBytes, &subject); err != nil {
		return nil, err
	}

	out.Subject.FillFromRDNSequence(&subject)

	var extensions []pkix.AttributeTypeAndValue

	for _, atvSet := range in.TBSCSR.Attributes {
		if !atvSet.Type.Equal(oidExtensionRequest) {
			continue
		}

		for _, atvs := range atvSet.Value {
			extensions = append(extensions, atvs...)
		}
	}

	out.Extensions = make([]pkix.Extension, 0, len(extensions))

	for _, e := range extensions {
		value, ok := e.Value.([]byte)
		if !ok {
			return nil, errors.New("x509: extension attribute contained non-OCTET STRING data")
		}

		out.Extensions = append(out.Extensions, pkix.Extension{
			Id:    e.Type,
			Value: value,
		})

		if len(e.Type) == 4 && e.Type[0] == 2 && e.Type[1] == 5 && e.Type[2] == 29 {
			switch e.Type[3] {
			case 17:
				out.DNSNames, out.EmailAddresses, out.IPAddresses, err = parseSANExtension(value)
				if err != nil {
					return nil, err
				}
			}
		}
	}

	return out, nil
}

// 	@compatible: 此方法在 v1.3 版本初次添加.
//
// signingParamsForPrivateKey returns the parameters to use for signing with
// priv. If requestedSigAlgo is not zero then it overrides the default
// signature algorithm.
func signingParamsForPrivateKey(priv interface{}, requestedSigAlgo SignatureAlgorithm) (hashFunc crypto.Hash, sigAlgo pkix.AlgorithmIdentifier, err error) {
	var pubType PublicKeyAlgorithm

	switch priv := priv.(type) {
	case *rsa.PrivateKey:
		pubType = RSA
		sigAlgo.Algorithm = oidSignatureSHA256WithRSA
		hashFunc = crypto.SHA256

	case *ecdsa.PrivateKey:
		pubType = ECDSA

		switch priv.Curve {
		case elliptic.P224(), elliptic.P256():
			hashFunc = crypto.SHA256
			sigAlgo.Algorithm = oidSignatureECDSAWithSHA256
		case elliptic.P384():
			hashFunc = crypto.SHA384
			sigAlgo.Algorithm = oidSignatureECDSAWithSHA384
		case elliptic.P521():
			hashFunc = crypto.SHA512
			sigAlgo.Algorithm = oidSignatureECDSAWithSHA512
		default:
			err = errors.New("x509: unknown elliptic curve")
		}

	default:
		err = errors.New("x509: only RSA and ECDSA private keys supported")
	}

	if err != nil {
		return
	}

	if requestedSigAlgo == 0 {
		return
	}

	found := false
	for _, details := range signatureAlgorithmDetails {
		if details.algo == requestedSigAlgo {
			if details.pubKeyAlgo != pubType {
				err = errors.New("x509: requested SignatureAlgorithm does not match private key type")
				return
			}
			sigAlgo.Algorithm, hashFunc = details.oid, details.hash
			if hashFunc == 0 {
				err = errors.New("x509: cannot sign with hash function requested")
				return
			}
			found = true
			break
		}
	}

	if !found {
		err = errors.New("x509: unknown SignatureAlgorithm")
	}

	return
}

// 	@compatible: 此方法在 v1.3 版本初次添加.
//
// marshalSANs marshals a list of addresses into a the contents of an X.509
// SubjectAlternativeName extension.
func marshalSANs(dnsNames, emailAddresses []string, ipAddresses []net.IP) (derBytes []byte, err error) {
	var rawValues []asn1.RawValue
	for _, name := range dnsNames {
		rawValues = append(rawValues, asn1.RawValue{Tag: 2, Class: 2, Bytes: []byte(name)})
	}
	for _, email := range emailAddresses {
		rawValues = append(rawValues, asn1.RawValue{Tag: 1, Class: 2, Bytes: []byte(email)})
	}
	for _, rawIP := range ipAddresses {
		// If possible, we always want to encode IPv4 addresses in 4 bytes.
		ip := rawIP.To4()
		if ip == nil {
			ip = rawIP
		}
		rawValues = append(rawValues, asn1.RawValue{Tag: 7, Class: 2, Bytes: ip})
	}
	return asn1.Marshal(rawValues)
}

// 	@compatible: 此方法在 v1.3 版本初次添加.
//
func parseSANExtension(value []byte) (dnsNames, emailAddresses []string, ipAddresses []net.IP, err error) {
	// RFC 5280, 4.2.1.6

	// SubjectAltName ::= GeneralNames
	//
	// GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
	//
	// GeneralName ::= CHOICE {
	//      otherName                       [0]     OtherName,
	//      rfc822Name                      [1]     IA5String,
	//      dNSName                         [2]     IA5String,
	//      x400Address                     [3]     ORAddress,
	//      directoryName                   [4]     Name,
	//      ediPartyName                    [5]     EDIPartyName,
	//      uniformResourceIdentifier       [6]     IA5String,
	//      iPAddress                       [7]     OCTET STRING,
	//      registeredID                    [8]     OBJECT IDENTIFIER }
	var seq asn1.RawValue
	if _, err = asn1.Unmarshal(value, &seq); err != nil {
		return
	}
	if !seq.IsCompound || seq.Tag != 16 || seq.Class != 0 {
		err = asn1.StructuralError{Msg: "bad SAN sequence"}
		return
	}

	rest := seq.Bytes
	for len(rest) > 0 {
		var v asn1.RawValue
		rest, err = asn1.Unmarshal(rest, &v)
		if err != nil {
			return
		}
		switch v.Tag {
		case 1:
			emailAddresses = append(emailAddresses, string(v.Bytes))
		case 2:
			dnsNames = append(dnsNames, string(v.Bytes))
		case 7:
			switch len(v.Bytes) {
			case net.IPv4len, net.IPv6len:
				ipAddresses = append(ipAddresses, v.Bytes)
			default:
				err = errors.New("x509: certificate contained IP address of length " + strconv.Itoa(len(v.Bytes)))
				return
			}
		}
	}

	return
}

// 	@compatible: 此结构在 v1.3 版本初次添加.
//
var signatureAlgorithmDetails = []struct {
	algo       SignatureAlgorithm
	oid        asn1.ObjectIdentifier
	pubKeyAlgo PublicKeyAlgorithm
	hash       crypto.Hash
}{
	{MD2WithRSA, oidSignatureMD2WithRSA, RSA, crypto.Hash(0) /* no value for MD2 */},
	{MD5WithRSA, oidSignatureMD5WithRSA, RSA, crypto.MD5},
	{SHA1WithRSA, oidSignatureSHA1WithRSA, RSA, crypto.SHA1},
	{SHA256WithRSA, oidSignatureSHA256WithRSA, RSA, crypto.SHA256},
	{SHA384WithRSA, oidSignatureSHA384WithRSA, RSA, crypto.SHA384},
	{SHA512WithRSA, oidSignatureSHA512WithRSA, RSA, crypto.SHA512},
	{DSAWithSHA1, oidSignatureDSAWithSHA1, DSA, crypto.SHA1},
	{DSAWithSHA256, oidSignatureDSAWithSHA256, DSA, crypto.SHA256},
	{ECDSAWithSHA1, oidSignatureECDSAWithSHA1, ECDSA, crypto.SHA1},
	{ECDSAWithSHA256, oidSignatureECDSAWithSHA256, ECDSA, crypto.SHA256},
	{ECDSAWithSHA384, oidSignatureECDSAWithSHA384, ECDSA, crypto.SHA384},
	{ECDSAWithSHA512, oidSignatureECDSAWithSHA512, ECDSA, crypto.SHA512},
}
