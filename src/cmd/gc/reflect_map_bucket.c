#include "reflect.h"

// Builds a type respresenting a Bucket structure for
// the given map type.  This type is not visible to users -
// we include only enough information to generate a correct GC
// program for it.
// Make sure this stays in sync with ../../pkg/runtime/hashmap.c!
enum {
	BUCKETSIZE = 8,
	MAXKEYSIZE = 128,
	MAXVALSIZE = 128,
};

Type* mapbucket(Type *t)
{
	Type *keytype, *valtype;
	Type *bucket;
	Type *overflowfield, *keysfield, *valuesfield;
	int32 offset;

	if(t->bucket != T)
		return t->bucket;

	keytype = t->down;
	valtype = t->type;
	if(keytype->width > MAXKEYSIZE)
		keytype = ptrto(keytype);
	if(valtype->width > MAXVALSIZE)
		valtype = ptrto(valtype);

	bucket = typ(TSTRUCT);
	bucket->noalg = 1;

	// The first field is: uint8 topbits[BUCKETSIZE].
	// We don't need to encode it as GC doesn't care about it.
	offset = BUCKETSIZE * 1;

	overflowfield = typ(TFIELD);
	overflowfield->type = ptrto(bucket);
	overflowfield->width = offset;         // "width" is offset in structure
	overflowfield->sym = mal(sizeof(Sym)); // not important but needs to be set to give this type a name
	overflowfield->sym->name = "overflow";
	offset += widthptr;

	keysfield = typ(TFIELD);
	keysfield->type = typ(TARRAY);
	keysfield->type->type = keytype;
	keysfield->type->bound = BUCKETSIZE;
	keysfield->type->width = BUCKETSIZE * keytype->width;
	keysfield->width = offset;
	keysfield->sym = mal(sizeof(Sym));
	keysfield->sym->name = "keys";
	offset += BUCKETSIZE * keytype->width;

	valuesfield = typ(TFIELD);
	valuesfield->type = typ(TARRAY);
	valuesfield->type->type = valtype;
	valuesfield->type->bound = BUCKETSIZE;
	valuesfield->type->width = BUCKETSIZE * valtype->width;
	valuesfield->width = offset;
	valuesfield->sym = mal(sizeof(Sym));
	valuesfield->sym->name = "values";
	offset += BUCKETSIZE * valtype->width;

	// link up fields
	bucket->type = overflowfield;
	overflowfield->down = keysfield;
	keysfield->down = valuesfield;
	valuesfield->down = T;

	bucket->width = offset;
	bucket->local = t->local;
	t->bucket = bucket;
	return bucket;
}

// Builds a type respresenting a Hmap structure for
// the given map type.  This type is not visible to users -
// we include only enough information to generate a correct GC
// program for it.
// Make sure this stays in sync with ../../pkg/runtime/hashmap.c!
Type* hmap(Type *t)
{
	Type *h, *bucket;
	Type *bucketsfield, *oldbucketsfield;
	int32 offset;

	if(t->hmap != T)
		return t->hmap;

	bucket = mapbucket(t);
	h = typ(TSTRUCT);
	h->noalg = 1;

	offset = widthint; // count
	offset += 4;       // flags
	offset += 4;       // hash0
	offset += 1;       // B
	offset += 1;       // keysize
	offset += 1;       // valuesize
	offset = (offset + 1) / 2 * 2;
	offset += 2;       // bucketsize
	offset = (offset + widthptr - 1) / widthptr * widthptr;
	
	bucketsfield = typ(TFIELD);
	bucketsfield->type = ptrto(bucket);
	bucketsfield->width = offset;
	bucketsfield->sym = mal(sizeof(Sym));
	bucketsfield->sym->name = "buckets";
	offset += widthptr;

	oldbucketsfield = typ(TFIELD);
	oldbucketsfield->type = ptrto(bucket);
	oldbucketsfield->width = offset;
	oldbucketsfield->sym = mal(sizeof(Sym));
	oldbucketsfield->sym->name = "oldbuckets";
	offset += widthptr;

	offset += widthptr; // nevacuate (last field in Hmap)

	// link up fields
	h->type = bucketsfield;
	bucketsfield->down = oldbucketsfield;
	oldbucketsfield->down = T;

	h->width = offset;
	h->local = t->local;
	t->hmap = h;
	h->hmap = t;
	return h;
}
