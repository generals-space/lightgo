// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package url parses URLs and implements query escaping.
// See RFC 3986.
package url

import (
	"bytes"
	"errors"
	"strings"
)

// Error reports an error and the operation and URL that caused it.
type Error struct {
	Op  string
	URL string
	Err error
}

func (e *Error) Error() string { return e.Op + " " + e.URL + ": " + e.Err.Error() }

type encoding int

const (
	encodePath        encoding = 1 + iota
	encodePathSegment          // 	@compatible: 该变量在 v1.8 版本中初次添加.
	encodeUserPassword
	encodeQueryComponent
	encodeFragment
)

// A URL represents a parsed URL (technically, a URI reference).
// The general form represented is:
//
//	scheme://[userinfo@]host/path[?query][#fragment]
//
// URLs that do not start with a slash after the scheme are interpreted as:
//
//	scheme:opaque[?query][#fragment]
//
// Note that the Path field is stored in decoded form: /%47%6f%2f becomes /Go/.
// A consequence is that it is impossible to tell which slashes in the Path were
// slashes in the raw URL and which were %2f. This distinction is rarely important,
// but when it is a client must use other routines to parse the raw URL or construct
// the parsed URL. For example, an HTTP server can consult req.RequestURI, and
// an HTTP client can use URL{Host: "example.com", Opaque: "//example.com/Go%2f"}
// instead of URL{Host: "example.com", Path: "/Go/"}.
type URL struct {
	Scheme   string
	Opaque   string    // encoded opaque data
	User     *Userinfo // username and password information
	Host     string    // host or host:port
	Path     string
	RawQuery string // encoded query values, without '?'
	Fragment string // fragment for references, without '#'

	// 	@compatible: 此字段在 v1.5 版本添加
	RawPath string // encoded path hint (Go 1.5 and later only; see EscapedPath method)

}

// Maybe rawurl is of the form scheme:path.
// (Scheme must be [a-zA-Z][a-zA-Z0-9+-.]*)
// If so, return scheme, path; else return "", rawurl.
func getscheme(rawurl string) (scheme, path string, err error) {
	for i := 0; i < len(rawurl); i++ {
		c := rawurl[i]
		switch {
		case 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z':
		// do nothing
		case '0' <= c && c <= '9' || c == '+' || c == '-' || c == '.':
			if i == 0 {
				return "", rawurl, nil
			}
		case c == ':':
			if i == 0 {
				return "", "", errors.New("missing protocol scheme")
			}
			return rawurl[0:i], rawurl[i+1:], nil
		default:
			// we have encountered an invalid character,
			// so there is no valid scheme
			return "", rawurl, nil
		}
	}
	return "", rawurl, nil
}

// Parse parses rawurl into a URL structure.
// The rawurl may be relative or absolute.
func Parse(rawurl string) (url *URL, err error) {
	// Cut off #frag
	u, frag := split(rawurl, "#", true)
	if url, err = parse(u, false); err != nil {
		return nil, err
	}
	if frag == "" {
		return url, nil
	}
	if url.Fragment, err = unescape(frag, encodeFragment); err != nil {
		return nil, &Error{"parse", rawurl, err}
	}
	return url, nil
}

// ParseRequestURI parses rawurl into a URL structure.  It assumes that
// rawurl was received in an HTTP request, so the rawurl is interpreted
// only as an absolute URI or an absolute path.
// The string rawurl is assumed not to have a #fragment suffix.
// (Web browsers strip #fragment before sending the URL to a web server.)
func ParseRequestURI(rawurl string) (url *URL, err error) {
	return parse(rawurl, true)
}

// parse parses a URL from a string in one of two contexts.  If
// viaRequest is true, the URL is assumed to have arrived via an HTTP request,
// in which case only absolute URLs or path-absolute relative URLs are allowed.
// If viaRequest is false, all forms of relative URLs are allowed.
func parse(rawurl string, viaRequest bool) (url *URL, err error) {
	var rest string

	if rawurl == "" && viaRequest {
		err = errors.New("empty url")
		goto Error
	}
	url = new(URL)

	if rawurl == "*" {
		url.Path = "*"
		return
	}

	// Split off possible leading "http:", "mailto:", etc.
	// Cannot contain escaped characters.
	if url.Scheme, rest, err = getscheme(rawurl); err != nil {
		goto Error
	}
	url.Scheme = strings.ToLower(url.Scheme)

	rest, url.RawQuery = split(rest, "?", true)

	if !strings.HasPrefix(rest, "/") {
		if url.Scheme != "" {
			// We consider rootless paths per RFC 3986 as opaque.
			url.Opaque = rest
			return url, nil
		}
		if viaRequest {
			err = errors.New("invalid URI for request")
			goto Error
		}
	}

	if (url.Scheme != "" || !viaRequest && !strings.HasPrefix(rest, "///")) && strings.HasPrefix(rest, "//") {
		var authority string
		authority, rest = split(rest[2:], "/", false)
		url.User, url.Host, err = parseAuthority(authority)
		if err != nil {
			goto Error
		}
		if strings.Contains(url.Host, "%") {
			err = errors.New("hexadecimal escape in host")
			goto Error
		}
	}
	if url.Path, err = unescape(rest, encodePath); err != nil {
		goto Error
	}
	return url, nil

Error:
	return nil, &Error{"parse", rawurl, err}
}

func parseAuthority(authority string) (user *Userinfo, host string, err error) {
	i := strings.LastIndex(authority, "@")
	if i < 0 {
		host = authority
		return
	}
	userinfo, host := authority[:i], authority[i+1:]
	if strings.Index(userinfo, ":") < 0 {
		if userinfo, err = unescape(userinfo, encodeUserPassword); err != nil {
			return
		}
		user = User(userinfo)
	} else {
		username, password := split(userinfo, ":", true)
		if username, err = unescape(username, encodeUserPassword); err != nil {
			return
		}
		if password, err = unescape(password, encodeUserPassword); err != nil {
			return
		}
		user = UserPassword(username, password)
	}
	return
}

// String reassembles the URL into a valid URL string.
func (u *URL) String() string {
	var buf bytes.Buffer
	if u.Scheme != "" {
		buf.WriteString(u.Scheme)
		buf.WriteByte(':')
	}
	if u.Opaque != "" {
		buf.WriteString(u.Opaque)
	} else {
		if u.Scheme != "" || u.Host != "" || u.User != nil {
			buf.WriteString("//")
			if ui := u.User; ui != nil {
				buf.WriteString(ui.String())
				buf.WriteByte('@')
			}
			if h := u.Host; h != "" {
				buf.WriteString(h)
			}
		}
		if u.Path != "" && u.Path[0] != '/' && u.Host != "" {
			buf.WriteByte('/')
		}
		buf.WriteString(escape(u.Path, encodePath))
	}
	if u.RawQuery != "" {
		buf.WriteByte('?')
		buf.WriteString(u.RawQuery)
	}
	if u.Fragment != "" {
		buf.WriteByte('#')
		buf.WriteString(escape(u.Fragment, encodeFragment))
	}
	return buf.String()
}

// ParseQuery parses the URL-encoded query string and returns
// a map listing the values specified for each key.
// ParseQuery always returns a non-nil map containing all the
// valid query parameters found; err describes the first decoding error
// encountered, if any.
func ParseQuery(query string) (m Values, err error) {
	m = make(Values)
	err = parseQuery(m, query)
	return
}

func parseQuery(m Values, query string) (err error) {
	for query != "" {
		key := query
		if i := strings.IndexAny(key, "&;"); i >= 0 {
			key, query = key[:i], key[i+1:]
		} else {
			query = ""
		}
		if key == "" {
			continue
		}
		value := ""
		if i := strings.Index(key, "="); i >= 0 {
			key, value = key[:i], key[i+1:]
		}
		key, err1 := QueryUnescape(key)
		if err1 != nil {
			if err == nil {
				err = err1
			}
			continue
		}
		value, err1 = QueryUnescape(value)
		if err1 != nil {
			if err == nil {
				err = err1
			}
			continue
		}
		m[key] = append(m[key], value)
	}
	return err
}

// resolvePath applies special path segments from refs and applies
// them to base, per RFC 3986.
func resolvePath(base, ref string) string {
	var full string
	if ref == "" {
		full = base
	} else if ref[0] != '/' {
		i := strings.LastIndex(base, "/")
		full = base[:i+1] + ref
	} else {
		full = ref
	}
	if full == "" {
		return ""
	}
	var dst []string
	src := strings.Split(full, "/")
	for _, elem := range src {
		switch elem {
		case ".":
			// drop
		case "..":
			if len(dst) > 0 {
				dst = dst[:len(dst)-1]
			}
		default:
			dst = append(dst, elem)
		}
	}
	if last := src[len(src)-1]; last == "." || last == ".." {
		// Add final slash to the joined path.
		dst = append(dst, "")
	}
	return "/" + strings.TrimLeft(strings.Join(dst, "/"), "/")
}

// IsAbs returns true if the URL is absolute.
func (u *URL) IsAbs() bool {
	return u.Scheme != ""
}

// Parse parses a URL in the context of the receiver.  The provided URL
// may be relative or absolute.  Parse returns nil, err on parse
// failure, otherwise its return value is the same as ResolveReference.
func (u *URL) Parse(ref string) (*URL, error) {
	refurl, err := Parse(ref)
	if err != nil {
		return nil, err
	}
	return u.ResolveReference(refurl), nil
}

// ResolveReference 将传入的 ref 对象合并入原 url 对象.
//
// 双方的 scheme, path, query, frag 等所有的字段, 都以 ref 对象为准进行覆盖.
//
// 即使双方 query 中包含多个参数, 也不会在常用理解那样进行"合并", 同样是进行覆盖.
//
// ResolveReference resolves a URI reference to an absolute URI from
// an absolute base URI, per RFC 3986 Section 5.2.
// The URI reference may be relative or absolute.
// ResolveReference always returns a new URL instance, even if the returned URL
// is identical to either the base or reference.
// If ref is an absolute URL, then ResolveReference ignores base and returns
// a copy of ref.
func (u *URL) ResolveReference(ref *URL) *URL {
	url := *ref
	if ref.Scheme == "" {
		url.Scheme = u.Scheme
	}
	if ref.Scheme != "" || ref.Host != "" || ref.User != nil {
		// The "absoluteURI" or "net_path" cases.
		url.Path = resolvePath(ref.Path, "")
		return &url
	}
	if ref.Opaque != "" {
		url.User = nil
		url.Host = ""
		url.Path = ""
		return &url
	}
	if ref.Path == "" {
		if ref.RawQuery == "" {
			url.RawQuery = u.RawQuery
			if ref.Fragment == "" {
				url.Fragment = u.Fragment
			}
		}
	}
	// The "abs_path" or "rel_path" cases.
	url.Host = u.Host
	url.User = u.User
	url.Path = resolvePath(u.Path, ref.Path)
	return &url
}

// Query parses RawQuery and returns the corresponding values.
func (u *URL) Query() Values {
	v, _ := ParseQuery(u.RawQuery)
	return v
}

// RequestURI returns the encoded path?query or opaque?query
// string that would be used in an HTTP request for u.
func (u *URL) RequestURI() string {
	result := u.Opaque
	if result == "" {
		result = escape(u.Path, encodePath)
		if result == "" {
			result = "/"
		}
	} else {
		if strings.HasPrefix(result, "//") {
			result = u.Scheme + ":" + result
		}
	}
	if u.RawQuery != "" {
		result += "?" + u.RawQuery
	}
	return result
}
