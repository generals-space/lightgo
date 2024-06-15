// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <u.h>
#include <libc.h>
#include "reflect.h"
#include "../../pkg/runtime/mgc0.h"
#include "../../pkg/runtime/mgc0__stats.h"
#include "../../pkg/runtime/mgc0__funcs.h"

/*
 * runtime interface and reflection data structures
 */

NodeList*	signatlist;

// caller: 
// 	1. src/cmd/gc/obj.c -> dumpobj() 只有这一处.
// 	使用 6g 命令进行源码编译, 输出 main.6 OBJ文件时, 会调用本函数.
void dumptypestructs(void)
{
	int i;
	NodeList *l;
	Node *n;
	Type *t;
	Pkg *p;

	// copy types from externdcl list to signatlist
	for(l=externdcl; l; l=l->next) {
		n = l->n;
		if(n->op != OTYPE) {
			continue;
		}
		signatlist = list(signatlist, n);
	}

	// process signatlist
	for(l=signatlist; l; l=l->next) {
		n = l->n;
		if(n->op != OTYPE) {
			continue;
		}
		t = n->type;
		dtypesym(t);
		if(t->sym) {
			dtypesym(ptrto(t));
		}
	}

	// 打印当前 package 直接引用的 package 信息.
	// print("dumpexport() localpkg name: %s\n", localpkg->name);
	//
	// generate import strings for imported packages
	for(i=0; i<nelem(phash); i++) {
		for(p=phash[i]; p; p=p->link) {
			// print(
			// 	"dumpexport() import index: %d, package: %s, is direct?: %b\n",
			// 	i, p->name, p->direct
			// );
			if(p->direct) {
				dimportpath(p);
			}
		}
	}

	// do basic types if compiling package runtime.
	// they have to be in at least one package,
	// and runtime is always loaded implicitly,
	// so this is as good as any.
	// another possible choice would be package main,
	// but using runtime means fewer copies in .6 files.
	if(compiling_runtime) {
		for(i=1; i<=TBOOL; i++) {
			dtypesym(ptrto(types[i]));
		}
		dtypesym(ptrto(types[TSTRING]));
		dtypesym(ptrto(types[TUNSAFEPTR]));

		// emit type structs for error and func(error) string.
		// The latter is the type of an auto-generated wrapper.
		dtypesym(ptrto(errortype));
		dtypesym(functype(nil,
			list1(nod(ODCLFIELD, N, typenod(errortype))),
			list1(nod(ODCLFIELD, N, typenod(types[TSTRING])))));

		// add paths for runtime and main, which 6l imports implicitly.
		dimportpath(runtimepkg);
		if(flag_race) {
			dimportpath(racepkg);
		}
		dimportpath(mkpkg(strlit("main")));
	}
}

// caller:
// 	1. dumptypestructs() 唯一的主调函数.
// 	2. dcommontype() 由 dtypesym -> dcommontype -> dtypesym 形式的嵌套调用,
// 	所以不是起始的主调函数(dcommontype()的主调函数只有一个 dtypesym()).
// 	3. dextratype() 与 dcommontype() 一样, 也是由 dtypesym 先调用.
// 	4. dtypesym() 自身的递归调用
Sym* dtypesym(Type *t)
{
	int ot, xt, n, isddd, dupok;
	Sym *s, *s1, *s2, *s3, *s4, *slink;
	Sig *a, *m;
	Type *t1, *tbase, *t2;

	// Replace byte, rune aliases with real type.
	// They've been separate internally to make error messages
	// better, but we have to merge them in the reflect tables.
	if(t == bytetype || t == runetype) {
		t = types[t->etype];
	}

	if(isideal(t)) {
		fatal("dtypesym %T", t);
	}

	s = typesym(t);
	if(s->flags & SymSiggen) {
		return s;
	}
	s->flags |= SymSiggen;

	// special case (look for runtime below):
	// when compiling package runtime,
	// emit the type structures for int, float, etc.
	tbase = t;
	if(isptr[t->etype] && t->sym == S && t->type->sym != S) {
		tbase = t->type;
	}
	dupok = tbase->sym == S;

	if(compiling_runtime &&
			(tbase == types[tbase->etype] ||
			tbase == bytetype ||
			tbase == runetype ||
			tbase == errortype)) { // int, float, etc
		goto ok;
	}

	// named types from other files are defined only by those files
	if(tbase->sym && !tbase->local) {
		return s;
	}
	if(isforw[tbase->etype]) {
		return s;
	}

ok:
	ot = 0;
	xt = 0;
	switch(t->etype) {
		default: 
		{
			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			break;
		}

		case TARRAY: 
		{
			if(t->bound >= 0) {
				// ../../pkg/runtime/type.go:/ArrayType
				s1 = dtypesym(t->type);
				t2 = typ(TARRAY);
				t2->type = t->type;
				t2->bound = -1;  // slice
				s2 = dtypesym(t2);
				ot = dcommontype(s, ot, t);
				xt = ot - 2*widthptr;
				ot = dsymptr(s, ot, s1, 0);
				ot = dsymptr(s, ot, s2, 0);
				ot = duintptr(s, ot, t->bound);
			} else {
				// ../../pkg/runtime/type.go:/SliceType
				s1 = dtypesym(t->type);
				ot = dcommontype(s, ot, t);
				xt = ot - 2*widthptr;
				ot = dsymptr(s, ot, s1, 0);
			}
			break;
		}

		case TCHAN: 
		{
			// ../../pkg/runtime/type.go:/ChanType
			s1 = dtypesym(t->type);
			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			ot = dsymptr(s, ot, s1, 0);
			ot = duintptr(s, ot, t->chan);
			break;
		}

		case TFUNC: 
		{
			for(t1=getthisx(t)->type; t1; t1=t1->down) {
				dtypesym(t1->type);
			}
			isddd = 0;
			for(t1=getinargx(t)->type; t1; t1=t1->down) {
				isddd = t1->isddd;
				dtypesym(t1->type);
			}
			for(t1=getoutargx(t)->type; t1; t1=t1->down) {
				dtypesym(t1->type);
			}

			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			ot = duint8(s, ot, isddd);

			// two slice headers: in and out.
			ot = rnd(ot, widthptr);
			ot = dsymptr(s, ot, s, ot+2*(widthptr+2*widthint));
			n = t->thistuple + t->intuple;
			ot = duintxx(s, ot, n, widthint);
			ot = duintxx(s, ot, n, widthint);
			ot = dsymptr(s, ot, s, ot+1*(widthptr+2*widthint)+n*widthptr);
			ot = duintxx(s, ot, t->outtuple, widthint);
			ot = duintxx(s, ot, t->outtuple, widthint);

			// slice data
			for(t1=getthisx(t)->type; t1; t1=t1->down, n++) {
				ot = dsymptr(s, ot, dtypesym(t1->type), 0);
			}
			for(t1=getinargx(t)->type; t1; t1=t1->down, n++) {
				ot = dsymptr(s, ot, dtypesym(t1->type), 0);
			}
			for(t1=getoutargx(t)->type; t1; t1=t1->down, n++) {
				ot = dsymptr(s, ot, dtypesym(t1->type), 0);
			}
			break;
		}

		// Inter 是 interface...
		case TINTER: 
		{
			m = imethods(t);
			n = 0;
			for(a=m; a; a=a->link) {
				dtypesym(a->type);
				n++;
			}

			// ../../pkg/runtime/type.go:/InterfaceType
			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			ot = dsymptr(s, ot, s, ot+widthptr+2*widthint);
			ot = duintxx(s, ot, n, widthint);
			ot = duintxx(s, ot, n, widthint);
			for(a=m; a; a=a->link) {
				// ../../pkg/runtime/type.go:/imethod
				ot = dgostringptr(s, ot, a->name);
				ot = dgopkgpath(s, ot, a->pkg);
				ot = dsymptr(s, ot, dtypesym(a->type), 0);
			}
			break;
		}

		case TMAP: 
		{
			// ../../pkg/runtime/type.go:/MapType
			s1 = dtypesym(t->down);
			s2 = dtypesym(t->type);
			s3 = dtypesym(mapbucket(t));
			s4 = dtypesym(hmap(t));
			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			ot = dsymptr(s, ot, s1, 0);
			ot = dsymptr(s, ot, s2, 0);
			ot = dsymptr(s, ot, s3, 0);
			ot = dsymptr(s, ot, s4, 0);
			break;
		}

		case TPTR32:
		case TPTR64:
		{
			if(t->type->etype == TANY) {
				// ../../pkg/runtime/type.go:/UnsafePointerType
				ot = dcommontype(s, ot, t);
				break;
			}
			// ../../pkg/runtime/type.go:/PtrType
			s1 = dtypesym(t->type);
			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			ot = dsymptr(s, ot, s1, 0);
			break;
		}

		case TSTRUCT: 
		{
			// ../../pkg/runtime/type.go:/StructType
			// for security, only the exported fields.
			n = 0;
			for(t1=t->type; t1!=T; t1=t1->down) {
				dtypesym(t1->type);
				n++;
			}
			ot = dcommontype(s, ot, t);
			xt = ot - 2*widthptr;
			ot = dsymptr(s, ot, s, ot+widthptr+2*widthint);
			ot = duintxx(s, ot, n, widthint);
			ot = duintxx(s, ot, n, widthint);
			for(t1=t->type; t1!=T; t1=t1->down) {
				// ../../pkg/runtime/type.go:/structField
				if(t1->sym && !t1->embedded) {
					ot = dgostringptr(s, ot, t1->sym->name);
					if(exportname(t1->sym->name))
						ot = dgostringptr(s, ot, nil);
					else
						ot = dgopkgpath(s, ot, t1->sym->pkg);
				} else {
					ot = dgostringptr(s, ot, nil);
					if(t1->type->sym != S && t1->type->sym->pkg == builtinpkg)
						ot = dgopkgpath(s, ot, localpkg);
					else
						ot = dgostringptr(s, ot, nil);
				}
				ot = dsymptr(s, ot, dtypesym(t1->type), 0);
				ot = dgostrlitptr(s, ot, t1->note);
				ot = duintptr(s, ot, t1->width);	// field offset
			}
			break;
		}
	} // switch 结束...
	ot = dextratype(s, ot, t, xt);
	ggloblsym(s, ot, dupok, 1);

	// generate typelink.foo pointing at s = type.foo.
	// The linker will leave a table of all the typelinks for
	// types in the binary, so reflect can find them.
	// We only need the link for unnamed composites that
	// we want be able to find.
	if(t->sym == S) {
		switch(t->etype) {
		case TARRAY:
		case TCHAN:
		case TMAP:
			slink = typelinksym(t);
			dsymptr(slink, 0, s, 0);
			ggloblsym(slink, widthptr, dupok, 1);
		}
	}

	return s;
}

// dgcsym1 ...
//
// caller:
// 	1. dgcsym()
//
static int dgcsym1(Sym *s, int ot, Type *t, vlong *off, int stack_size)
{
	Type *t1;
	vlong o, off2, fieldoffset, i;

	if(t->align > 0 && (*off % t->align) != 0)
		fatal("dgcsym1: invalid initial alignment, %T", t);

	if(t->width == BADWIDTH)
		dowidth(t);
	
	switch(t->etype) {
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
	case TFLOAT32:
	case TFLOAT64:
	case TCOMPLEX64:
	case TCOMPLEX128:
		*off += t->width;
		break;

	case TPTR32:
	case TPTR64:
		// NOTE: Any changes here need to be made to reflect.PtrTo as well.
		if(*off % widthptr != 0)
			fatal("dgcsym1: invalid alignment, %T", t);
		if(!haspointers(t->type) || t->type->etype == TUINT8) {
			ot = duintptr(s, ot, GC_APTR);
			ot = duintptr(s, ot, *off);
		} else {
			ot = duintptr(s, ot, GC_PTR);
			ot = duintptr(s, ot, *off);
			ot = dsymptr(s, ot, dgcsym(t->type), 0);
		}
		*off += t->width;
		break;

	case TUNSAFEPTR:
	case TFUNC:
		if(*off % widthptr != 0)
			fatal("dgcsym1: invalid alignment, %T", t);
		ot = duintptr(s, ot, GC_APTR);
		ot = duintptr(s, ot, *off);
		*off += t->width;
		break;

	// struct Hchan*
	case TCHAN:
		// NOTE: Any changes here need to be made to reflect.ChanOf as well.
		if(*off % widthptr != 0)
			fatal("dgcsym1: invalid alignment, %T", t);
		ot = duintptr(s, ot, GC_CHAN_PTR);
		ot = duintptr(s, ot, *off);
		ot = dsymptr(s, ot, dtypesym(t), 0);
		*off += t->width;
		break;

	// struct Hmap*
	case TMAP:
		// NOTE: Any changes here need to be made to reflect.MapOf as well.
		if(*off % widthptr != 0)
			fatal("dgcsym1: invalid alignment, %T", t);
		ot = duintptr(s, ot, GC_PTR);
		ot = duintptr(s, ot, *off);
		ot = dsymptr(s, ot, dgcsym(hmap(t)), 0);
		*off += t->width;
		break;

	// struct { byte *str; int32 len; }
	case TSTRING:
		if(*off % widthptr != 0)
			fatal("dgcsym1: invalid alignment, %T", t);
		ot = duintptr(s, ot, GC_STRING);
		ot = duintptr(s, ot, *off);
		*off += t->width;
		break;

	// struct { Itab* tab;  void* data; }
	// struct { Type* type; void* data; }	// When isnilinter(t)==true
	case TINTER:
		if(*off % widthptr != 0)
			fatal("dgcsym1: invalid alignment, %T", t);
		if(isnilinter(t)) {
			ot = duintptr(s, ot, GC_EFACE);
			ot = duintptr(s, ot, *off);
		} else {
			ot = duintptr(s, ot, GC_IFACE);
			ot = duintptr(s, ot, *off);
		}
		*off += t->width;
		break;

	case TARRAY:
		if(t->bound < -1)
			fatal("dgcsym1: invalid bound, %T", t);
		if(t->type->width == BADWIDTH)
			dowidth(t->type);
		if(isslice(t)) {
			// NOTE: Any changes here need to be made to reflect.SliceOf as well.
			// struct { byte* array; uint32 len; uint32 cap; }
			if(*off % widthptr != 0)
				fatal("dgcsym1: invalid alignment, %T", t);
			if(t->type->width != 0) {
				ot = duintptr(s, ot, GC_SLICE);
				ot = duintptr(s, ot, *off);
				ot = dsymptr(s, ot, dgcsym(t->type), 0);
			} else {
				ot = duintptr(s, ot, GC_APTR);
				ot = duintptr(s, ot, *off);
			}
			*off += t->width;
		} else {
			// NOTE: Any changes here need to be made to reflect.ArrayOf as well,
			// at least once ArrayOf's gc info is implemented and ArrayOf is exported.
			// struct { byte* array; uint32 len; uint32 cap; }
			if(t->bound < 1 || !haspointers(t->type)) {
				*off += t->width;
			} else if(gcinline(t)) {
				for(i=0; i<t->bound; i++)
					ot = dgcsym1(s, ot, t->type, off, stack_size);  // recursive call of dgcsym1
			} else {
				if(stack_size < GC_STACK_CAPACITY) {
					ot = duintptr(s, ot, GC_ARRAY_START);  // a stack push during GC
					ot = duintptr(s, ot, *off);
					ot = duintptr(s, ot, t->bound);
					ot = duintptr(s, ot, t->type->width);
					off2 = 0;
					ot = dgcsym1(s, ot, t->type, &off2, stack_size+1);  // recursive call of dgcsym1
					ot = duintptr(s, ot, GC_ARRAY_NEXT);  // a stack pop during GC
				} else {
					ot = duintptr(s, ot, GC_REGION);
					ot = duintptr(s, ot, *off);
					ot = duintptr(s, ot, t->width);
					ot = dsymptr(s, ot, dgcsym(t), 0);
				}
				*off += t->width;
			}
		}
		break;

	case TSTRUCT:
		o = 0;
		for(t1=t->type; t1!=T; t1=t1->down) {
			fieldoffset = t1->width;
			*off += fieldoffset - o;
			ot = dgcsym1(s, ot, t1->type, off, stack_size);  // recursive call of dgcsym1
			o = fieldoffset + t1->type->width;
		}
		*off += t->width - o;
		break;

	default:
		fatal("dgcsym1: unexpected type %T", t);
	}

	return ot;
}

// caller:
// 	1. dcommontype()
// 	2. dgcsym1() 嵌套调用
Sym* dgcsym(Type *t)
{
	int ot;
	vlong off;
	Sym *s;

	s = typesymprefix(".gc", t);
	if(s->flags & SymGcgen)
		return s;
	s->flags |= SymGcgen;

	if(t->width == BADWIDTH)
		dowidth(t);

	ot = 0;
	off = 0;
	ot = duintptr(s, ot, t->width);
	ot = dgcsym1(s, ot, t, &off, 0);
	ot = duintptr(s, ot, GC_END);
	ggloblsym(s, ot, 1, 1);

	if(t->align > 0)
		off = rnd(off, t->align);
	if(off != t->width)
		fatal("dgcsym: off=%lld, size=%lld, type %T", off, t->width, t);

	return s;
}
