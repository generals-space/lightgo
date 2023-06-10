#include	"subr.h"

int algtype1(Type *t, Type **bad)
{
	int a, ret;
	Type *t1;
	
	if(bad)
		*bad = T;

	if(t->noalg)
		return ANOEQ;

	switch(t->etype) {
	case TANY:
	case TFORW:
		// will be defined later.
		*bad = t;
		return -1;

	case TINT8:
	case TUINT8:
	case TINT16:
	case TUINT16:
	case TINT32:
	case TUINT32:
	case TINT64:
	case TUINT64:
	case TINT:
	case TUINT:
	case TUINTPTR:
	case TBOOL:
	case TPTR32:
	case TPTR64:
	case TCHAN:
	case TUNSAFEPTR:
		return AMEM;

	case TFUNC:
	case TMAP:
		if(bad)
			*bad = t;
		return ANOEQ;

	case TFLOAT32:
		return AFLOAT32;

	case TFLOAT64:
		return AFLOAT64;

	case TCOMPLEX64:
		return ACPLX64;

	case TCOMPLEX128:
		return ACPLX128;

	case TSTRING:
		return ASTRING;
	
	case TINTER:
		if(isnilinter(t))
			return ANILINTER;
		return AINTER;
	
	case TARRAY:
		if(isslice(t)) {
			if(bad)
				*bad = t;
			return ANOEQ;
		}
		if(t->bound == 0)
			return AMEM;
		a = algtype1(t->type, bad);
		if(a == ANOEQ || a == AMEM) {
			if(a == ANOEQ && bad)
				*bad = t;
			return a;
		}
		return -1;  // needs special compare

	case TSTRUCT:
		if(t->type != T && t->type->down == T && !isblanksym(t->type->sym)) {
			// One-field struct is same as that one field alone.
			return algtype1(t->type->type, bad);
		}
		ret = AMEM;
		for(t1=t->type; t1!=T; t1=t1->down) {
			// All fields must be comparable.
			a = algtype1(t1->type, bad);
			if(a == ANOEQ)
				return ANOEQ;

			// Blank fields, padded fields, fields with non-memory
			// equality need special compare.
			if(a != AMEM || isblanksym(t1->sym) || ispaddedfield(t1, t->width)) {
				ret = -1;
				continue;
			}
		}
		return ret;
	}

	fatal("algtype1: unexpected type %T", t);
	return 0;
}

Type* maptype(Type *key, Type *val)
{
	Type *t;
	Type *bad;
	int atype;

	if(key != nil) {
		atype = algtype1(key, &bad);
		switch(bad == T ? key->etype : bad->etype) {
		default:
			if(atype == ANOEQ)
				yyerror("invalid map key type %T", key);
			break;
		case TANY:
			// will be resolved later.
			break;
		case TFORW:
			// map[key] used during definition of key.
			// postpone check until key is fully defined.
			// if there are multiple uses of map[key]
			// before key is fully defined, the error
			// will only be printed for the first one.
			// good enough.
			if(key->maplineno == 0)
				key->maplineno = lineno;
			break;
		}
	}
	t = typ(TMAP);
	t->down = key;
	t->type = val;
	return t;
}
