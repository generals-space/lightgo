package net

import (
	"context"
)

// 	@compatible: 该结构在 v1.8 版本初次添加
// 	其实就是对原本 lookup.go 中的函数做了下封装, 暴露的也都是同名方法.
//
// DefaultResolver is the resolver used by the package-level Lookup
// functions and by Dialers without a specified Resolver.
var DefaultResolver = &Resolver{}

// 	@compatible: 该结构在 v1.8 版本初次添加
//
// A Resolver looks up names and numbers.
//
// A nil *Resolver is equivalent to a zero Resolver.
type Resolver struct {
	// PreferGo controls whether Go's built-in DNS resolver is preferred
	// on platforms where it's available. It is equivalent to setting
	// GODEBUG=netdns=go, but scoped to just this resolver.
	PreferGo bool

	// TODO(bradfitz): optional interface impl override hook
	// TODO(bradfitz): Timeout time.Duration?

	////////////////////////////////////////////////////////////////////////////
	// 	@compatible: 此字段在 v1.9 版本添加
	//
	// StrictErrors controls the behavior of temporary errors
	// (including timeout, socket errors, and SERVFAIL) when using
	// Go's built-in resolver. For a query composed of multiple
	// sub-queries (such as an A+AAAA address lookup, or walking the
	// DNS search list), this option causes such errors to abort the
	// whole query instead of returning a partial result. This is
	// not enabled by default because it may affect compatibility
	// with resolvers that process AAAA queries incorrectly.
	StrictErrors bool
	// 	@compatible: 此字段在 v1.9 版本添加
	//
	// Dial optionally specifies an alternate dialer for use by
	// Go's built-in DNS resolver to make TCP and UDP connections
	// to DNS services. The host in the address parameter will
	// always be a literal IP address and not a host name, and the
	// port in the address parameter will be a literal port number
	// and not a service name.
	// If the Conn returned is also a PacketConn, sent and received DNS
	// messages must adhere to RFC 1035 section 4.2.1, "UDP usage".
	// Otherwise, DNS messages transmitted over Conn must adhere
	// to RFC 7766 section 5, "Transport Protocol Selection".
	// If nil, the default dialer is used.
	Dial func(ctx context.Context, network, address string) (Conn, error)

}

////////////////////////////////////////////////////////////////////////////////
// 	@note: 如下方法与 v1.8 中的原版方法有所不同
// 	为保证变动最小, 将ta们做了改造, 直接调用已有函数.

// LookupHost looks up the given host using the local resolver.
// It returns a slice of that host's addresses.
func (r *Resolver) LookupHost(ctx context.Context, host string) (addrs []string, err error) {
	return LookupHost(host)
}

// 	@note: 这个方法发生的变动有点复杂, 见下面的详细处理
// func (r *Resolver) LookupIPAddr(ctx context.Context, host string) ([]IPAddr, error) {
// 	return LookupIP(host)
// }

// LookupPort looks up the port for the given network and service.
func (r *Resolver) LookupPort(ctx context.Context, network, service string) (port int, err error) {
	return LookupPort(network, service)
}

// LookupCNAME returns the canonical name for the given host.
// Callers that do not care about the canonical name can call
// LookupHost or LookupIP directly; both take care of resolving
// the canonical name as part of the lookup.
//
// A canonical name is the final name after following zero
// or more CNAME records.
// LookupCNAME does not return an error if host does not
// contain DNS "CNAME" records, as long as host resolves to
// address records.
func (r *Resolver) LookupCNAME(ctx context.Context, host string) (cname string, err error) {
	return LookupCNAME(host)
}

// LookupSRV tries to resolve an SRV query of the given service,
// protocol, and domain name. The proto is "tcp" or "udp".
// The returned records are sorted by priority and randomized
// by weight within a priority.
//
// LookupSRV constructs the DNS name to look up following RFC 2782.
// That is, it looks up _service._proto.name. To accommodate services
// publishing SRV records under non-standard names, if both service
// and proto are empty strings, LookupSRV looks up name directly.
func (r *Resolver) LookupSRV(ctx context.Context, service, proto, name string) (cname string, addrs []*SRV, err error) {
	return LookupSRV(service, proto, name)
}

// LookupMX returns the DNS MX records for the given domain name sorted by preference.
func (r *Resolver) LookupMX(ctx context.Context, name string) ([]*MX, error) {
	return LookupMX(name)
}

// LookupNS returns the DNS NS records for the given domain name.
func (r *Resolver) LookupNS(ctx context.Context, name string) ([]*NS, error) {
	return LookupNS(name)
}

// LookupTXT returns the DNS TXT records for the given domain name.
func (r *Resolver) LookupTXT(ctx context.Context, name string) ([]string, error) {
	return LookupTXT(name)
}

// LookupAddr performs a reverse lookup for the given address, returning a list
// of names mapping to that address.
func (r *Resolver) LookupAddr(ctx context.Context, addr string) (names []string, err error) {
	return LookupAddr(addr)
}

////////////////////////////////////////////////////////////////////////////////

// 	@compatible: 此方法在 v1.8 版本添加
// 	@todo: 在 v1.8 中, 该方法的实现过于复杂, 这里只是单纯的将 []IP -> []IPAddr,
// 	如果遇到什么问题, 以后再说吧
//
// LookupIPAddr looks up host using the local resolver.
// It returns a slice of that host's IPv4 and IPv6 addresses.
func (r *Resolver) LookupIPAddr(ctx context.Context, host string) ([]IPAddr, error) {
	addrs, err := LookupIP(host)
	if err != nil {
		return nil, err
	}
	ips := make([]IPAddr, len(addrs))
	for i, ia := range addrs {
		ips[i] = IPAddr{
			IP: ia,
		}
	}
	return ips, nil
}
