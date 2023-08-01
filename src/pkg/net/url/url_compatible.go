package url

import "strings"

////////////////////////////////////////////////////////////////////////////////
// 	@compatible: 此函数在 v1.5 版本初次添加
//
// EscapedPath returns the escaped form of u.Path.
// In general there are multiple possible escaped forms of any path.
// EscapedPath returns u.RawPath when it is a valid escaping of u.Path.
// Otherwise EscapedPath ignores u.RawPath and computes an escaped
// form on its own.
// The String and RequestURI methods use EscapedPath to construct
// their results.
// In general, code should call EscapedPath instead of
// reading u.RawPath directly.
func (u *URL) EscapedPath() string {
	if u.RawPath != "" && validEncodedPath(u.RawPath) {
		p, err := unescape(u.RawPath, encodePath)
		if err == nil && p == u.Path {
			return u.RawPath
		}
	}
	if u.Path == "*" {
		return "*" // don't escape (Issue 11202)
	}
	return escape(u.Path, encodePath)
}

// 	@compatible: 此函数在 v1.5 版本初次添加
//
// validEncodedPath reports whether s is a valid encoded path.
// It must not contain any bytes that require escaping during path encoding.
func validEncodedPath(s string) bool {
	for i := 0; i < len(s); i++ {
		// RFC 3986, Appendix A.
		// pchar = unreserved / pct-encoded / sub-delims / ":" / "@".
		// shouldEscape is not quite compliant with the RFC,
		// so we check the sub-delims ourselves and let
		// shouldEscape handle the others.
		switch s[i] {
		case '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=', ':', '@':
			// ok
		case '[', ']':
			// ok - not specified in RFC 3986 but left alone by modern browsers
		case '%':
			// ok - percent encoded, will decode
		default:
			if shouldEscape(s[i], encodePath) {
				return false
			}
		}
	}
	return true
}

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此函数在 v1.8 版本初次添加
//
// PathUnescape does the inverse transformation of PathEscape, converting
// %AB into the byte 0xAB. It returns an error if any % is not followed by
// two hexadecimal digits.
//
// PathUnescape is identical to QueryUnescape except that it does not unescape '+' to ' ' (space).
func PathUnescape(s string) (string, error) {
	return unescape(s, encodePathSegment)
}

// 	@compatible: 此函数在 v1.8 版本初次添加
//
// PathEscape escapes the string so it can be safely placed
// inside a URL path segment.
func PathEscape(s string) string {
	return escape(s, encodePathSegment)
}

// Hostname returns u.Host, without any port number.
//
// If Host is an IPv6 literal with a port number, Hostname returns the
// IPv6 literal without the square brackets. IPv6 literals may include
// a zone identifier.
func (u *URL) Hostname() string {
	return stripPort(u.Host)
}

func stripPort(hostport string) string {
	colon := strings.IndexByte(hostport, ':')
	if colon == -1 {
		return hostport
	}
	if i := strings.IndexByte(hostport, ']'); i != -1 {
		return strings.TrimPrefix(hostport[:i], "[")
	}
	return hostport[:colon]
}

// 	@compatible: addAt v1.8
//
// Port returns the port part of u.Host, without the leading colon.
// If u.Host doesn't contain a port, Port returns an empty string.
func (u *URL) Port() string {
	return portOnly(u.Host)
}

func portOnly(hostport string) string {
	colon := strings.IndexByte(hostport, ':')
	if colon == -1 {
		return ""
	}
	if i := strings.Index(hostport, "]:"); i != -1 {
		return hostport[i+len("]:"):]
	}
	if strings.Contains(hostport, "]") {
		return ""
	}
	return hostport[colon+len(":"):]
}

////////////////////////////////////////////////////////////////////////////////
