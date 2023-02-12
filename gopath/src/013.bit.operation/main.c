// main.c
#include <stdio.h>

typedef	signed long long int	int64;
typedef	unsigned long long int	uint64;

#define bitShift 16

// 0000 0000 0000 0001(1)
#define bitAllocated        ((int64)1<<(bitShift*0))
// 0000 0000 0001 0000(65536) (与 bitBlockBoundary 取值相同)
/* when bitAllocated is set */
#define bitNoScan       ((int64)1<<(bitShift*1))
// 0000 0001 0000 0000(4294967296)
/* when bitAllocated is set */
#define bitMarked       ((int64)1<<(bitShift*2))
// 0001 0000 0000 0000(281474976710656)
/* when bitAllocated is set - has finalizer or being profiled */
#define bitSpecial      ((int64)1<<(bitShift*3))
// 0000 0000 0001 0000(65536) (与 bitNoScan 取值相同)
/* when bitAllocated is NOT set */
#define bitBlockBoundary    ((int64)1<<(bitShift*1))
// bitMask 是3种已知的"bit位"组合, 用来先对目标块的"bit位"进行清零的. 
// 最典型的例子就是: *bitp & ~(bitMask<<shift)
//
// 0001 0001 0000 0001
#define bitMask (bitBlockBoundary | bitAllocated | bitMarked | bitSpecial)

void strrev(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

//  @param num: 待转换的整型数值, 可为负值;
//  @param str: 输出的字符串指针, 主调函数应自行定义字符串数组并分配内存空间;
//  @param len: str 数组空间容量, 限制了 str 字符串能表示的数值的上限;
//  @param base: 进制, 如2, 8, 10, 16进制等;
//
//  @return: 返回值为0表示正常执行, -1表示转换出错;
//
// 来自: https://android.googlesource.com/kernel/lk/+/qcom-dima-8x74-fixes/lib/libc/itoa.c
int itoa(int num, unsigned char* str, int len, int base)
{
    int sign; // 正负号
    int i = 0;
    int digit;
    if (len == 0) {
        return -1;
    }
    if ((sign = num) < 0) {
        num = -num;
    }
    do {
        digit = num % base;
        if (digit < 0xA) {
            // 如果目标进制小于10, 则只在 '0'(ACSII)的基础上增加相应值
            str[i++] = '0' + digit;
        }
        else {
            // 如果目标进制大于10, 则要在 'A'符的基础上增加.
            str[i++] = 'A' + digit - 0xA;
        }
        num /= base;
    } while (num && (i < (len - 1)));
    if (i == (len - 1) && num) {
        return -1;
    }

    if (sign < 0) {
        str[i++] = '-';
    }
    str[i] = '\0';
    strrev(str);
    return 0;
}

int itoal(long long int num, unsigned char* str, int len, int base)
{
    int sign; // 正负号
    int i = 0;
    int digit;
    if (len == 0) {
        return -1;
    }
    if ((sign = num) < 0) {
        num = -num;
    }
    do {
        digit = num % base;
        if (digit < 0xA) {
            // 如果目标进制小于10, 则只在 '0'(ACSII)的基础上增加相应值
            str[i++] = '0' + digit;
        }
        else {
            // 如果目标进制大于10, 则要在 'A'符的基础上增加.
            str[i++] = 'A' + digit - 0xA;
        }
        num /= base;
    } while (num && (i < (len - 1)));
    if (i == (len - 1) && num) {
        return -1;
    }

    if (sign < 0) {
        str[i++] = '-';
    }
    str[i] = '\0';
    strrev(str);
    return 0;
}

// 全局二进制字符串
char binary_string[64] = {0};

void setAlloc() {

}

void main(){
    itoa(bitAllocated, binary_string, 32, 16);
    printf("%d, %s\n", bitAllocated, binary_string);

    itoa(bitNoScan, binary_string, 32, 16);
    printf("%d, %s\n", bitNoScan, binary_string);

    itoal(bitMarked, binary_string, 64, 16);
    printf("%ld, %s\n", bitMarked, binary_string);

    itoal(bitSpecial, binary_string, 64, 16);
    printf("%ld, %s\n", bitSpecial, binary_string);

    setAlloc();
}

// gcc -g -o main main.c
