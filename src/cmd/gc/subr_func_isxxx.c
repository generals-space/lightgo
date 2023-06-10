#include	<u.h>
#include	<libc.h>
#include	"go.h"

int isnil(Node *n)
{
	if(n == N)
		return 0;
	if(n->op != OLITERAL)
		return 0;
	if(n->val.ctype != CTNIL)
		return 0;
	return 1;
}

int isptrto(Type *t, int et)
{
	if(t == T)
		return 0;
	if(!isptr[t->etype])
		return 0;
	t = t->type;
	if(t == T)
		return 0;
	if(t->etype != et)
		return 0;
	return 1;
}

int istype(Type *t, int et)
{
	return t != T && t->etype == et;
}

// 判断是否为"数组"类型, 区别于 slice 切片, 数组的长度是固定的.
int isfixedarray(Type *t)
{
	return t != T && t->etype == TARRAY && t->bound >= 0;
}

// 判断是否为"切片"类型, 区别于 array 数组, 切片的长度是不固定的.
int isslice(Type *t)
{
	return t != T && t->etype == TARRAY && t->bound < 0;
}

int isblank(Node *n)
{
	if(n == N)
		return 0;
	return isblanksym(n->sym);
}

int isblanksym(Sym *s)
{
	char *p;

	if(s == S)
		return 0;
	p = s->name;
	if(p == nil)
		return 0;
	return p[0] == '_' && p[1] == '\0';
}

int isinter(Type *t)
{
	return t != T && t->etype == TINTER;
}

int isnilinter(Type *t)
{
	if(!isinter(t))
		return 0;
	if(t->type != T)
		return 0;
	return 1;
}

int isideal(Type *t)
{
	if(t == T)
		return 0;
	if(t == idealstring || t == idealbool)
		return 1;
	switch(t->etype) {
	case TNIL:
	case TIDEAL:
		return 1;
	}
	return 0;
}

/*
 * Is this a 64-bit type?
 */
int is64(Type *t)
{
	if(t == T)
		return 0;
	switch(simtype[t->etype]) {
	case TINT64:
	case TUINT64:
	case TPTR64:
		return 1;
	}
	return 0;
}
