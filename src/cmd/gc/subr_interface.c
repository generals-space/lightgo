#include	<u.h>
#include	<libc.h>
#include	"go.h"

static int methcmp(const void *va, const void *vb)
{
	Type *a, *b;
	int i;

	a = *(Type**)va;
	b = *(Type**)vb;
	if(a->sym == S && b->sym == S)
		return 0;
	if(a->sym == S)
		return -1;
	if(b->sym == S)
		return 1;
	i = strcmp(a->sym->name, b->sym->name);
	if(i != 0)
		return i;
	if(!exportname(a->sym->name)) {
		i = strcmp(a->sym->pkg->path->s, b->sym->pkg->path->s);
		if(i != 0)
			return i;
	}
	return 0;
}

Type* sortinter(Type *t)
{
	Type *f;
	int i;
	Type **a;
	
	if(t->type == nil || t->type->down == nil)
		return t;

	i=0;
	for(f=t->type; f; f=f->down)
		i++;
	a = mal(i*sizeof f);
	i = 0;
	for(f=t->type; f; f=f->down)
		a[i++] = f;
	qsort(a, i, sizeof a[0], methcmp);
	while(i-- > 0) {
		a[i]->down = f;
		f = a[i];
	}
	t->type = f;
	return t;
}