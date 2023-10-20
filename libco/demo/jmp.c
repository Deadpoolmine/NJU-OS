#include "setjmp.h"
#include <stdio.h>

int main()
{
    int n = 0;
    jmp_buf buf;
    setjmp(buf);
    printf("Hello %d\n", n);
    longjmp(buf, n++);
}