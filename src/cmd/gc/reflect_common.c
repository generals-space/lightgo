#include "reflect.h"

static	Sym*	weaktypesym(Type*);
static	Sym*	dalgsym(Type*);

static int kinds[] =
{
	[TINT]		= KindInt,
	[TUINT]		= KindUint,
	[TINT8]		= KindInt8,
	[TUINT8]	= KindUint8,
	[TINT16]	= KindInt16,
	[TUINT16]	= KindUint16,
	[TINT32]	= KindInt32,
	[TUINT32]	= KindUint32,
	[TINT64]	= KindInt64,
	[TUINT64]	= KindUint64,
	[TUINTPTR]	= KindUintptr,
	[TFLOAT32]	= KindFloat32,
	[TFLOAT64]	= KindFloat64,
	[TBOOL]		= KindBool,
	[TSTRING]		= KindString,
	[TPTR32]		= KindPtr,
	[TPTR64]		= KindPtr,
	[TSTRUCT]	= KindStruct,
	[TINTER]		= KindInterface,
	[TCHAN]		= KindChan,
	[TMAP]		= KindMap,
	[TARRAY]		= KindArray,
	[TFUNC]		= KindFunc,
	[TCOMPLEX64]	= KindComplex64,
	[TCOMPLEX128]	= KindComplex128,
	[TUNSAFEPTR]	= KindUnsafePointer,
};

// 
// commonType
// ../../pkg/runtime/type.go:/commonType
// 
// caller:
// 	1. dtypesym() 只有这一处
int dcommontype(Sym *s, int ot, Type *t)
{
	int i, alg, sizeofAlg;
	Sym *sptr, *algsym;
	static Sym *algarray;
	char *p;
	
	if(ot != 0)
		fatal("dcommontype %d", ot);

	sizeofAlg = 4*widthptr;
	if(algarray == nil)
		algarray = pkglookup("algarray", runtimepkg);
	alg = algtype(t);
	algsym = S;
	if(alg < 0)
		algsym = dalgsym(t);

	dowidth(t);
	if(t->sym != nil && !isptr[t->etype])
		sptr = dtypesym(ptrto(t));
	else
		sptr = weaktypesym(ptrto(t));

	// ../../pkg/reflect/type.go:/^type.commonType
	// actual type structure
	//	type commonType struct {
	//		size          uintptr
	//		hash          uint32
	//		_             uint8
	//		align         uint8
	//		fieldAlign    uint8
	//		kind          uint8
	//		alg           unsafe.Pointer
	//		gc            unsafe.Pointer
	//		string        *string
	//		*extraType
	//		ptrToThis     *Type
	//	}
	ot = duintptr(s, ot, t->width);
	ot = duint32(s, ot, typehash(t));
	ot = duint8(s, ot, 0);	// unused

	// runtime (and common sense) expects alignment to be a power of two.
	i = t->align;
	if(i == 0)
		i = 1;
	if((i&(i-1)) != 0)
		fatal("invalid alignment %d for %T", t->align, t);
	ot = duint8(s, ot, t->align);	// align
	ot = duint8(s, ot, t->align);	// fieldAlign

	i = kinds[t->etype];
	if(t->etype == TARRAY && t->bound < 0)
		i = KindSlice;
	if(!haspointers(t))
		i |= KindNoPointers;
	ot = duint8(s, ot, i);  // kind
	if(alg >= 0)
		ot = dsymptr(s, ot, algarray, alg*sizeofAlg);
	else
		ot = dsymptr(s, ot, algsym, 0);
	ot = dsymptr(s, ot, dgcsym(t), 0);  // gc
	p = smprint("%-uT", t);
	//print("dcommontype: %s\n", p);
	ot = dgostringptr(s, ot, p);	// string
	free(p);

	// skip pointer to extraType,
	// which follows the rest of this type structure.
	// caller will fill in if needed.
	// otherwise linker will assume 0.
	ot += widthptr;

	ot = dsymptr(s, ot, sptr, 0);  // ptrto type
	return ot;
}

static Sym* weaktypesym(Type *t)
{
	char *p;
	Sym *s;

	p = smprint("%-T", t);
	s = pkglookup(p, weaktypepkg);
	//print("weaktypesym: %s -> %+S\n", p, s);
	free(p);
	return s;
}

static Sym* dalgsym(Type *t)
{
	int ot;
	Sym *s, *hash, *eq;
	char buf[100];

	// dalgsym is only called for a type that needs an algorithm table,
	// which implies that the type is comparable (or else it would use ANOEQ).

	s = typesymprefix(".alg", t);
	hash = typesymprefix(".hash", t);
	genhash(hash, t);
	eq = typesymprefix(".eq", t);
	geneq(eq, t);

	// ../../pkg/runtime/runtime.h:/Alg
	ot = 0;
	ot = dsymptr(s, ot, hash, 0);
	ot = dsymptr(s, ot, eq, 0);
	ot = dsymptr(s, ot, pkglookup("memprint", runtimepkg), 0);
	switch(t->width) {
	default:
		ot = dsymptr(s, ot, pkglookup("memcopy", runtimepkg), 0);
		break;
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		snprint(buf, sizeof buf, "memcopy%d", (int)t->width*8);
		ot = dsymptr(s, ot, pkglookup(buf, runtimepkg), 0);
		break;
	}

	ggloblsym(s, ot, 1, 1);
	return s;
}
