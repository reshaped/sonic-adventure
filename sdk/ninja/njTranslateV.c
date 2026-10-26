#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set m to T * m, where T translates by v's x, y and z. a null m selects the
 * current matrix, in XMTRX */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c629754 follows the
 * body's own return at 0x8c629750 */
#pragma inline_asm(njTranslateV)
void njTranslateV(NJS_MATRIX *m, NJS_VECTOR *v)
{
        FMOV.S      @R5+,FR4
        TST         R4,R4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        BT/S        ?current
        FLDI1       FR7
        MOV         #16,R0
        FMOV.S      @R4,FR0
        FMOV.S      @(R0,R4),FR1
        MOV         #32,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #48,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV4,FV0
        MOV         #48,R0
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
        MOV         #52,R0
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
        MOV         #56,R0
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
        MOV         #60,R0
        RTS
        FMOV.S      FR3,@(R0,R4)
?current:
        FTRV        XMTRX,FV4
        FSCHG
        FMOV        DR4,XD12
        FMOV        DR6,XD14
        RTS
        FSCHG
}
#endif
/* clang-format on */
