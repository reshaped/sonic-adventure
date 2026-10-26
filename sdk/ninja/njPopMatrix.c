#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* pop n matrices: move the current matrix pointer down n slots, and load the
 * matrix there into XMTRX as the current matrix. returns TRUE, or FALSE,
 * popping nothing, when n equals the stack depth. it pops wherever the depth
 * less n, taken unsigned, is at least 1, so an n above the depth pops too:
 * the depth goes negative, and the pointer moves below the stack's base */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c628ac4 follows the
 * body's own return at 0x8c628abc */
#pragma inline_asm(njPopMatrix)
Bool njPopMatrix(Uint32 n)
{
        .IMPORT     __nj_current_matrix_ptr_
        .IMPORT     __stack_size_
        MOV.L       #__stack_size_,R5
        MOV         #1,R2
        MOV.L       @R5,R3
        SUB         R4,R3
        CMP/HS      R2,R3
        BT          ?pop
        RTS
        MOV         #0,R0
?pop:
        SHLL2       R4
        MOV.L       R3,@R5
        SHLL2       R4
        MOV.L       #__nj_current_matrix_ptr_,R5
        SHLL2       R4
        MOV.L       @R5,R1
        SUB         R4,R1
        MOV.L       R1,@R5
        FSCHG
        FMOV        @R1+,XD0
        FMOV        @R1+,XD2
        FMOV        @R1+,XD4
        FMOV        @R1+,XD6
        FMOV        @R1+,XD8
        FMOV        @R1+,XD10
        FMOV        @R1+,XD12
        FMOV        @R1+,XD14
        FSCHG
        RTS
        MOV         #1,R0
}
#endif
/* clang-format on */
