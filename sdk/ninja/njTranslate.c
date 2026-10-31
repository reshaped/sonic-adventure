#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* set m to T * m, where T translates by x, y and z. a null m selects the
 * current matrix, in XMTRX */
#ifdef GCC
#undef njTranslate
void njTranslate(NJS_MATRIX *m, Float x, Float y, Float z)
{
    zam_njTranslate(m, x, y, z);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c6297bc follows the
 * body's own return at 0x8c6297b6 */
#pragma inline_asm(njTranslate)
void njTranslate(NJS_MATRIX *m, Float x, Float y, Float z)
{
        TST         R4,R4
        BF/S        ?memory
        FLDI1       FR7
        FTRV        XMTRX,FV4
        FSCHG
        FMOV        DR4,XD12
        FMOV        DR6,XD14
        RTS
        FSCHG
?memory:
        FMOV.S      @R4,FR0
        MOV         #16,R0
        FMOV.S      @(R0,R4),FR1
        MOV         #32,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #48,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,@(R0,R4)
        MOV         #4,R0
        FMOV.S      @(R0,R4),FR0
        MOV         #20,R0
        FMOV.S      @(R0,R4),FR1
        MOV         #36,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #52,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,@(R0,R4)
        MOV         #8,R0
        FMOV.S      @(R0,R4),FR0
        MOV         #24,R0
        FMOV.S      @(R0,R4),FR1
        MOV         #40,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #56,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,@(R0,R4)
        MOV         #12,R0
        FMOV.S      @(R0,R4),FR0
        MOV         #28,R0
        FMOV.S      @(R0,R4),FR1
        MOV         #44,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #60,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV4,FV0
        RTS
        FMOV.S      FR3,@(R0,R4)
}
#endif
/* clang-format on */
#endif
