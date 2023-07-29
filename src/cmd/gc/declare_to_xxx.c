#include	<u.h>
#include	<libc.h>

#include	"go.h"

/*
 * structs, functions, and methods.
 * they don't belong here, but where do they belong?
 */

// caller:
// 	1. structfield()
// 	2. interfacefield()
static void checkembeddedtype(Type *t)
{
	if (t == T)
		return;

	if(t->sym == S && isptr[t->etype]) {
		t = t->type;
		if(t->etype == TINTER)
			yyerror("embedded type cannot be a pointer to interface");
	}
	if(isptr[t->etype])
		yyerror("embedded type cannot be a pointer");
	else if(t->etype == TFORW && t->embedlineno == 0)
		t->embedlineno = lineno;
}

// caller:
// 	1. tostruct()
// 	2. tofunargs()
// 	3. addmethod()
Type* structfield(Node *n)
{
	Type *f;
	int lno;

	lno = lineno;
	lineno = n->lineno;

	if(n->op != ODCLFIELD)
		fatal("structfield: oops %N\n", n);

	f = typ(TFIELD);
	f->isddd = n->isddd;

	if(n->right != N) {
		typecheck(&n->right, Etype);
		n->type = n->right->type;
		if(n->left != N)
			n->left->type = n->type;
		if(n->embedded)
			checkembeddedtype(n->type);
	}
	n->right = N;
		
	f->type = n->type;
	if(f->type == T)
		f->broke = 1;

	switch(n->val.ctype) {
	case CTSTR:
		f->note = n->val.u.sval;
		break;
	default:
		yyerror("field annotation must be string");
		// fallthrough
	case CTxxx:
		f->note = nil;
		break;
	}

	if(n->left && n->left->op == ONAME) {
		f->nname = n->left;
		f->embedded = n->embedded;
		f->sym = f->nname->sym;
	}

	lineno = lno;
	return f;
}

static uint32 uniqgen;

// 	@param what: 可选值有: field, method, argument
static void checkdupfields(Type *t, char* what)
{
	int lno;

	lno = lineno;

	for( ; t; t=t->down) {
		if(t->sym && t->nname && !isblank(t->nname)) {
			if(t->sym->uniqgen == uniqgen) {
				lineno = t->nname->lineno;
				yyerror("duplicate %s %s", what, t->sym->name);
			} else
				t->sym->uniqgen = uniqgen;
		}
	}

	lineno = lno;
}

// caller:
// 	1. src/cmd/gc/typecheck1.c -> typecheck1()
// 	2. src/cmd/gc/go.y -> hidden_type_misc{}
//
// convert a parsed id/type list into a type for struct/interface/arglist
// 
Type* tostruct(NodeList *l)
{
	Type *t, *f, **tp;
	t = typ(TSTRUCT);

	for(tp = &t->type; l; l=l->next) {
		f = structfield(l->n);

		*tp = f;
		tp = &f->down;
	}

	for(f=t->type; f && !t->broke; f=f->down)
		if(f->broke)
			t->broke = 1;

	uniqgen++;
	checkdupfields(t->type, "field");

	if (!t->broke)
		checkwidth(t);

	return t;
}

// caller:
// 	1. functype() 只有这一处
static Type* tofunargs(NodeList *l)
{
	Type *t, *f, **tp;

	t = typ(TSTRUCT);
	t->funarg = 1;

	for(tp = &t->type; l; l=l->next) {
		f = structfield(l->n);
		f->funarg = 1;

		// esc.c needs to find f given a PPARAM to add the tag.
		if(l->n->left && l->n->left->class == PPARAM)
			l->n->left->paramfld = f;

		*tp = f;
		tp = &f->down;
	}

	for(f=t->type; f && !t->broke; f=f->down)
		if(f->broke)
			t->broke = 1;

	return t;
}

// 将目标函数封装成 interface{} 接口中的一个字段类型并返回
//
// 	@param n: 某个 interface{} 接口中定义的一个函数节点
//
// caller:
// 	1. tointerface() 只有这一处
static Type* interfacefield(Node *n)
{
	Type *f;
	int lno;

	lno = lineno;
	lineno = n->lineno;

	if(n->op != ODCLFIELD) {
		fatal("interfacefield: oops %N\n", n);
	}

	if (n->val.ctype != CTxxx) {
		yyerror("interface method cannot have annotation");
	}

	f = typ(TFIELD);
	f->isddd = n->isddd;
	
	if(n->right != N) {
		if(n->left != N) {
			// queue resolution of method type for later.
			// right now all we need is the name list.
			// avoids cycles for recursive interface types.
			n->type = typ(TINTERMETH);
			n->type->nname = n->right;
			n->left->type = n->type;
			queuemethod(n);

			if(n->left->op == ONAME) {
				f->nname = n->left;
				f->embedded = n->embedded;
				f->sym = f->nname->sym;
				if(importpkg && !exportname(f->sym->name)) {
					f->sym = pkglookup(f->sym->name, structpkg);
				}
			}

		} else {
			typecheck(&n->right, Etype);
			n->type = n->right->type;

			if(n->embedded) {
				checkembeddedtype(n->type);
			}

			if(n->type)
				switch(n->type->etype) {
				case TINTER:
					break;
				case TFORW:
					yyerror("interface type loop involving %T", n->type);
					f->broke = 1;
					break;
				default:
					yyerror("interface contains embedded non-interface %T", n->type);
					f->broke = 1;
					break;
				}
		}
	}

	n->right = N;
	
	f->type = n->type;
	if(f->type == T) {
		f->broke = 1;
	}
	
	lineno = lno;
	return f;
}

// 	@param l: interface{} 接口中所定义的函数列表
//
// caller:
// 	1. src/cmd/gc/typecheck1.c -> typecheck1() case OTINTER{}
// 	2. src/cmd/gc/go.y -> hidden_type_misc{}
Type* tointerface(NodeList *l)
{
	Type *t, *f, **tp, *t1;

	t = typ(TINTER);

	tp = &t->type;
	for(; l; l=l->next) {
		f = interfacefield(l->n);

		if (l->n->left == N && f->type->etype == TINTER) {
			// embedded interface, inline methods
			for(t1=f->type->type; t1; t1=t1->down) {
				f = typ(TFIELD);
				f->type = t1->type;
				f->broke = t1->broke;
				f->sym = t1->sym;
				if(f->sym) {
					f->nname = newname(f->sym);
				}
				*tp = f;
				tp = &f->down;
			}
		} else {
			*tp = f;
			tp = &f->down;
		}
	}

	for(f=t->type; f && !t->broke; f=f->down) {
		if(f->broke) {
			t->broke = 1;
		}
	}

	uniqgen++;
	checkdupfields(t->type, "method");
	t = sortinter(t);
	checkwidth(t);

	return t;
}

// 构建一个函数/方法类型并返回
//
// 	@param this: 方法声明中, 所属的 receiver
// 	@param in: 方法声明中的参数列表(即形参列表)
// 	@param out: 方法声明中的返回值列表
//
// caller:
// 	1. src/cmd/gc/go.y -> hidden_fndcl{}
//
// turn a parsed function declaration into a type
Type* functype(Node *this, NodeList *in, NodeList *out)
{
	Type *t;
	NodeList *rcvr;
	Sym *s;

	t = typ(TFUNC);

	rcvr = nil;
	if(this) {
		rcvr = list1(this);
	}
	t->type = tofunargs(rcvr);
	t->type->down = tofunargs(out);
	t->type->down->down = tofunargs(in);

	uniqgen++;
	checkdupfields(t->type->type, "argument");
	checkdupfields(t->type->down->type, "argument");
	checkdupfields(t->type->down->down->type, "argument");

	if (t->type->broke || t->type->down->broke || t->type->down->down->broke) {
		t->broke = 1;
	}

	if(this) {
		t->thistuple = 1;
	}
	t->outtuple = count(out);
	t->intuple = count(in);
	t->outnamed = 0;
	if(t->outtuple > 0 && out->n->left != N && out->n->left->orig != N) {
		s = out->n->left->orig->sym;
		if(s != S && s->name[0] != '~') {
			t->outnamed = 1;
		}
	}

	return t;
}
