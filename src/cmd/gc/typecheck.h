#include <u.h>
#include <libc.h>
#include "go.h"

Node*	resolve(Node*);

Type* lookdot1(Node *errnode, Sym *s, Type *t, Type *f, int dostrcmp);
int	lookdot(Node*, Type*, int);
int	looktypedot(Node*, Type*, int);

int	checkmake(Type*, char*, Node*);
int	checksliceindex(Node*, Type*);
int	checksliceconst(Node*, Node*);
void	checkdefergo(Node*);
void	implicitstar(Node**);
int	onearg(Node*, char*, ...);
int	twoarg(Node*);
void	typecheckaste(int, Node*, int, Type*, NodeList*, char*);
void	typecheckcomplit(Node**);
int islvalue(Node *n);
void	checklvalue(Node*, char*);
void	checkassign(Node*);
void	typecheckas(Node*);
void	typecheckas2(Node*);
void	typecheckfunc(Node*);
void	stringtoarraylit(Node**);
void typecheckdeftype(Node *n);

void typecheck1(Node **, int);
char* typekind(Type *t);
void indexlit(Node **np);
int callrecv(Node *n);
