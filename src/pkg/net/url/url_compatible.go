package url

import "strings"

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
