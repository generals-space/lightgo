extern char *goos, *goarch, *goroot;
enum
{
	EOF		= -1,
};


void addexp(char *s);
void addidir(char* dir);
void doversion(void);
void usage(void);
void fault(int s);


// 	@note: 注意这块代码必须放的 getc()/ungetc() 的函数声明前面
#undef	getc
#undef	ungetc
#define	getc	ccgetc
#define	ungetc	ccungetc

int getc(void);
void ungetc(int c);
int isfrog(int c);
int getlinepragma(void);
int32 getr(void);
int escchar(int e, int *escflg, vlong *val);


int yy_isdigit(int c);
int yy_isspace(int c);
int yy_isalpha(int c);
int yy_isalnum(int c);


void lexinit(void);
void lexinit1(void);
void lexfini(void);


void yytinit(void);
