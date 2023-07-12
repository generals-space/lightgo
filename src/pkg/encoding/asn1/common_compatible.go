package asn1

// 	@compatible: 如下变量在 v1.6 版本中, 由原本的 tagXXX 被改为 TagXXX
// 	@note: 原本的 tagXXX 列表仍然保留着
//
// ASN.1 tags represent the type of the following object.
const (
	TagBoolean         = 1
	TagInteger         = 2
	TagBitString       = 3
	TagOctetString     = 4
	TagOID             = 6
	TagEnum            = 10
	TagUTF8String      = 12
	TagSequence        = 16
	TagSet             = 17
	TagPrintableString = 19
	TagT61String       = 20
	TagIA5String       = 22
	TagUTCTime         = 23
	TagGeneralizedTime = 24
	TagGeneralString   = 27
)

// 	@compatible: 如下变量在 v1.6 版本中, 由原本的 classXXX 被改为 ClassXXX
// 	@note: 原本的 tagXXX 列表仍然保留着
//
// ASN.1 class types represent the namespace of the tag.
const (
	ClassUniversal       = 0
	ClassApplication     = 1
	ClassContextSpecific = 2
	ClassPrivate         = 3
)
