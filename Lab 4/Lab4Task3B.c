
#include "init.h"
#include <stdio.h>




int main(void)
{
    Sys_Init();
    printf("\r\n Lab 4 Task 3: Task set 2\r\n");

    //------------------------------------------------
    // 1. Basic Inline Assembly: Add two constants (float)
    //------------------------------------------------

    float fsum = 0.0f;

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


    //------------------------------------------------
    // 2. Multiply same two floats
    //------------------------------------------------
    float fprod = 0.0f;
    float fa = 10.5f;
    float fb = 25.2f;

    asm volatile(
        "VMUL.F32 %[out], %[a], %[b]"
        : [out] "=t"(fprod)
        : [a] "t"(fa), [b] "t"(fb)
    );

    printf("Product (%.1f × %.1f) = %.2f\r\n", fa, fb, fprod);

    //------------------------------------------------
        // 3. Compute (2x)/3 + 5 for floating-point x
        //------------------------------------------------
        float x = 9.3f;
        float fexpr = 0.0f;
        float two = 2.0f, three = 3.0f, five = 5.0f;

        asm volatile(
            "VMUL.F32 s0, %[X], %[TWO]   \n"  // s0 = 2*x
            "VDIV.F32 s1, s0, %[THREE]   \n"  // s1 = (2x)/3
            "VADD.F32 %[OUT], s1, %[FIVE]"    // OUT = (2x)/3 + 5
            : [OUT] "=t"(fexpr)
            : [X] "t"(x), [TWO] "t"(two), [THREE] "t"(three), [FIVE] "t"(five)
            : "s0","s1"
        );
        printf("(2×%.1f)/3 + 5 = %.2f\r\n", x, fexpr);

        //------------------------------------------------
        // 4. Same equation using Multiply–Accumulate (VMLA)
        //     (Compute (2x)/3 + 5)
        //------------------------------------------------
        float fmac = 0.0f;
        float one = 1.0f;   // for clarity

        asm volatile(
            "VMUL.F32 s0, %[X], %[TWO]   \n"  // s0 = 2*x
            "VDIV.F32 s1, s0, %[THREE]   \n"  // s1 = (2x)/3
            "VMOV.F32 s2, %[FIVE]        \n"  // s2 = 5.0  (accumulator)
            "VMLA.F32 s2, s1, %[ONE]     \n"  // s2 = s2 + s1*1
            "VMOV.F32 %[OUT], s2"             // move result back to C
            : [OUT] "=t"(fmac)
            : [X] "t"(x), [TWO] "t"(two), [THREE] "t"(three),
              [FIVE] "t"(five), [ONE] "t"(one)
            : "s0","s1","s2"
        );

        printf("MAC version (2×%.1f)/3 + 5 = %.2f\r\n", x, fmac);


    while (1);
}
