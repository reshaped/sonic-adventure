#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* ov = v1 x v2. v1 and v2 are read whole before ov is written, so ov may be
 * either. returns the length of ov */
/* @asm operand: fipr-registers, FIPR FV0,FV8 at 0x8c628a7a */
#pragma inline_asm(njOuterProduct)
Float njOuterProduct(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *ov)
{
        MOV         #4,R0
        FMOV.S      @R4,FR10
        FMOV.S      @R5,FR6
        FMOV.S      @(R0,R4),FR4
        FMOV.S      @(R0,R5),FR5
        MOV         #8,R0
        FMOV.S      FR4,FR8
        FMOV.S      @(R0,R4),FR9
        FMOV.S      @(R0,R5),FR11
        FMUL        FR11,FR8
        FMOV.S      FR9,FR0
        FNEG        FR0
        FMAC        FR0,FR5,FR8
        MOV         #4,R0
        FMOV.S      FR8,@R6
        FMUL        FR6,FR9
        FMOV.S      FR10,FR0
        FNEG        FR11
        FMAC        FR0,FR11,FR9
        FMOV.S      FR9,@(R0,R6)
        FMUL        FR5,FR10
        FNEG        FR6
        FMOV.S      FR4,FR0
        FMAC        FR0,FR6,FR10
        MOV         #8,R0
        FMOV.S      FR10,@(R0,R6)
        FMOV.S      FR8,FR0
        FMOV.S      FR9,FR1
        FMOV.S      FR10,FR2
        FLDI0       FR3
        FLDI0       FR11
        FIPR        FV0,FV8
        FMOV.S      FR11,FR0
        FSQRT       FR0
}
#endif
/* clang-format on */
