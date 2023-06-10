#include	<u.h>
#include	<libc.h>
#include	"go.h"

// NodeList 相关的操作函数, 初始化, 合并, 排序, 计数等.

// concat 将目标链表 b, 追加到链表 a 末尾.
NodeList* concat(NodeList *a, NodeList *b)
{
	if(a == nil) {
		return b;
	}
	if(b == nil) {
		return a;
	}

	a->end->next = b;
	a->end = b->end;
	b->end = nil;
	return a;
}

NodeList* list1(Node *n)
{
	NodeList *l;

	if(n == nil)
		return nil;
	if(n->op == OBLOCK && n->ninit == nil) {
		// Flatten list and steal storage.
		// Poison pointer to catch errant uses.
		l = n->list;
		n->list = (NodeList*)1;
		return l;
	}
	l = mal(sizeof *l);
	l->n = n;
	l->end = l;
	return l;
}

// list 将目标 node 节点 n 加入到指定的 nodelist 链表 l 末尾.
NodeList* list(NodeList *l, Node *n)
{
	return concat(l, list1(n));
}

void listsort(NodeList** l, int(*f)(Node*, Node*))
{
	NodeList *l1, *l2, *le;

	if(*l == nil || (*l)->next == nil)
		return;

	l1 = *l;
	l2 = *l;
	for(;;) {
		l2 = l2->next;
		if(l2 == nil)
			break;
		l2 = l2->next;
		if(l2 == nil)
			break;
		l1 = l1->next;
	}

	l2 = l1->next;
	l1->next = nil;
	l2->end = (*l)->end;
	(*l)->end = l1;

	l1 = *l;
	listsort(&l1, f);
	listsort(&l2, f);

	if((*f)(l1->n, l2->n) < 0) {
		*l = l1;
	} else {
		*l = l2;
		l2 = l1;
		l1 = *l;
	}

	// now l1 == *l; and l1 < l2

	while ((l1 != nil) && (l2 != nil)) {
		while ((l1->next != nil) && (*f)(l1->next->n, l2->n) < 0)
			l1 = l1->next;
		
		// l1 is last one from l1 that is < l2
		le = l1->next;		// le is the rest of l1, first one that is >= l2
		if(le != nil)
			le->end = (*l)->end;

		(*l)->end = l1;		// cut *l at l1
		*l = concat(*l, l2);	// glue l2 to *l's tail

		l1 = l2;		// l1 is the first element of *l that is < the new l2
		l2 = le;		// ... because l2 now is the old tail of l1
	}

	*l = concat(*l, l2);		// any remainder 
}

NodeList* listtreecopy(NodeList *l)
{
	NodeList *out;

	out = nil;
	for(; l; l=l->next)
		out = list(out, treecopy(l->n));
	return out;
}

Node* liststmt(NodeList *l)
{
	Node *n;

	n = nod(OBLOCK, N, N);
	n->list = l;
	if(l)
		n->lineno = l->n->lineno;
	return n;
}

/*
 * return nelem of list
 */
int count(NodeList *l)
{
	vlong n;

	n = 0;
	for(; l; l=l->next)
		n++;
	if((int)n != n) { // Overflow.
		yyerror("too many elements in list");
	}
	return n;
}

Node* treecopy(Node *n)
{
	Node *m;

	if(n == N)
		return N;

	switch(n->op) {
	default:
		m = nod(OXXX, N, N);
		*m = *n;
		m->orig = m;
		m->left = treecopy(n->left);
		m->right = treecopy(n->right);
		m->list = listtreecopy(n->list);
		if(m->defn)
			abort();
		break;

	case ONONAME:
		if(n->sym == lookup("iota")) {
			// Not sure yet whether this is the real iota,
			// but make a copy of the Node* just in case,
			// so that all the copies of this const definition
			// don't have the same iota value.
			m = nod(OXXX, N, N);
			*m = *n;
			m->iota = iota;
			break;
		}
		// fall through
	case ONAME:
	case OLITERAL:
	case OTYPE:
		m = n;
		break;
	}
	return m;
}
