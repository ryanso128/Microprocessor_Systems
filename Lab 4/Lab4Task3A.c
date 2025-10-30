

#include "init.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    Sys_Init();
    printf("\033[2J\033[H");
    printf("Lab 4 Task 3: Task set 1\r\n");

    //1
    uint32_t add_result = 0;

    asm volatile (
        "MOV r0, #10      \n"   // r0 = 10
        "MOV r1, #25      \n"   // r1 = 25
        "ADD %[out], r0, r1"
        : [out] "=r" (add_result)
        :
        : "r0","r1"
    );

    printf("Sum (10 + 25) = %lu\r\n", add_result);

    //2
    uint32_t mult_result;

    asm volatile (
        "MOV r0, #10      \n"
        "MOV r1, #25      \n"
        "MUL %[res], r0, r1"
        : [res] "=r" (mult_result)
        :
        : "r0","r1"
    );

    printf("Product (10 * 25) = %lu\r\n", mult_result);

    //3
    int32_t x = 9;
    int32_t expr_result;

    asm volatile (
        "LSL r2, %[X], #1       \n"   // mla = 2*x  (shift left by 1)
        "MOV r3, #3             \n"   // r3 = 3
        "SDIV r2, r2, r3        \n"   // r2 = (2*x)/3
        "ADD %[OUT], r2, #5"         // result = (2x/3) + 5
        : [OUT] "=r" (expr_result)
        : [X] "r" (x)
        : "r2", "r3"
    );

    printf("(2×%ld)/3 + 5 = %ld\r\n", (long)x, (long)expr_result);

  //4
    int32_t mac_result;

    asm volatile (
    	"MOV r0, #1      \n"   // r0 = 10
        "LSL r4, %[X], #1       \n"   // r4 = 2*x
        "MOV r5, #3             \n"   // r5 = 3
        "SDIV r4, r4, r5        \n"   // r4 = (2x)/3
        "MOV r6, #5             \n"   // r6 = 5
        "MLA %[OUT], r4, r0, r6"      // OUT = r4*1 + 5
        : [OUT] "=r" (mac_result)
        : [X] "r" (x)
        : "r4", "r5", "r6"
    );

    printf("MAC version (2×%ld)/3 + 5 = %ld\r\n", (long)x, (long)mac_result);


    while (1);
}
