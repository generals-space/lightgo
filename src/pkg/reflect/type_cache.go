package reflect

import (
	"sync"
)

// 这是一个匿名结构体, 直接定义 lookupCache 对象.
//
// The lookupCache caches ChanOf, MapOf, and SliceOf lookups.
var lookupCache struct {
	sync.RWMutex
	m map[cacheKey]*rtype
}

// A cacheKey is the key for use in the lookupCache.
// Four values describe any of the types we are looking for:
// type kind, one or two subtypes, and an extra integer.
type cacheKey struct {
	kind  Kind
	t1    *rtype
	t2    *rtype
	extra uintptr
}

// cacheGet looks for a type under the key k in the lookupCache.
// If it finds one, it returns that type.
// If not, it returns nil with the cache locked.
// The caller is expected to use cachePut to unlock the cache.
func cacheGet(k cacheKey) Type {
	lookupCache.RLock()
	t := lookupCache.m[k]
	lookupCache.RUnlock()
	if t != nil {
		return t
	}

	lookupCache.Lock()
	t = lookupCache.m[k]
	if t != nil {
		lookupCache.Unlock()
		return t
	}

	if lookupCache.m == nil {
		lookupCache.m = make(map[cacheKey]*rtype)
	}

	return nil
}

// cachePut stores the given type in the cache, unlocks the cache,
// and returns the type. It is expected that the cache is locked
// because cacheGet returned nil.
func cachePut(k cacheKey, t *rtype) Type {
	lookupCache.m[k] = t
	lookupCache.Unlock()
	return t
}
