// main.c
#include <stdio.h>

void main(){
    int array[10] = {0, 1, 3, 6, 10};
    int* array_p0 = &array[0];
    int* array_p1 = &array[1];
    int* array_p2 = &array[2];

    printf("size of pointer: %d\n", sizeof(void*)); // 8

    printf("pointer: %p, value: %d\n", array_p0, *array_p0); // 0x7fff26e20ce0, 0
    printf("pointer: %p, value: %d\n", array_p1, *array_p1); // 0x7fff26e20ce4, 1
    printf("pointer: %p, value: %d\n", array_p2, *array_p2); // 0x7fff26e20ce8, 3

    // 指针+/-整型, 相当于前后移动, 变更索引.
    printf("pointer: %p, value: %d\n", array_p0+1, *(array_p0+1)); // 0x7fff26e20ce4, 1
    printf("pointer: %p, value: %d\n", array_p0+2, *(array_p0+2)); // 0x7fff26e20ce8, 3

    // 指针相减, 得到的是中间间隔的元素数量, 而非两者的地址差.
    printf("value: %d\n", array_p1 - array_p0); // 1
    printf("value: %d\n", array_p2 - array_p0); // 2

    // 没有指针+指针的用法.
}

// gcc -g -o main main.c
