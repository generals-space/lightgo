// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package http

// 	@compatible: 该列表在 v1.7 版本中更全, 且有全部的 RFC 出处.
// 见 [golang v1.7 status.go](https://github.com/golang/go/tree/go1.7/src/net/http/status.go)
//
// HTTP status codes as registered with IANA.
// See: http://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml
//
// HTTP status codes, defined in RFC 2616.
const (
	StatusContinue           = 100
	StatusSwitchingProtocols = 101
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusProcessing         = 102 // RFC 2518, 10.1
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusEarlyHints         = 103 // RFC 8297

	StatusOK                   = 200
	StatusCreated              = 201
	StatusAccepted             = 202
	StatusNonAuthoritativeInfo = 203
	StatusNoContent            = 204
	StatusResetContent         = 205
	StatusPartialContent       = 206
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusMultiStatus          = 207 // RFC 4918, 11.1
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusAlreadyReported      = 208 // RFC 5842, 7.1
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusIMUsed               = 226 // RFC 3229, 10.4.1

	StatusMultipleChoices   = 300
	StatusMovedPermanently  = 301
	StatusFound             = 302
	StatusSeeOther          = 303
	StatusNotModified       = 304
	StatusUseProxy          = 305
	StatusTemporaryRedirect = 307
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusPermanentRedirect = 308 // RFC 7538, 3

	StatusBadRequest                   = 400
	StatusUnauthorized                 = 401
	StatusPaymentRequired              = 402
	StatusForbidden                    = 403
	StatusNotFound                     = 404
	StatusMethodNotAllowed             = 405
	StatusNotAcceptable                = 406
	StatusProxyAuthRequired            = 407
	StatusRequestTimeout               = 408
	StatusConflict                     = 409
	StatusGone                         = 410
	StatusLengthRequired               = 411
	StatusPreconditionFailed           = 412
	StatusRequestEntityTooLarge        = 413
	StatusRequestURITooLong            = 414
	StatusUnsupportedMediaType         = 415
	StatusRequestedRangeNotSatisfiable = 416
	StatusExpectationFailed            = 417
	StatusTeapot                       = 418
	// 	@compatible: 此变量在 v1.11 版本添加
	StatusMisdirectedRequest           = 421 // RFC 7540, 9.1.2
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusUnprocessableEntity          = 422 // RFC 4918, 11.2
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusLocked                       = 423 // RFC 4918, 11.3
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusFailedDependency             = 424 // RFC 4918, 11.4
	// 	@compatible: 如下字段在 v1.12 版本添加
	StatusTooEarly                     = 425 // RFC 8470, 5.2.
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusUpgradeRequired              = 426 // RFC 7231, 6.5.15

	StatusInternalServerError     = 500
	StatusNotImplemented          = 501
	StatusBadGateway              = 502
	StatusServiceUnavailable      = 503
	StatusGatewayTimeout          = 504
	StatusHTTPVersionNotSupported = 505
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusVariantAlsoNegotiates         = 506 // RFC 2295, 8.1
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusInsufficientStorage           = 507 // RFC 4918, 11.5
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusLoopDetected                  = 508 // RFC 5842, 7.2
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusNotExtended                   = 510 // RFC 2774, 7

	// New HTTP status codes from RFC 6585. Not exported yet in Go 1.1.
	// See discussion at https://codereview.appspot.com/7678043/
	statusPreconditionRequired          = 428
	statusTooManyRequests               = 429
	statusRequestHeaderFieldsTooLarge   = 431
	statusNetworkAuthenticationRequired = 511

	// 	@compatible: 如下字段在 v1.6 版本被暴露(451是新增的)
	StatusPreconditionRequired          = 428
	StatusTooManyRequests               = 429
	StatusRequestHeaderFieldsTooLarge   = 431
	StatusUnavailableForLegalReasons    = 451
	StatusNetworkAuthenticationRequired = 511

)

var statusText = map[int]string{
	StatusContinue:           "Continue",
	StatusSwitchingProtocols: "Switching Protocols",
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusProcessing:         "Processing",
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusEarlyHints:         "Early Hints",

	StatusOK:                   "OK",
	StatusCreated:              "Created",
	StatusAccepted:             "Accepted",
	StatusNonAuthoritativeInfo: "Non-Authoritative Information",
	StatusNoContent:            "No Content",
	StatusResetContent:         "Reset Content",
	StatusPartialContent:       "Partial Content",
	StatusMultiStatus:          "Multi-Status",
	StatusAlreadyReported:      "Already Reported",
	StatusIMUsed:               "IM Used",

	StatusMultipleChoices:   "Multiple Choices",
	StatusMovedPermanently:  "Moved Permanently",
	StatusFound:             "Found",
	StatusSeeOther:          "See Other",
	StatusNotModified:       "Not Modified",
	StatusUseProxy:          "Use Proxy",
	StatusTemporaryRedirect: "Temporary Redirect",
	// 	@compatible: 此变量在 v1.7 版本添加
	StatusPermanentRedirect: "Permanent Redirect",

	StatusBadRequest:                   "Bad Request",
	StatusUnauthorized:                 "Unauthorized",
	StatusPaymentRequired:              "Payment Required",
	StatusForbidden:                    "Forbidden",
	StatusNotFound:                     "Not Found",
	StatusMethodNotAllowed:             "Method Not Allowed",
	StatusNotAcceptable:                "Not Acceptable",
	StatusProxyAuthRequired:            "Proxy Authentication Required",
	StatusRequestTimeout:               "Request Timeout",
	StatusConflict:                     "Conflict",
	StatusGone:                         "Gone",
	StatusLengthRequired:               "Length Required",
	StatusPreconditionFailed:           "Precondition Failed",
	StatusRequestEntityTooLarge:        "Request Entity Too Large",
	StatusRequestURITooLong:            "Request URI Too Long",
	StatusUnsupportedMediaType:         "Unsupported Media Type",
	StatusRequestedRangeNotSatisfiable: "Requested Range Not Satisfiable",
	StatusExpectationFailed:            "Expectation Failed",
	StatusTeapot:                       "I'm a teapot",
	// 	@compatible: 此变量在 v1.11 版本添加
	StatusMisdirectedRequest:           "Misdirected Request",
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusUnprocessableEntity:          "Unprocessable Entity",
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusLocked:                       "Locked",
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusFailedDependency:             "Failed Dependency",
	// 	@compatible: 如下字段在 v1.12 版本添加
	StatusTooEarly:                     "Too Early",
	// 	@compatible: 如下字段在 v1.7 版本添加
	StatusUpgradeRequired:              "Upgrade Required",

	StatusInternalServerError:     "Internal Server Error",
	StatusNotImplemented:          "Not Implemented",
	StatusBadGateway:              "Bad Gateway",
	StatusServiceUnavailable:      "Service Unavailable",
	StatusGatewayTimeout:          "Gateway Timeout",
	StatusHTTPVersionNotSupported: "HTTP Version Not Supported",
	StatusVariantAlsoNegotiates:         "Variant Also Negotiates",
	StatusInsufficientStorage:           "Insufficient Storage",
	StatusLoopDetected:                  "Loop Detected",
	StatusNotExtended:                   "Not Extended",

	// 	@compatible: 如下字段在 v1.6 版本添加(被暴露)
	//
	// statusPreconditionRequired:          "Precondition Required",
	// statusTooManyRequests:               "Too Many Requests",
	// statusRequestHeaderFieldsTooLarge:   "Request Header Fields Too Large",
	// statusNetworkAuthenticationRequired: "Network Authentication Required",
	StatusPreconditionRequired:          "Precondition Required",
	StatusTooManyRequests:               "Too Many Requests",
	StatusRequestHeaderFieldsTooLarge:   "Request Header Fields Too Large",
	StatusNetworkAuthenticationRequired: "Network Authentication Required",
	StatusUnavailableForLegalReasons:    "Unavailable For Legal Reasons",
}

// StatusText returns a text for the HTTP status code. It returns the empty
// string if the code is unknown.
func StatusText(code int) string {
	return statusText[code]
}
