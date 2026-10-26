#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c668d90 follows the
 * body's own return at 0x8c668d6c */
#pragma inline_asm(njClearMatrix)
void njClearMatrix(void)
{
        .IMPORT     __nj_base_matrix_ptr_
        .IMPORT     __nj_current_matrix_ptr_
        .IMPORT     __nj_view_
        .IMPORT     __nj_view_matrix_
        .IMPORT     __stack_size_
        .IMPORT     __unit_matrix_
        .IMPORT     _njMultiMatrix
        .IMPORT     _njScale
        MOV         #1,R1
        MOV.L       R14,@-R15
        MOV.L       #__stack_size_,R0
        STS.L       PR,@-R15
        MOV.L       #__nj_base_matrix_ptr_,R14
        MOV.L       #__unit_matrix_,R5
        MOV.L       @R14,R14
        MOV.L       R1,@R0
        FSCHG
        FMOV.S      @R14+,FR1
        FMOV.S      @R14+,FR3
        FMOV.S      @R14+,FR5
        FMOV.S      @R14+,FR7
        FMOV.S      @R14+,FR9
        FMOV.S      @R14+,FR11
        FMOV.S      @R14+,FR13
        FMOV.S      @R14+,FR15
        ADD         #32,R5
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR6
        FMOV.S      FR6,@-R14
        ADD         #-64,R5
        FMOV.S      FR4,@-R14
        FMOV.S      FR2,@-R14
        FMOV.S      FR0,@-R14
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR6
        FMOV.S      FR6,@-R14
        FMOV.S      FR4,@-R14
        FMOV.S      FR2,@-R14
        FMOV.S      FR0,@-R14
        FSCHG
        MOV.L       #__nj_current_matrix_ptr_,R2
        MOV.L       R14,@R2
        FLDI1       FR4
        MOV.L       #_njScale,R3
        FNEG        FR4
        FLDI1       FR6
        FMOV.S      FR4,FR5
        JSR         @R3
        MOV         R14,R4
        MOV.L       #__nj_view_,R3
        MOV.L       #_njMultiMatrix,R2
        MOV.L       @R3,R5
        JSR         @R2
        MOV         R14,R4
        MOV.L       #__nj_view_matrix_,R2
        MOV         R14,R5
        ADD         #64,R2
        FSCHG
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR7
        FMOV.S      @R5+,FR9
        FMOV.S      @R5+,FR11
        FMOV.S      @R5+,FR13
        FMOV.S      @R5+,FR15
        FMOV.S      FR15,@-R2
        FMOV.S      FR13,@-R2
        FMOV.S      FR11,@-R2
        FMOV.S      FR9,@-R2
        FMOV.S      FR7,@-R2
        FMOV.S      FR5,@-R2
        FMOV.S      FR3,@-R2
        FMOV.S      FR1,@-R2
        FSCHG
        LDS.L       @R15+,PR
        RTS
        MOV.L       @R15+,R14
}
#endif
/* clang-format on */
