#include	"walk.h"

Node* callnew(Type *t)
{
	Node *fn;

	dowidth(t);
	fn = syslook("new", 1);
	argtype(fn, t);
	return mkcall1(fn, ptrto(t), nil, typename(t));
}

// caller:
// 	1. candiscard() 只有这一处
static int candiscardlist(NodeList *l)
{
	for(; l; l=l->next)
		if(!candiscard(l->n))
			return 0;
	return 1;
}

// caller:
//  1. src/cmd/gc/init.c -> anyinit()
//  2. src/cmd/gc/sinit.c -> init1()
int candiscard(Node *n)
{
	if(n == N)
		return 1;
	
	switch(n->op) {
	default:
		return 0;

	case ONAME:
	case ONONAME:
	case OTYPE:
	case OPACK:
	case OLITERAL:
	case OADD:
	case OSUB:
	case OOR:
	case OXOR:
	case OADDSTR:
	case OADDR:
	case OANDAND:
	case OARRAYBYTESTR:
	case OARRAYRUNESTR:
	case OSTRARRAYBYTE:
	case OSTRARRAYRUNE:
	case OCAP:
	case OCMPIFACE:
	case OCMPSTR:
	case OCOMPLIT:
	case OMAPLIT:
	case OSTRUCTLIT:
	case OARRAYLIT:
	case OPTRLIT:
	case OCONV:
	case OCONVIFACE:
	case OCONVNOP:
	case ODOT:
	case OEQ:
	case ONE:
	case OLT:
	case OLE:
	case OGT:
	case OGE:
	case OKEY:
	case OLEN:
	case OMUL:
	case OLSH:
	case ORSH:
	case OAND:
	case OANDNOT:
	case ONEW:
	case ONOT:
	case OCOM:
	case OPLUS:
	case OMINUS:
	case OOROR:
	case OPAREN:
	case ORUNESTR:
	case OREAL:
	case OIMAG:
	case OCOMPLEX:
		// Discardable as long as the subpieces are.
		break;

	case ODIV:
	case OMOD:
		// Discardable as long as we know it's not division by zero.
		if(isconst(n->right, CTINT) && mpcmpfixc(n->right->val.u.xval, 0) != 0)
			break;
		if(isconst(n->right, CTFLT) && mpcmpfltc(n->right->val.u.fval, 0) != 0)
			break;
		return 0;

	case OMAKECHAN:
	case OMAKEMAP:
		// Discardable as long as we know it won't fail because of a bad size.
		if(isconst(n->left, CTINT) && mpcmpfixc(n->left->val.u.xval, 0) == 0)
			break;
		return 0;
	
	case OMAKESLICE:
		// Difficult to tell what sizes are okay.
		return 0;		
	}
	
	if(!candiscard(n->left) ||
	   !candiscard(n->right) ||
	   !candiscard(n->ntest) ||
	   !candiscard(n->nincr) ||
	   !candiscardlist(n->ninit) ||
	   !candiscardlist(n->nbody) ||
	   !candiscardlist(n->nelse) ||
	   !candiscardlist(n->list) ||
	   !candiscardlist(n->rlist)) {
		return 0;
	}
	
	return 1;
}
