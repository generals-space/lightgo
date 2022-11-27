#include <stdio.h>

// nelem 算是 len() 的变体, 用来求目标数组长度的(总数量而非实际赋值的数量).
// 用整个数组所占用的空间(sizeof(x)), 除以每个元素占用的空间(sizeof((x)[0])).
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

// 获取能够整除 n 的最接近 x 的值, 类似于 ceil(x/n) * n, 即向上取整
// 假设 x = 5, n = 4, 那 ROUND(x, n) = 8
#define	ROUND(x, n)	(((x)+(n)-1)&~((n)-1))

typedef	unsigned int    uint32;
// offsetof 用于计算目标成员 m 在 s 结构体中的偏移量, 也就是 m 成员前面的成员占用的空间数量.
// 结果值单位为字节.
// 	@param s: 为一个 struct 类对象(非实例对象, 直接传入类型名即可);
// 	@param m: 为其中的一个成员的名称(直接写名称, 不需要通过 a_struct_obj.xxx 或 a_struct_obj->xxx 指定)
//
#define	offsetof(s,m)	(uint32)(&(((s*)0)->m))

void len() {
    int len;
    int intArray[5] = {5};
    intArray[0] = 1;
    printf("0: %d, 4: %d\n", intArray[4]); // 此处元素 4 未赋值.
    len = nelem(intArray);
    printf("len: %d\n", len);
    return;
}

// 参考文章
// [C语言& ~运算和&运算](https://blog.csdn.net/qq_26747797/article/details/82806226)
void mod() {
    int x = 1000;
    int n = 64;

    // 取余运算
    int mod = x & (n - 1);
    printf("mod: %d\n", mod);       // 64

    // x 中最后一个能够整除 n 的整数
    int final = x &~ (n - 1);
    printf("final: %d\n", final);   // 960
}

// c语言原生带有 round 函数(四舍五入), 所以这里命名为 round1
void round1() {
    int x = 1000;
    int n = 64;
    int round = ROUND(x, n);
    printf("round: %d\n", round);
    // ROUND 函数其实等价于下面的表达式
    printf("round: %d\n", (x+n-1) &~ (n-1));
}

struct student {
    char name[20]; // 姓名
    int   age; // 年龄
    float score1; // 分数1
    double score2; // 分数2
    int something; // 随便, 主要为了看看 score 成员占用的空间;
};

void offset() {
    int offset;
    offset = offsetof(struct student, name);
    printf("name offset in student struct: %d\n", offset);  // 0

    offset = offsetof(struct student, age);
    printf("age offset in student struct: %d\n", offset);   // 20

    offset = offsetof(struct student, score1);
    printf("score1 offset in student struct: %d\n", offset); // 24 int类型占用4个字节(32位)

    offset = offsetof(struct student, score2);
    printf("score2 offset in student struct: %d\n", offset); // 32 float也是占用4个字节

    offset = offsetof(struct student, something);
    printf("something offset in student struct: %d\n", offset); // 40 double则占用8个字节
}

void main() {
    len();
    mod();
    round1();
    offset();
}
