#include	<u.h>
#include	<libc.h>
#include	"go.h"

Node* walkprint(Node *nn, NodeList **init, int defer);
int	bounded(Node*, int64);
void	walkrotate(Node**);
void	walkcompare(Node**, NodeList**);
Node* mapfndel(char *name, Type *t);
NodeList*	ascompatee(int, NodeList*, NodeList*, NodeList**);
Node*	addstr(Node*, NodeList**);
Node*	appendslice(Node*, NodeList**);
Node*	append(Node*, NodeList**);
Node*	copyany(Node*, NodeList**);

NodeList*	ascompatet(int, NodeList*, Type**, int, NodeList**);
NodeList*	ascompatte(int, Node*, int, Type**, NodeList*, int, NodeList**);

NodeList*	reorder1(NodeList*);
NodeList*	reorder3(NodeList*);

void	walkmul(Node**, NodeList**);
void	walkdiv(Node**, NodeList**);

Node*	convas(Node*, NodeList**);

Node*	mapfn(char*, Type*);
Node*	sliceany(Node*, NodeList**);
int vmatch2(Node *l, Node *r);
void	heapmoves(void);
