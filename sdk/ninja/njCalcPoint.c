#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* pd = (ps, 1) times m. a null m selects the current matrix, in XMTRX. pd may
 * be ps */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627e34 follows the
 * body's own return at 0x8c627e30 */
#pragma inline_asm(njCalcPoint)
void njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd)
{
        TST         R4,R4
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        BF/S        ?memory
        FLDI1       FR3
        ADD         #12,R6
        FTRV        XMTRX,FV0
        FMOV.S      FR2,@-R6
        FMOV.S      FR1,@-R6
        RTS
        FMOV.S      FR0,@-R6
?memory:
        MOV         #16,R0
        FMOV.S      @R4,FR4
        FMOV.S      @(R0,R4),FR5
        MOV         #32,R0
        FMOV.S      @(R0,R4),FR6
        MOV         #48,R0
        FMOV.S      @(R0,R4),FR7
        FIPR        FV0,FV4
        MOV         #4,R0
        FMOV.S      @(R0,R4),FR8
        MOV         #20,R0
        FMOV.S      @(R0,R4),FR9
        MOV         #36,R0
        FMOV.S      @(R0,R4),FR10
        MOV         #52,R0
        FMOV.S      @(R0,R4),FR11
        FMOV.S      FR7,@R6
        FIPR        FV0,FV8
        ADD         #4,R6
        MOV         #8,R0
        FMOV.S      @(R0,R4),FR4
        MOV         #24,R0
        FMOV.S      @(R0,R4),FR5
        MOV         #40,R0
        FMOV.S      @(R0,R4),FR6
        MOV         #56,R0
        FMOV.S      @(R0,R4),FR7
        FMOV.S      FR11,@R6
        FIPR        FV0,FV4
        ADD         #4,R6
        RTS
        FMOV.S      FR7,@R6
}
#endif
/* clang-format on */
