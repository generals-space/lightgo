#include	<u.h>
#include	<libc.h>
#include	"go.h"

// nod 初始化一个空的 Node 对象
Node* nod(int op, Node *nleft, Node *nright)
{
	Node *n;

	n = mal(sizeof(*n));
	n->op = op;
	n->left = nleft;
	n->right = nright;
	n->lineno = parserline();
	n->xoffset = BADWIDTH;
	n->orig = n;
	n->curfn = curfn;
	return n;
}

// 将一个常规的整型数值, 包装成 Node 对象.
//
// 一般用于为某些语句添加默认值, 如 make(map[xxx]xxx), 会生成值为 0 的 Node.
//
// 下面还有几个相似的方法, 用于转换浮点型, nil等.
//
// caller:
// 	1. src/cmd/gc/typecheck.c -> typecheck1()
Node* nodintconst(int64 v)
{
	Node *c;

	c = nod(OLITERAL, N, N);
	c->addable = 1;
	c->val.u.xval = mal(sizeof(*c->val.u.xval));
	mpmovecfix(c->val.u.xval, v);
	c->val.ctype = CTINT;
	c->type = types[TIDEAL];
	ullmancalc(c);
	return c;
}

Node* nodfltconst(Mpflt* v)
{
	Node *c;

	c = nod(OLITERAL, N, N);
	c->addable = 1;
	c->val.u.fval = mal(sizeof(*c->val.u.fval));
	mpmovefltflt(c->val.u.fval, v);
	c->val.ctype = CTFLT;
	c->type = types[TIDEAL];
	ullmancalc(c);
	return c;
}

void nodconst(Node *n, Type *t, int64 v)
{
	memset(n, 0, sizeof(*n));
	n->op = OLITERAL;
	n->addable = 1;
	ullmancalc(n);
	n->val.u.xval = mal(sizeof(*n->val.u.xval));
	mpmovecfix(n->val.u.xval, v);
	n->val.ctype = CTINT;
	n->type = t;

	if(isfloat[t->etype]) {
		fatal("nodconst: bad type %T", t);
	}
}

Node* nodnil(void)
{
	Node *c;

	c = nodintconst(0);
	c->val.ctype = CTNIL;
	c->type = types[TNIL];
	return c;
}

Node* nodbool(int b)
{
	Node *c;

	c = nodintconst(0);
	c->val.ctype = CTBOOL;
	c->val.u.bval = b;
	c->type = idealbool;
	return c;
}

void saveorignode(Node *n)
{
	Node *norig;

	if(n->orig != N) {
		return;
	}
	norig = nod(n->op, N, N);
	*norig = *n;
	n->orig = norig;
}

Sym* ngotype(Node *n)
{
	if(n->type != T) {
		return typenamesym(n->type);
	}
	return S;
}

////////////////////////////////////////////////////////////////////////////////

/*
 * calculate sethi/ullman number
 * roughly how many registers needed to compile a node.
 * used to compile the hardest side first to minimize registers.
 */
void ullmancalc(Node *n)
{
	int ul, ur;

	if(n == N)
		return;

	if(n->ninit != nil) {
		ul = UINF;
		goto out;
	}

	switch(n->op) {
	case OREGISTER:
	case OLITERAL:
	case ONAME:
		ul = 1;
		if(n->class == PPARAMREF || (n->class & PHEAP))
			ul++;
		goto out;
	case OCALL:
	case OCALLFUNC:
	case OCALLMETH:
	case OCALLINTER:
		ul = UINF;
		goto out;
	case OANDAND:
	case OOROR:
		// hard with race detector
		if(flag_race) {
			ul = UINF;
			goto out;
		}
	}
	ul = 1;
	if(n->left != N)
		ul = n->left->ullman;
	ur = 1;
	if(n->right != N)
		ur = n->right->ullman;
	if(ul == ur)
		ul += 1;
	if(ur > ul)
		ul = ur;

out:
	if(ul > 200)
		ul = 200; // clamp to uchar with room to grow
	n->ullman = ul;
}

////////////////////////////////////////////////////////////////////////////////

void addinit(Node **np, NodeList *init)
{
	Node *n;
	
	if(init == nil)
		return;

	n = *np;
	switch(n->op) {
	case ONAME:
	case OLITERAL:
		// There may be multiple refs to this node;
		// introduce OCONVNOP to hold init list.
		n = nod(OCONVNOP, n, N);
		n->type = n->left->type;
		n->typecheck = 1;
		*np = n;
		break;
	}
	n->ninit = concat(init, n->ninit);
	n->ullman = UINF;
}

void checknil(Node *x, NodeList **init)
{
	Node *n;
	
	if(isinter(x->type)) {
		x = nod(OITAB, x, N);
		typecheck(&x, Erv);
	}
	n = nod(OCHECKNIL, x, N);
	n->typecheck = 1;
	*init = list(*init, n);
}
