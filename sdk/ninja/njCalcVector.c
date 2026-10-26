#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* vd = (vs, 0) times m, made unit length, to FSRRA's precision, while
 * _nj_matrix_flag_ is set. a null m selects the current matrix, in XMTRX. vd
 * may be vs */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627ea0 follows the
 * body's own return at 0x8c627e98 */
#pragma inline_asm(njCalcVector)
void njCalcVector(NJS_MATRIX *m, NJS_VECTOR *vs, NJS_VECTOR *vd)
{
        .IMPORT     __nj_matrix_flag_
        FMOV.S      @R5+,FR0
        TST         R4,R4
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        BF/S        ?memory
        FLDI0       FR3
        BRA         ?unit
        FTRV        XMTRX,FV0
?memory:
        MOV         #16,R0
        FMOV.S      @R4,FR4
        FMOV.S      @(R0,R4),FR5
        MOV         #32,R0
        FMOV.S      @(R0,R4),FR6
        FLDI0       FR7
        FIPR        FV0,FV4
        MOV         #4,R0
        FMOV.S      @(R0,R4),FR8
        MOV         #20,R0
        FMOV.S      @(R0,R4),FR9
        MOV         #36,R0
        FMOV.S      @(R0,R4),FR10
        FLDI0       FR11
        FIPR        FV0,FV8
        FMOV.S      FR7,FR10
        MOV         #8,R0
        FMOV.S      @(R0,R4),FR4
        MOV         #24,R0
        FMOV.S      @(R0,R4),FR5
        MOV         #40,R0
        FMOV.S      @(R0,R4),FR6
        FLDI0       FR7
        FIPR        FV0,FV4
        FMOV.S      FR10,FR0
        FMOV.S      FR11,FR1
        FMOV.S      FR7,FR2
?unit:
        FLDI0       FR3
        MOV.L       #__nj_matrix_flag_,R3
        MOV.L       @R3,R3
        TST         R3,R3
        BT/S        ?store
        ADD         #12,R6
        FIPR        FV0,FV0
        FSRRA       FR3
        FMUL        FR3,FR2
        FMUL        FR3,FR1
        FMUL        FR3,FR0
?store:
        FMOV.S      FR2,@-R6
        FMOV.S      FR1,@-R6
        RTS
        FMOV.S      FR0,@-R6
}
#endif
/* clang-format on */
