#include	<u.h>

// Our own isdigit, isspace, isalpha, isalnum that take care 
// of EOF and other out of range arguments.
int yy_isdigit(int c)
{
	return c >= 0 && c <= 0xFF && isdigit(c);
}

int yy_isspace(int c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// yy_isalpha 判断目标字符 c 是否为字母(包括大写或小写)
//
// 主调函数一般通过文件路径的前2个字符来确认当前是否为 windows,
// 因为 windows 的路径一般是, d://code 这样的, 前2个字符是字母+冒号.
//
// caller:
// 	1. main()
// 	2. islocalname()
int yy_isalpha(int c)
{
	// isalpha 是 c 标准库函数, 确切来说是个宏定义, 并不是函数.
	// 不过下面这一行感觉前半部分很多余啊, 为啥还要先确认 c 是[0,255] ASCII 码呢
	return c >= 0 && c <= 0xFF && isalpha(c);
}

int yy_isalnum(int c)
{
	return c >= 0 && c <= 0xFF && isalnum(c);
}

// Disallow use of isdigit etc.
#undef isdigit
#undef isspace
#undef isalpha
#undef isalnum
#define isdigit use_yy_isdigit_instead_of_isdigit
#define isspace use_yy_isspace_instead_of_isspace
#define isalpha use_yy_isalpha_instead_of_isalpha
#define isalnum use_yy_isalnum_instead_of_isalnum
