// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "arch_amd64.h"
#include "malloc.h"
#include "type.h"
#include "race.h"
#include "../../cmd/ld/textflag.h"

// This file contains the implementation of Go's map type.
//
// The map is just a hash table. 
// The data is arranged into an array of buckets. 
// Each bucket contains up to 8 key/value pairs. 
// The low-order bits of the hash are used to select a bucket. 
// Each bucket contains a few
// high-order bits of each hash to distinguish the entries
// within a single bucket.
//
// If more than 8 keys hash to a bucket, we chain on extra buckets.
//
// When the hashtable grows, we allocate a new array of buckets twice as big. 
// Buckets are incrementally copied from the old bucket array to the new bucket array.
//
// Map iterators walk through the array of buckets and
// return the keys in walk order (bucket #, then overflow
// chain order, then bucket index).  To maintain iteration
// semantics, we never move keys within their bucket (if
// we did, keys might be returned 0 or 2 times).  When
// growing the table, iterators remain iterating through the
// old table and must check the new table if the bucket
// they are iterating through has been moved ("evacuated")
// to the new table.

// Maximum number of key/value pairs a bucket can hold.
#define BUCKETSIZE 8

// Maximum average load of a bucket that triggers growth.
#define LOAD 6.5

// Picking LOAD: too large and we have lots of overflow
// buckets, too small and we waste a lot of space.  I wrote
// a simple program to check some stats for different loads:
// (64-bit, 8 byte keys and values)
//        LOAD    %overflow  bytes/entry     hitprobe    missprobe
//        4.00         2.13        20.77         3.00         4.00
//        4.50         4.05        17.30         3.25         4.50
//        5.00         6.85        14.77         3.50         5.00
//        5.50        10.55        12.94         3.75         5.50
//        6.00        15.27        11.67         4.00         6.00
//        6.50        20.90        10.79         4.25         6.50
//        7.00        27.14        10.15         4.50         7.00
//        7.50        34.03         9.73         4.75         7.50
//        8.00        41.10         9.40         5.00         8.00
//
// %overflow   = percentage of buckets which have an overflow bucket
// bytes/entry = overhead bytes used per key/value pair
// hitprobe    = # of entries to check when looking up a present key
// missprobe   = # of entries to check when looking up an absent key
//
// Keep in mind this data is for maximally loaded tables, i.e. just
// before the table grows.  Typical tables will be somewhat less loaded.

// Maximum key or value size to keep inline (instead of mallocing per element).
// Must fit in a uint8.
// Fast versions cannot handle big values - the cutoff size for
// fast versions in ../../cmd/gc/walk.c must be at most this value.
#define MAXKEYSIZE 128
#define MAXVALUESIZE 128

// Bucket map 中存放单个 hash 对应的 key 数据的地方.
typedef struct Bucket Bucket;
struct Bucket
{
	// Note: the format of the Bucket is encoded in ../../cmd/gc/reflect.c and
	// ../reflect/type.go. 
	// Don't change this structure without also changing that code!
	uint8  tophash[BUCKETSIZE]; // top 8 bits of hash of each entry (0 = empty)
	Bucket *overflow;           // overflow bucket, if any
	byte   data[1];             // BUCKETSIZE keys followed by BUCKETSIZE values
};
// NOTE: packing all the keys together and then all the values together 
// makes the code a bit more complicated than alternating key/value/key/value/... 
// but it allows us to eliminate padding which would be needed for,
// e.g., map[int64]int8.

// Low-order bit of overflow field is used to mark a bucket as already evacuated
// without destroying the overflow pointer.
// Only buckets in oldbuckets will be marked as evacuated.
// Evacuated bit will be set identically on the base bucket and any overflow buckets.
#define evacuated(b) (((uintptr)(b)->overflow & 1) != 0)
#define overflowptr(b) ((Bucket*)((uintptr)(b)->overflow & ~(uintptr)1))

struct Hmap
{
	// Note: the format of the Hmap is encoded in ../../cmd/gc/reflect.c and
	// ../reflect/type.go. 
	// Don't change this structure without also changing that code!
	
	// count map 中的元素数量(内置函数 len() 返回的就是此值)
	//
	// # live cells == size of map.  Must be first (used by len() builtin)
	// 元素数量
	uintgo  count;
	// 状态标识
	uint32  flags;
	// 用于计算 hash 值的 seed, 是一个随机整数, 在创建哈希表时确定.
	// map 中存储的 key 都是以该值计算 hash 值的.
	//
	// hash seed
	uint32  hash0;
	// len(buckets) == bucketsize == 2^B
	//
	// ta能决定当前 map 中 bucket 的数量.
	//
	// log_2 of # of buckets (can hold up to LOAD * 2^B items)
	uint8   B;
	// 当前 map 对象 key 的大小, 一般为 string, int 这样的变量类型大小
	//
	// key size in bytes
	uint8   keysize;
	// 当前 map 对象 value 的大小, 与 key 相似, 同样是类型大小,
	// 不过 value 还可能是 struct 类型.
	//
	// value size in bytes
	uint8   valuesize;
	// bucket size in bytes
	uint16  bucketsize;

	// buckets 是一个数组, 是真正存放 key/value 数据的地方.
	//
	// 其中每个 bucket 成员都存储着某一 hash 值的一个或多个 k/v 对,
	// map 操作中, 会先根据 hash0 成员与目标 key 生成一个 hash 值, 并根据这个 hash 值,
	// 计算出 buckets 数组中的索引, 确定该 key 存放在哪一个 bucket 成员中.
	//
	// 另外, 需要注意, hash 函数根据 key 值得到 hash 值, 并不是完美的. 
	// 虽然几率很小, 总会出现不同 key 计算出相同 hash 值的情况.
	// 这些不同的 key, 其实都存放在同一个 bucket 中, 因为 bucket 中也存在一个数组(slots),
	// 我们可以把每个 bucket 中的 slots 数组看成槽位, 发生重复的 key 会按照先后顺序找到
	// 自己的槽位.
	//
	// array of 2^B Buckets. may be nil if count==0.
	byte    *buckets;
	// previous bucket array of half the size, non-nil only when growing
	byte    *oldbuckets;
	// progress counter for evacuation
	// (buckets less than this have been evacuated)
	uintptr nevacuate;
};

// possible flags
enum
{
	IndirectKey = 1,    // storing pointers to keys
	IndirectValue = 2,  // storing pointers to values
	Iterator = 4,       // there may be an iterator using buckets
	OldIterator = 8,    // there may be an iterator using oldbuckets
};

// Macros for dereferencing indirect keys
#define IK(h, p) (((h)->flags & IndirectKey) != 0 ? *(byte**)(p) : (p))
#define IV(h, p) (((h)->flags & IndirectValue) != 0 ? *(byte**)(p) : (p))

enum
{
	// check invariants before and after every op.  Slow!!!
	docheck = 0,
	// print every operation
	debug = 0,
	// check interaction of mallocgc() with the garbage collector
	checkgc = 0 || docheck,
};

static void check(MapType *t, Hmap *h)
{
	uintptr bucket, oldbucket;
	Bucket *b;
	uintptr i;
	uintptr hash;
	uintgo cnt;
	uint8 top;
	bool eq;
	byte *k, *v;

	cnt = 0;

	// check buckets
	for(bucket = 0; bucket < (uintptr)1 << h->B; bucket++) {
		if(h->oldbuckets != nil) {
			oldbucket = bucket & (((uintptr)1 << (h->B - 1)) - 1);
			b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
			// b is still uninitialized
			if(!evacuated(b)) continue; 
		}
		for(b = (Bucket*)(h->buckets + bucket * h->bucketsize); b != nil; b = b->overflow) {
			for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
				if(b->tophash[i] == 0) continue;
				cnt++;
				t->key->alg->equal(&eq, t->key->size, IK(h, k), IK(h, k));
				// NaN!
				if(!eq) continue; 
				hash = h->hash0;
				t->key->alg->hash(&hash, t->key->size, IK(h, k));
				top = hash >> (8*sizeof(uintptr) - 8);
				if(top == 0) top = 1;
				if(top != b->tophash[i]) runtime·throw("bad hash");
			}
		}
	}

	// check oldbuckets
	if(h->oldbuckets != nil) {
		for(oldbucket = 0; oldbucket < (uintptr)1 << (h->B - 1); oldbucket++) {
			b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
			if(evacuated(b)) continue;
			if(oldbucket < h->nevacuate) runtime·throw("bucket became unevacuated");
			for(; b != nil; b = overflowptr(b)) {
				for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
					if(b->tophash[i] == 0) continue;
					cnt++;
					t->key->alg->equal(&eq, t->key->size, IK(h, k), IK(h, k));
					if(!eq) continue; // NaN!
					hash = h->hash0;
					t->key->alg->hash(&hash, t->key->size, IK(h, k));
					top = hash >> (8*sizeof(uintptr) - 8);
					if(top == 0) top = 1;
					if(top != b->tophash[i]) runtime·throw("bad hash (old)");
				}
			}
		}
	}

	if(cnt != h->count) {
		runtime·printf("%D %D\n", (uint64)cnt, (uint64)h->count);
		runtime·throw("entries missing");
	}
}

// hash_init 初始化 map 对象(主调函数在调用此方法时事先一定通过 malloc 为该 map 申请了空间).
//
// 	@param t: src/pkg/runtime/type.h -> MapType{} 类型对象, 其中包含目标 map 对象的 key/value 类型属性.
// 	@param h: 目标 map 对象的指针(起始地址)
// 	@param hint: make(map[string]string, hint)的第2个参数, 表示该 map 对象的容量.
//
// caller:
// 	1. runtime·makemap_c() 只有这一处, 在使用 make() 创建 map 对象时被调用.
//
static void hash_init(MapType *t, Hmap *h, uint32 hint)
{
	uint8 B;
	byte *buckets;
	uintptr keysize, valuesize, bucketsize;
	uint8 flags;

	flags = 0;

	// figure out how big we have to make everything
	keysize = t->key->size;
	if(keysize > MAXKEYSIZE) {
		flags |= IndirectKey;
		keysize = sizeof(byte*);
	}
	valuesize = t->elem->size;
	if(valuesize > MAXVALUESIZE) {
		flags |= IndirectValue;
		valuesize = sizeof(byte*);
	}
	bucketsize = offsetof(Bucket, data[0]) + (keysize + valuesize) * BUCKETSIZE;

	// invariants we depend on. 
	// We should probably check these at compile time somewhere,
	// but for now we'll do it here.
	if(t->key->align > BUCKETSIZE) {
		runtime·throw("key align too big");
	}
	if(t->elem->align > BUCKETSIZE) {
		runtime·throw("value align too big");
	}
	if(t->key->size % t->key->align != 0) {
		runtime·throw("key size not a multiple of key align");
	}
	if(t->elem->size % t->elem->align != 0) {
		runtime·throw("value size not a multiple of value align");
	}
	if(BUCKETSIZE < 8) {
		runtime·throw("bucketsize too small for proper alignment");
	}
	if(sizeof(void*) == 4 && t->key->align > 4) {
		runtime·throw("need padding in bucket (key)");
	}
	if(sizeof(void*) == 4 && t->elem->align > 4) {
		runtime·throw("need padding in bucket (value)");
	}
	// 根据传入的 hint 计算出需要的最小需要的桶的数量
	//
	// find size parameter which will hold the requested # of elements
	B = 0;
	while(hint > BUCKETSIZE && hint > LOAD * ((uintptr)1 << B)) {
		B++;
	}

	// allocate initial hash table
	// If hint is large zeroing this memory could take a while.
	if(checkgc) {
		mstats.next_gc = mstats.heap_alloc;
	} 
	if(B == 0) {
		// done lazily later.
		buckets = nil;
	} else {
		buckets = runtime·cnewarray(t->bucket, (uintptr)1 << B);
	}

	// initialize Hmap
	h->count = 0;
	h->B = B;
	h->flags = flags;
	h->keysize = keysize;
	h->valuesize = valuesize;
	h->bucketsize = bucketsize;
	h->hash0 = runtime·fastrand1();
	h->buckets = buckets;
	h->oldbuckets = nil;
	h->nevacuate = 0;
	if(docheck) {
		check(t, h);
	}
}

// Moves entries in oldbuckets[i] to buckets[i] and buckets[i+2^k].
// We leave the original bucket intact, except for the evacuated marks,
// so that iterators can still iterate through the old buckets.
static void evacuate(MapType *t, Hmap *h, uintptr oldbucket)
{
	Bucket *b;
	Bucket *nextb;
	Bucket *x, *y;
	Bucket *newx, *newy;
	uintptr xi, yi;
	uintptr newbit;
	uintptr hash;
	uintptr i;
	byte *k, *v;
	byte *xk, *yk, *xv, *yv;
	uint8 top;
	bool eq;

	b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
	newbit = (uintptr)1 << (h->B - 1);

	if(!evacuated(b)) {
		// TODO: reuse overflow buckets instead of using new ones, if there
		// is no iterator using the old buckets.  (If !OldIterator.)

		x = (Bucket*)(h->buckets + oldbucket * h->bucketsize);
		y = (Bucket*)(h->buckets + (oldbucket + newbit) * h->bucketsize);
		xi = 0;
		yi = 0;
		xk = x->data;
		yk = y->data;
		xv = xk + h->keysize * BUCKETSIZE;
		yv = yk + h->keysize * BUCKETSIZE;
		do {
			for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
				top = b->tophash[i];
				if(top == 0)
					continue;

				// Compute hash to make our evacuation decision (whether we need
				// to send this key/value to bucket x or bucket y).
				hash = h->hash0;
				t->key->alg->hash(&hash, t->key->size, IK(h, k));
				if((h->flags & Iterator) != 0) {
					t->key->alg->equal(&eq, t->key->size, IK(h, k), IK(h, k));
					if(!eq) {
						// If key != key (NaNs), then the hash could be (and probably
						// will be) entirely different from the old hash.  Moreover,
						// it isn't reproducible.  Reproducibility is required in the
						// presence of iterators, as our evacuation decision must
						// match whatever decision the iterator made.
						// Fortunately, we have the freedom to send these keys either
						// way.  Also, tophash is meaningless for these kinds of keys.
						// We let the low bit of tophash drive the evacuation decision.
						// We recompute a new random tophash for the next level so
						// these keys will get evenly distributed across all buckets
						// after multiple grows.
						if((top & 1) != 0)
							hash |= newbit;
						else
							hash &= ~newbit;
						top = hash >> (8*sizeof(uintptr)-8);
						if(top == 0)
							top = 1;
					}
				}

				if((hash & newbit) == 0) {
					if(xi == BUCKETSIZE) {
						if(checkgc) mstats.next_gc = mstats.heap_alloc;
						newx = runtime·cnew(t->bucket);
						x->overflow = newx;
						x = newx;
						xi = 0;
						xk = x->data;
						xv = xk + h->keysize * BUCKETSIZE;
					}
					x->tophash[xi] = top;
					if((h->flags & IndirectKey) != 0) {
						*(byte**)xk = *(byte**)k;               // copy pointer
					} else {
						t->key->alg->copy(t->key->size, xk, k); // copy value
					}
					if((h->flags & IndirectValue) != 0) {
						*(byte**)xv = *(byte**)v;
					} else {
						t->elem->alg->copy(t->elem->size, xv, v);
					}
					xi++;
					xk += h->keysize;
					xv += h->valuesize;
				} else {
					if(yi == BUCKETSIZE) {
						if(checkgc) mstats.next_gc = mstats.heap_alloc;
						newy = runtime·cnew(t->bucket);
						y->overflow = newy;
						y = newy;
						yi = 0;
						yk = y->data;
						yv = yk + h->keysize * BUCKETSIZE;
					}
					y->tophash[yi] = top;
					if((h->flags & IndirectKey) != 0) {
						*(byte**)yk = *(byte**)k;
					} else {
						t->key->alg->copy(t->key->size, yk, k);
					}
					if((h->flags & IndirectValue) != 0) {
						*(byte**)yv = *(byte**)v;
					} else {
						t->elem->alg->copy(t->elem->size, yv, v);
					}
					yi++;
					yk += h->keysize;
					yv += h->valuesize;
				}
			}

			// mark as evacuated so we don't do it again.
			// this also tells any iterators that this data isn't golden anymore.
			nextb = b->overflow;
			b->overflow = (Bucket*)((uintptr)nextb + 1);

			b = nextb;
		} while(b != nil);

		// Unlink the overflow buckets to help GC.
		b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
		if((h->flags & OldIterator) == 0)
			b->overflow = (Bucket*)1;
	}

	// advance evacuation mark
	if(oldbucket == h->nevacuate) {
		h->nevacuate = oldbucket + 1;
		if(oldbucket + 1 == newbit) // newbit == # of oldbuckets
			// free main bucket array
			h->oldbuckets = nil;
	}
	if(docheck)
		check(t, h);
}

static void grow_work(MapType *t, Hmap *h, uintptr bucket)
{
	uintptr noldbuckets;

	noldbuckets = (uintptr)1 << (h->B - 1);

	// make sure we evacuate the oldbucket corresponding
	// to the bucket we're about to use
	evacuate(t, h, bucket & (noldbuckets - 1));

	// evacuate one more oldbucket to make progress on growing
	if(h->oldbuckets != nil)
		evacuate(t, h, h->nevacuate);
}

static void hash_grow(MapType *t, Hmap *h)
{
	byte *old_buckets;
	byte *new_buckets;
	uint8 flags;

	// allocate a bigger hash table
	if(h->oldbuckets != nil)
		runtime·throw("evacuation not done in time");
	old_buckets = h->buckets;
	// NOTE: this could be a big malloc,
	// but since we don't need zeroing it is probably fast.
	if(checkgc) mstats.next_gc = mstats.heap_alloc;
	new_buckets = runtime·cnewarray(t->bucket, (uintptr)1 << (h->B + 1));
	flags = (h->flags & ~(Iterator | OldIterator));
	if((h->flags & Iterator) != 0)
		flags |= OldIterator;

	// commit the grow (atomic wrt gc)
	h->B++;
	h->flags = flags;
	h->oldbuckets = old_buckets;
	h->buckets = new_buckets;
	h->nevacuate = 0;

	// the actual copying of the hash table data is done incrementally
	// by grow_work() and evacuate().
	if(docheck)
		check(t, h);
}

// returns ptr to value associated with key *keyp, or nil if none.
// if it returns non-nil, updates *keyp to point to the currently stored key.
static byte* hash_lookup(MapType *t, Hmap *h, byte **keyp)
{
	void *key;
	uintptr hash;
	uintptr bucket, oldbucket;
	Bucket *b;
	uint8 top;
	uintptr i;
	bool eq;
	byte *k, *k2, *v;

	key = *keyp;
	if(docheck)
		check(t, h);
	if(h->count == 0)
		return nil;
	hash = h->hash0;
	t->key->alg->hash(&hash, t->key->size, key);
	bucket = hash & (((uintptr)1 << h->B) - 1);
	if(h->oldbuckets != nil) {
		oldbucket = bucket & (((uintptr)1 << (h->B - 1)) - 1);
		b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
		if(evacuated(b)) {
			b = (Bucket*)(h->buckets + bucket * h->bucketsize);
		}
	} else {
		b = (Bucket*)(h->buckets + bucket * h->bucketsize);
	}
	top = hash >> (sizeof(uintptr)*8 - 8);
	if(top == 0)
		top = 1;
	do {
		for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
			if(b->tophash[i] == top) {
				k2 = IK(h, k);
				t->key->alg->equal(&eq, t->key->size, key, k2);
				if(eq) {
					*keyp = k2;
					return IV(h, v);
				}
			}
		}
		b = b->overflow;
	} while(b != nil);
	return nil;
}

// When an item is not found, fast versions return a pointer to this zeroed memory.
#pragma dataflag RODATA
static uint8 empty_value[MAXVALUESIZE];

// Specialized versions of mapaccess1 for specific types.
// See ./hashmap_fast.c and ../../cmd/gc/walk.c.
#define HASH_LOOKUP1 runtime·mapaccess1_fast32
#define HASH_LOOKUP2 runtime·mapaccess2_fast32
#define KEYTYPE uint32
#define HASHFUNC runtime·algarray[AMEM32].hash
#define FASTKEY(x) true
#define QUICK_NE(x,y) ((x) != (y))
#define QUICK_EQ(x,y) true
#define SLOW_EQ(x,y) true
#define MAYBE_EQ(x,y) true
#include "hashmap_fast.c"

#undef HASH_LOOKUP1
#undef HASH_LOOKUP2
#undef KEYTYPE
#undef HASHFUNC
#undef FASTKEY
#undef QUICK_NE
#undef QUICK_EQ
#undef SLOW_EQ
#undef MAYBE_EQ

#define HASH_LOOKUP1 runtime·mapaccess1_fast64
#define HASH_LOOKUP2 runtime·mapaccess2_fast64
#define KEYTYPE uint64
#define HASHFUNC runtime·algarray[AMEM64].hash
#define FASTKEY(x) true
#define QUICK_NE(x,y) ((x) != (y))
#define QUICK_EQ(x,y) true
#define SLOW_EQ(x,y) true
#define MAYBE_EQ(x,y) true
#include "hashmap_fast.c"

#undef HASH_LOOKUP1
#undef HASH_LOOKUP2
#undef KEYTYPE
#undef HASHFUNC
#undef FASTKEY
#undef QUICK_NE
#undef QUICK_EQ
#undef SLOW_EQ
#undef MAYBE_EQ

#ifdef GOARCH_amd64
#define CHECKTYPE uint64
#endif
#ifdef GOARCH_386
#define CHECKTYPE uint32
#endif
#ifdef GOARCH_arm
// can't use uint32 on arm because our loads aren't aligned.
// TODO: use uint32 for arm v6+?
#define CHECKTYPE uint8
#endif

#define HASH_LOOKUP1 runtime·mapaccess1_faststr
#define HASH_LOOKUP2 runtime·mapaccess2_faststr
#define KEYTYPE String
#define HASHFUNC runtime·algarray[ASTRING].hash
#define FASTKEY(x) ((x).len < 32)
#define QUICK_NE(x,y) ((x).len != (y).len)
#define QUICK_EQ(x,y) ((x).str == (y).str)
#define SLOW_EQ(x,y) runtime·memeq((x).str, (y).str, (x).len)
#define MAYBE_EQ(x,y) (*(CHECKTYPE*)(x).str == *(CHECKTYPE*)(y).str && *(CHECKTYPE*)((x).str + (x).len - sizeof(CHECKTYPE)) == *(CHECKTYPE*)((y).str + (x).len - sizeof(CHECKTYPE)))
#include "hashmap_fast.c"

// hash_insert 为目标 map 对象设置指定的 key/value 值.
//
// 	@param h: 目标 map 的存储地址
// 	@param key: set map 时, key 的名称(的地址)
// 	@param value: set map 时, value 的值(的地址)
//
// caller:
// 	1. runtime·mapassign() 只有这一处
static void hash_insert(MapType *t, Hmap *h, void *key, void *value)
{
	uintptr hash;
	uintptr bucket;
	uintptr i;
	bool eq;
	Bucket *b;
	Bucket *newb;
	uint8 *inserti;
	byte *insertk, *insertv;
	uint8 top;
	byte *k, *v;
	byte *kmem, *vmem;

	if(docheck) {
		check(t, h);
	}
	// 为目标 key 计算 hash 值
	hash = h->hash0;
	t->key->alg->hash(&hash, t->key->size, key);
	if(h->buckets == nil) {
		// 底层调用 cnew() 创建对象, 这里的参数1, 指的是 
		// src/pkg/runtime/malloc.h -> TypeInfo_Array 变量, 表示创建数组类型.
		h->buckets = runtime·cnewarray(t->bucket, 1);
	}

again:
	// 计算桶的索引 bucket
	bucket = hash & (((uintptr)1 << h->B) - 1);
	if(h->oldbuckets != nil) {
		grow_work(t, h, bucket);
	}
	// 根据桶索引, 在 h->buckets 数组中, 找到桶对象 b
	b = (Bucket*)(h->buckets + bucket * h->bucketsize);
	// 计算 hash 值的高8位信息 top
	top = hash >> (sizeof(uintptr)*8 - 8);
	if(top == 0) {
		top = 1;
	}
	inserti = nil;
	insertk = nil;
	insertv = nil;
	while(true) {
		// 遍历当前 bucket 对象(即变量b)中的所有 slot 槽位, 找到可以插入的位置.
		//
		// k 为 bucket 中存放 key 列表的起始位置, v 则为 value 列表的起始位置.
		// (bucket->data 中, 是以 [key1,key2,..keyN,val1,val2,..valN] 的顺序存放的)
		// 存放在同一 bucket 中的数据, 都是因为 key 计算出来的 hash 值出现了"哈希冲突"
		for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
			// 如果当前槽位的 tophash 值与待插入 key 的 top 计算值不等, 有2种情况
			// 1. 该槽位为 0, 即为空
			// 2. 该槽位被其他 hash 值相同的 key 占用
			if(b->tophash[i] != top) {
				// tophash[i] == 0 表示该处的槽位为空, 可以写入, 则先记录位置, 并不赋值.
				if(b->tophash[i] == 0 && inserti == nil) {
					inserti = &b->tophash[i];
					insertk = k;
					insertv = v;
				}
				continue;
			}
			t->key->alg->equal(&eq, t->key->size, key, IK(h, k));
			if(!eq) {
				continue;
			}
			// already have a mapping for key.  Update it.
			// Need to update key for keys which are distinct but equal
			// (e.g. +0.0 and -0.0)
			t->key->alg->copy(t->key->size, IK(h, k), key); 
			t->elem->alg->copy(t->elem->size, IV(h, v), value);
			if(docheck) {
				check(t, h);
			}
			return;
		}
		if(b->overflow == nil) {
			break;
		}
		b = b->overflow;
	}

	// did not find mapping for key.  Allocate new cell & add entry.
	if(h->count >= LOAD * ((uintptr)1 << h->B) && h->count >= BUCKETSIZE) {
		hash_grow(t, h);
		// Growing the table invalidates everything, so try again
		goto again; 
	}

	if(inserti == nil) {
		// all current buckets are full, allocate a new one.
		if(checkgc) {
			mstats.next_gc = mstats.heap_alloc;
		} 
		newb = runtime·cnew(t->bucket);
		b->overflow = newb;
		inserti = newb->tophash;
		insertk = newb->data;
		insertv = insertk + h->keysize * BUCKETSIZE;
	}

	// store new key/value at insert position
	if((h->flags & IndirectKey) != 0) {
		if(checkgc) {
			mstats.next_gc = mstats.heap_alloc;
		} 
		kmem = runtime·cnew(t->key);
		*(byte**)insertk = kmem;
		insertk = kmem;
	}
	if((h->flags & IndirectValue) != 0) {
		if(checkgc) {
			mstats.next_gc = mstats.heap_alloc;
		} 
		vmem = runtime·cnew(t->elem);
		*(byte**)insertv = vmem;
		insertv = vmem;
	}
	t->key->alg->copy(t->key->size, insertk, key);
	t->elem->alg->copy(t->elem->size, insertv, value);
	*inserti = top;
	h->count++;
	if(docheck) {
		check(t, h);
	}
}

static void hash_remove(MapType *t, Hmap *h, void *key)
{
	uintptr hash;
	uintptr bucket;
	Bucket *b;
	uint8 top;
	uintptr i;
	byte *k, *v;
	bool eq;
	
	if(docheck) {
		check(t, h);
	}
	if(h->count == 0) {
		return;
	}
	hash = h->hash0;
	t->key->alg->hash(&hash, t->key->size, key);
	bucket = hash & (((uintptr)1 << h->B) - 1);
	if(h->oldbuckets != nil) {
		grow_work(t, h, bucket);
	}
	b = (Bucket*)(h->buckets + bucket * h->bucketsize);
	top = hash >> (sizeof(uintptr)*8 - 8);
	if(top == 0) {
		top = 1;
	}
	do {
		for(i = 0, k = b->data, v = k + h->keysize * BUCKETSIZE; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
			if(b->tophash[i] != top)
				continue;
			t->key->alg->equal(&eq, t->key->size, key, IK(h, k));
			if(!eq)
				continue;

			if((h->flags & IndirectKey) != 0) {
				*(byte**)k = nil;
			} else {
				t->key->alg->copy(t->key->size, k, nil);
			}
			if((h->flags & IndirectValue) != 0) {
				*(byte**)v = nil;
			} else {
				t->elem->alg->copy(t->elem->size, v, nil);
			}

			b->tophash[i] = 0;
			h->count--;
			
			// TODO: consolidate buckets if they are mostly empty
			// can only consolidate if there are no live iterators at this size.
			if(docheck)
				check(t, h);
			return;
		}
		b = b->overflow;
	} while(b != nil);
}

// TODO: shrink the map, the same way we grow it.

// If you modify hash_iter, also change cmd/gc/range.c to indicate
// the size of this structure.
struct hash_iter
{
	// Must be in first position. 
	// Write nil to indicate iteration end (see cmd/gc/range.c).
	uint8* key;
	uint8* value;

	MapType *t;
	Hmap *h;

	// end point for iteration
	uintptr endbucket;
	bool wrapped;

	// state of table at time iterator is initialized
	uint8 B;
	byte *buckets;

	// iter state
	uintptr bucket;
	struct Bucket *bptr;
	uintptr i;
	intptr check_bucket;
};

// iterator state:
// bucket: the current bucket ID
// b: the current Bucket in the chain
// i: the next offset to check in the current bucket
static void hash_iter_init(MapType *t, Hmap *h, struct hash_iter *it)
{
	uint32 old;

	if(sizeof(struct hash_iter) / sizeof(uintptr) != 11) {
		// see ../../cmd/gc/range.c
		runtime·throw("hash_iter size incorrect"); 
	}
	it->t = t;
	it->h = h;

	// grab snapshot of bucket state
	it->B = h->B;
	it->buckets = h->buckets;

	// iterator state
	it->bucket = it->endbucket = runtime·fastrand1() & (((uintptr)1 << h->B) - 1);
	it->wrapped = false;
	it->bptr = nil;

	// Remember we have an iterator.
	// Can run concurrently with another hash_iter_init().
	for(;;) {
		old = h->flags;
		if((old&(Iterator|OldIterator)) == (Iterator|OldIterator)) {
			break;
		}
		if(runtime·cas(&h->flags, old, old|Iterator|OldIterator)) {
			break;
		}
	}

	if(h->buckets == nil) {
		// Empty map. Force next hash_next to exit without
		// evalulating h->bucket.
		it->wrapped = true;
	}
}

// initializes it->key and it->value to the next key/value pair
// in the iteration, or nil if we've reached the end.
static void hash_next(struct hash_iter *it)
{
	Hmap *h;
	MapType *t;
	uintptr bucket, oldbucket;
	uintptr hash;
	Bucket *b;
	uintptr i;
	intptr check_bucket;
	bool eq;
	byte *k, *v;
	byte *rk, *rv;

	h = it->h;
	t = it->t;
	bucket = it->bucket;
	b = it->bptr;
	i = it->i;
	check_bucket = it->check_bucket;

next:
	if(b == nil) {
		if(bucket == it->endbucket && it->wrapped) {
			// end of iteration
			it->key = nil;
			it->value = nil;
			return;
		}
		if(h->oldbuckets != nil && it->B == h->B) {
			// Iterator was started in the middle of a grow, and the grow isn't done yet.
			// If the bucket we're looking at hasn't been filled in yet (i.e. the old
			// bucket hasn't been evacuated) then we need to iterate through the old
			// bucket and only return the ones that will be migrated to this bucket.
			oldbucket = bucket & (((uintptr)1 << (it->B - 1)) - 1);
			b = (Bucket*)(h->oldbuckets + oldbucket * h->bucketsize);
			if(!evacuated(b)) {
				check_bucket = bucket;
			} else {
				b = (Bucket*)(it->buckets + bucket * h->bucketsize);
				check_bucket = -1;
			}
		} else {
			b = (Bucket*)(it->buckets + bucket * h->bucketsize);
			check_bucket = -1;
		}
		bucket++;
		if(bucket == ((uintptr)1 << it->B)) {
			bucket = 0;
			it->wrapped = true;
		}
		i = 0;
	}
	k = b->data + h->keysize * i;
	v = b->data + h->keysize * BUCKETSIZE + h->valuesize * i;
	for(; i < BUCKETSIZE; i++, k += h->keysize, v += h->valuesize) {
		if(b->tophash[i] != 0) {
			if(check_bucket >= 0) {
				// Special case: iterator was started during a grow and the
				// grow is not done yet.  We're working on a bucket whose
				// oldbucket has not been evacuated yet.  So we're iterating
				// through the oldbucket, skipping any keys that will go
				// to the other new bucket (each oldbucket expands to two
				// buckets during a grow).
				t->key->alg->equal(&eq, t->key->size, IK(h, k), IK(h, k));
				if(eq) {
					// If the item in the oldbucket is not destined for
					// the current new bucket in the iteration, skip it.
					hash = h->hash0;
					t->key->alg->hash(&hash, t->key->size, IK(h, k));
					if((hash & (((uintptr)1 << it->B) - 1)) != check_bucket) {
						continue;
					}
				} else {
					// Hash isn't repeatable if k != k (NaNs).  We need a
					// repeatable and randomish choice of which direction
					// to send NaNs during evacuation.  We'll use the low
					// bit of tophash to decide which way NaNs go.
					if(check_bucket >> (it->B - 1) != (b->tophash[i] & 1)) {
						continue;
					}
				}
			}
			if(!evacuated(b)) {
				// this is the golden data, we can return it.
				it->key = IK(h, k);
				it->value = IV(h, v);
			} else {
				// The hash table has grown since the iterator was started.
				// The golden data for this key is now somewhere else.
				t->key->alg->equal(&eq, t->key->size, IK(h, k), IK(h, k));
				if(eq) {
					// Check the current hash table for the data.
					// This code handles the case where the key
					// has been deleted, updated, or deleted and reinserted.
					// NOTE: we need to regrab the key as it has potentially been
					// updated to an equal() but not identical key (e.g. +0.0 vs -0.0).
					rk = IK(h, k);
					rv = hash_lookup(t, it->h, &rk);
					if(rv == nil)
						continue; // key has been deleted
					it->key = rk;
					it->value = rv;
				} else {
					// if key!=key then the entry can't be deleted or
					// updated, so we can just return it.  That's lucky for
					// us because when key!=key we can't look it up
					// successfully in the current table.
					it->key = IK(h, k);
					it->value = IV(h, v);
				}
			}
			it->bucket = bucket;
			it->bptr = b;
			it->i = i + 1;
			it->check_bucket = check_bucket;
			return;
		}
	}
	b = overflowptr(b);
	i = 0;
	goto next;
}

//
/// interfaces to go runtime
//

void reflect·ismapkey(Type *typ, bool ret)
{
	ret = typ != nil && typ->alg->hash != runtime·nohash;
	FLUSH(&ret);
}

// runtime·makemap_c ...
//
// 	@param hint: make(map[string]string, hint)的第2个参数, 表示该 map 对象的容量
//
// caller:
// 	1. runtime·makemap()
//
// 	@compatible: 在 v1.5 版本中, 函数原型发生了变动, 成为了
// func makemap(t *maptype, hint int64, h *hmap, bucket unsafe.Pointer) *hmap {}
// 见 [go v1.5 hashmap.go](https://github.com/golang/go/tree/go1.5/src/runtime/hashmap.go#187L)
//
Hmap* runtime·makemap_c(MapType *typ, int64 hint)
{
	Hmap *h;
	Type *key;

	key = typ->key;

	if(sizeof(Hmap) > 48) {
		runtime·panicstring("hmap too large");
	}

	if(hint < 0 || (int32)hint != hint) {
		runtime·panicstring("makemap: size out of range");
	}

	if(key->alg->hash == runtime·nohash) {
		runtime·throw("runtime.makemap: unsupported map key type");
	}

	h = runtime·cnew(typ->hmap);
	hash_init(typ, h, hint);

	// these calculations are compiler dependent.
	// figure out offsets of map call arguments.

	if(debug) {
		runtime·printf(
			"makemap: map=%p; keysize=%p; valsize=%p; keyalg=%p; valalg=%p\n",
			h, key->size, typ->elem->size, key->alg, typ->elem->alg
		);
	}

	return h;
}

// golang原生: make(map[]interface{})
//
// 	@param hint: make(map[string]string, hint)的第2个参数, 表示 map 对象的容量
//
// makemap(key, val *Type, hint int64) (hmap *map[any]any);
void runtime·makemap(MapType *typ, int64 hint, Hmap *ret)
{
	ret = runtime·makemap_c(typ, hint);
	FLUSH(&ret);
}

// For reflect:
//	func makemap(Type *mapType) (hmap *map)
void reflect·makemap(MapType *t, Hmap *ret)
{
	ret = runtime·makemap_c(t, 0);
	FLUSH(&ret);
}

//
// 	@implementOf: src/pkg/reflect/value_compatible.go -> makemap_v1_9()
//
// For reflect:
//	func makemap(Type *mapType) (hmap *map)
void reflect·makemap_v1_9(MapType *t, int64 hint, Hmap *ret)
{
	ret = runtime·makemap_c(t, hint);
	FLUSH(&ret);
}

// 这里不是获取 map 中某个 key 的操作, 真正的 get 函数在
// src/pkg/runtime/hashmap_fast.c -> HASH_LOOKUP1()
void runtime·mapaccess(MapType *t, Hmap *h, byte *ak, byte *av, bool *pres)
{
	byte *res;
	Type *elem;

	elem = t->elem;
	if(h == nil || h->count == 0) {
		elem->alg->copy(elem->size, av, nil);
		*pres = false;
		return;
	}

	res = hash_lookup(t, h, &ak);

	if(res != nil) {
		*pres = true;
		elem->alg->copy(elem->size, av, res);
	} else {
		*pres = false;
		elem->alg->copy(elem->size, av, nil);
	}
}

// v := map[key], 区别于 runtime·mapaccess2
//
// mapaccess1(hmap *map[any]any, key any) (val any);
#pragma textflag NOSPLIT
void runtime·mapaccess1(MapType *t, Hmap *h, ...)
{
	byte *ak, *av;
	byte *res;

	if(raceenabled && h != nil) {
		runtime·racereadpc(h, runtime·getcallerpc(&t), runtime·mapaccess1);
	}

	ak = (byte*)(&h + 1);
	av = ak + ROUND(t->key->size, Structrnd);

	if(h == nil || h->count == 0) {
		t->elem->alg->copy(t->elem->size, av, nil);
	} else {
		res = hash_lookup(t, h, &ak);
		t->elem->alg->copy(t->elem->size, av, res);
	}

	if(debug) {
		runtime·prints("runtime.mapaccess1: map=");
		runtime·printpointer(h);
		runtime·prints("; key=");
		t->key->alg->print(t->key->size, ak);
		runtime·prints("; val=");
		t->elem->alg->print(t->elem->size, av);
		runtime·prints("\n");
	}
}

// v, ok := map[key], 区别于 runtime·mapaccess1
//
// mapaccess2(hmap *map[any]any, key any) (val any, pres bool);
#pragma textflag NOSPLIT
void runtime·mapaccess2(MapType *t, Hmap *h, ...)
{
	byte *ak, *av, *ap;

	if(raceenabled && h != nil)
		runtime·racereadpc(h, runtime·getcallerpc(&t), runtime·mapaccess2);

	ak = (byte*)(&h + 1);
	av = ak + ROUND(t->key->size, Structrnd);
	ap = av + t->elem->size;

	runtime·mapaccess(t, h, ak, av, ap);

	if(debug) {
		runtime·prints("runtime.mapaccess2: map=");
		runtime·printpointer(h);
		runtime·prints("; key=");
		t->key->alg->print(t->key->size, ak);
		runtime·prints("; val=");
		t->elem->alg->print(t->elem->size, av);
		runtime·prints("; pres=");
		runtime·printbool(*ap);
		runtime·prints("\n");
	}
}

// For reflect:
//	func mapaccess(t type, h map, key iword) (val iword, pres bool)
// where an iword is the same word an interface value would use:
// the actual data if it fits, or else a pointer to the data.
void reflect·mapaccess(MapType *t, Hmap *h, uintptr key, uintptr val, bool pres)
{
	byte *ak, *av;

	if(raceenabled && h != nil)
		runtime·racereadpc(h, runtime·getcallerpc(&t), reflect·mapaccess);

	if(t->key->size <= sizeof(key))
		ak = (byte*)&key;
	else
		ak = (byte*)key;
	val = 0;
	pres = false;
	if(t->elem->size <= sizeof(val))
		av = (byte*)&val;
	else {
		av = runtime·mal(t->elem->size);
		val = (uintptr)av;
	}
	runtime·mapaccess(t, h, ak, av, &pres);
	FLUSH(&val);
	FLUSH(&pres);
}

// runtime·mapassign 为目标 map 对象设置指定的 key/value 值.
//
// 	@param h: 目标 map 的存储地址
// 	@param ak: arguement key, set map 时, key 的名称(的地址)
// 	@param av: arguement value, set map 时, value 的值(的地址)
void runtime·mapassign(MapType *t, Hmap *h, byte *ak, byte *av)
{
	if(h == nil) {
		runtime·panicstring("assignment to entry in nil map");
	}

	if(av == nil) {
		// av 为 nil, 表示将目标 key 从 map 中移除
		hash_remove(t, h, ak);
	} else {
		hash_insert(t, h, ak, av);
	}

	if(debug) {
		runtime·prints("mapassign: map=");
		runtime·printpointer(h);
		runtime·prints("; key=");
		t->key->alg->print(t->key->size, ak);
		runtime·prints("; val=");
		if(av) {
			t->elem->alg->print(t->elem->size, av);
		}
		else {
			runtime·prints("nil");
		}
		runtime·prints("\n");
	}
}

// golang原生: map["key"] = "value" 在 map 中添加 key/value 对.
//
// mapassign1(mapType *type, hmap *map[any]any, key any, val any);
#pragma textflag NOSPLIT
void runtime·mapassign1(MapType *t, Hmap *h, ...)
{
	byte *ak, *av;

	if(h == nil) {
		runtime·panicstring("assignment to entry in nil map");
	}

	if(raceenabled) {
		runtime·racewritepc(h, runtime·getcallerpc(&t), runtime·mapassign1);
	}
	// todo: 这个计算方式什么意思???
	// ak: arguement key, set map 时, key 的名称(的地址)
	ak = (byte*)(&h + 1);
	// av: arguement value, set map 时, value 的值(的地址)
	av = ak + ROUND(t->key->size, t->elem->align);

	runtime·mapassign(t, h, ak, av);
}

// mapdelete(mapType *type, hmap *map[any]any, key any)
#pragma textflag NOSPLIT
void runtime·mapdelete(MapType *t, Hmap *h, ...)
{
	byte *ak;

	if(h == nil) {
		return;
	}

	if(raceenabled) {
		runtime·racewritepc(h, runtime·getcallerpc(&t), runtime·mapdelete);
	}
	ak = (byte*)(&h + 1);
	runtime·mapassign(t, h, ak, nil);

	if(debug) {
		runtime·prints("mapdelete: map=");
		runtime·printpointer(h);
		runtime·prints("; key=");
		t->key->alg->print(t->key->size, ak);
		runtime·prints("\n");
	}
}

// For reflect:
//	func mapassign(t type h map, key, val iword, pres bool)
// where an iword is the same word an interface value would use:
// the actual data if it fits, or else a pointer to the data.
void reflect·mapassign(MapType *t, Hmap *h, uintptr key, uintptr val, bool pres)
{
	byte *ak, *av;

	if(h == nil)
		runtime·panicstring("assignment to entry in nil map");
	if(raceenabled)
		runtime·racewritepc(h, runtime·getcallerpc(&t), reflect·mapassign);
	if(t->key->size <= sizeof(key))
		ak = (byte*)&key;
	else
		ak = (byte*)key;
	if(t->elem->size <= sizeof(val))
		av = (byte*)&val;
	else
		av = (byte*)val;
	if(!pres)
		av = nil;
	runtime·mapassign(t, h, ak, av);
}

// mapiterinit(mapType *type, hmap *map[any]any, hiter *any);
void runtime·mapiterinit(MapType *t, Hmap *h, struct hash_iter *it)
{
	if(h == nil || h->count == 0) {
		it->key = nil;
		return;
	}
	if(raceenabled)
		runtime·racereadpc(h, runtime·getcallerpc(&t), runtime·mapiterinit);
	hash_iter_init(t, h, it);
	hash_next(it);
	if(debug) {
		runtime·prints("runtime.mapiterinit: map=");
		runtime·printpointer(h);
		runtime·prints("; iter=");
		runtime·printpointer(it);
		runtime·prints("; key=");
		runtime·printpointer(it->key);
		runtime·prints("\n");
	}
}

// For reflect:
//	func mapiterinit(h map) (it iter)
void reflect·mapiterinit(MapType *t, Hmap *h, struct hash_iter *it)
{
	it = runtime·mal(sizeof *it);
	FLUSH(&it);
	runtime·mapiterinit(t, h, it);
}

// mapiternext(hiter *any);
void runtime·mapiternext(struct hash_iter *it)
{
	if(raceenabled) {
		runtime·racereadpc(it->h, runtime·getcallerpc(&it), runtime·mapiternext);
	}

	hash_next(it);
	if(debug) {
		runtime·prints("runtime.mapiternext: iter=");
		runtime·printpointer(it);
		runtime·prints("; key=");
		runtime·printpointer(it->key);
		runtime·prints("\n");
	}
}

// For reflect:
//	func mapiternext(it iter)
void reflect·mapiternext(struct hash_iter *it)
{
	runtime·mapiternext(it);
}

// mapiter1(hiter *any) (key any);
#pragma textflag NOSPLIT
void runtime·mapiter1(struct hash_iter *it, ...)
{
	byte *ak, *res;
	Type *key;

	ak = (byte*)(&it + 1);

	res = it->key;
	if(res == nil) {
		runtime·throw("runtime.mapiter1: key:val nil pointer");
	}

	key = it->t->key;
	key->alg->copy(key->size, ak, res);

	if(debug) {
		runtime·prints("mapiter1: iter=");
		runtime·printpointer(it);
		runtime·prints("; map=");
		runtime·printpointer(it->h);
		runtime·prints("\n");
	}
}

bool runtime·mapiterkey(struct hash_iter *it, void *ak)
{
	byte *res;
	Type *key;

	res = it->key;
	if(res == nil)
		return false;
	key = it->t->key;
	key->alg->copy(key->size, ak, res);
	return true;
}

// For reflect:
//	func mapiterkey(h map) (key iword, ok bool)
// where an iword is the same word an interface value would use:
// the actual data if it fits, or else a pointer to the data.
void reflect·mapiterkey(struct hash_iter *it, uintptr key, bool ok)
{
	byte *res;
	Type *tkey;

	key = 0;
	ok = false;
	res = it->key;
	if(res != nil) {
		tkey = it->t->key;
		if(tkey->size <= sizeof(key))
			tkey->alg->copy(tkey->size, (byte*)&key, res);
		else
			key = (uintptr)res;
		ok = true;
	}
	FLUSH(&key);
	FLUSH(&ok);
}

// For reflect:
//	func maplen(h map) (len int)
// Like len(m) in the actual language, we treat the nil map as length 0.
void reflect·maplen(Hmap *h, intgo len)
{
	if(h == nil) {
		len = 0;
	}
	else {
		len = h->count;
		if(raceenabled)
			runtime·racereadpc(h, runtime·getcallerpc(&h), reflect·maplen);
	}
	FLUSH(&len);
}

// mapiter2(hiter *any) (key any, val any);
#pragma textflag NOSPLIT
void runtime·mapiter2(struct hash_iter *it, ...)
{
	byte *ak, *av, *res;
	MapType *t;

	t = it->t;
	ak = (byte*)(&it + 1);
	av = ak + ROUND(t->key->size, t->elem->align);

	res = it->key;
	if(res == nil)
		runtime·throw("runtime.mapiter2: key:val nil pointer");

	t->key->alg->copy(t->key->size, ak, res);
	t->elem->alg->copy(t->elem->size, av, it->value);

	if(debug) {
		runtime·prints("mapiter2: iter=");
		runtime·printpointer(it);
		runtime·prints("; map=");
		runtime·printpointer(it->h);
		runtime·prints("\n");
	}
}

// exported value for testing
float64 runtime·hashLoad = LOAD;
