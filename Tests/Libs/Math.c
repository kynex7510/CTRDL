#include <stdio.h>

extern void doMath(int a, int b) {
    puts("Math logs, for mathematicians");
    printf("A: %d, B: %d\n", a, b);
    printf("A + B = %d\n", a + b);
    printf("A - B = %d\n", a - b);
    printf("A * B = %d\n", a * b);
    
    if (b) {
        printf("A / B = %d\n", a / b);
    } else {
        printf("A / B = IMPOSSIBLE");
    }
}