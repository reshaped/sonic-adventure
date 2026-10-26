#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* make m the matrix stack, with room for n matrices, holding one: a scale of
 * -1 in x and y, the current matrix, in m[0] and in XMTRX. _nj_matrix_flag_
 * takes flag */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627f78 follows the
 * body's own return at 0x8c627f5a */
#pragma inline_asm(njInitMatrix)
void njInitMatrix(NJS_MATRIX *m, Sint32 n, Int flag)
{
        .IMPORT     __max_stack_size_
        .IMPORT     __nj_base_matrix_ptr_
        .IMPORT     __nj_current_matrix_ptr_
        .IMPORT     __nj_matrix_flag_
        .IMPORT     __stack_size_
        .IMPORT     __unit_matrix_
        MOV.L       #__stack_size_,R0
        MOV         #1,R7
        MOV.L       #__nj_matrix_flag_,R2
        MOV.L       R7,@R0
        MOV.L       #__max_stack_size_,R1
        MOV.L       R6,@R2
        MOV.L       #__nj_current_matrix_ptr_,R0
        MOV.L       #__nj_base_matrix_ptr_,R3
        MOV.L       R4,@R0
        MOV.L       R5,@R1
        MOV.L       R4,@R3
        MOV.L       #__unit_matrix_,R1
        ADD         #64,R4
        ADD         #32,R1
        FMOV.S      @R1+,FR0
        FMOV.S      @R1+,FR1
        FMOV.S      @R1+,FR2
        FMOV.S      @R1+,FR3
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1+,FR6
        FMOV.S      @R1+,FR7
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        ADD         #-64,R1
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      @R1+,FR0
        FMOV.S      @R1+,FR1
        FMOV.S      @R1+,FR2
        FMOV.S      @R1+,FR3
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1+,FR6
        FMOV.S      @R1+,FR7
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FNEG        FR0
        MOV         #20,R0
        FNEG        FR5
        FMOV.S      FR0,@R4
        FMOV.S      FR5,@(R0,R4)
        FRCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR3
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR5
        FMOV.S      @R4+,FR6
        FMOV.S      @R4+,FR7
        FMOV.S      @R4+,FR8
        FMOV.S      @R4+,FR9
        FMOV.S      @R4+,FR10
        FMOV.S      @R4+,FR11
        FMOV.S      @R4+,FR12
        FMOV.S      @R4+,FR13
        FMOV.S      @R4+,FR14
        FMOV.S      @R4+,FR15
        RTS
        FRCHG
}
#endif
/* clang-format on */
