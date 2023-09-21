package fmt

import "sync"

// A cache holds a set of reusable objects.
// The slice is a stack (LIFO).
// If more are needed, the cache creates them by calling new.
type cache struct {
	mu    sync.Mutex
	saved []interface{}
	new   func() interface{}
}

func (c *cache) put(x interface{}) {
	c.mu.Lock()
	if len(c.saved) < cap(c.saved) {
		c.saved = append(c.saved, x)
	}
	c.mu.Unlock()
}

func (c *cache) get() interface{} {
	c.mu.Lock()
	n := len(c.saved)
	if n == 0 {
		c.mu.Unlock()
		return c.new()
	}
	x := c.saved[n-1]
	c.saved = c.saved[0 : n-1]
	c.mu.Unlock()
	return x
}

func newCache(f func() interface{}) *cache {
	return &cache{saved: make([]interface{}, 0, 100), new: f}
}
