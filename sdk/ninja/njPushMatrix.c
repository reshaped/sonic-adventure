#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* push the current matrix: store XMTRX in the current matrix pointer's slot,
 * and move the pointer to the next slot. with m null, XMTRX keeps its matrix,
 * which is stored in the new slot too. otherwise XMTRX takes m, and the new
 * slot is left as it was. returns TRUE, or FALSE, pushing nothing, when the
 * stack is full */
#ifdef GCC
/* ninja_zam.h defines no njPushMatrix macro, so every call reaches this
 * definition */
Bool njPushMatrix(NJS_MATRIX *m)
{
    return zam_njPushMatrix(m);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c628bd0 follows the
 * body's own return at 0x8c628bcc */
#pragma inline_asm(njPushMatrix)
Bool njPushMatrix(NJS_MATRIX *m)
{
        .IMPORT     __max_stack_size_
        .IMPORT     __nj_current_matrix_ptr_
        .IMPORT     __stack_size_
        MOV.L       #__max_stack_size_,R1
        MOV.L       #__stack_size_,R6
        MOV.L       @R1,R3
        MOV.L       @R6,R2
        CMP/GE      R3,R2
        BF          ?push
        RTS
        MOV         #0,R0
?push:
        ADD         #1,R2
        MOV.L       #__nj_current_matrix_ptr_,R1
        MOV.L       R2,@R6
        MOV.L       @R1,R5
        TST         R4,R4
        ADD         #64,R5
        FSCHG
        FMOV        XD14,@-R5
        FMOV        XD12,@-R5
        FMOV        XD10,@-R5
        FMOV        XD8,@-R5
        FMOV        XD6,@-R5
        FMOV        XD4,@-R5
        FMOV        XD2,@-R5
        FMOV        XD0,@-R5
        FSCHG
        ADD         #64,R5
        BF/S        ?load
        MOV.L       R5,@R1
        ADD         #64,R5
        FSCHG
        FMOV        XD14,@-R5
        FMOV        XD12,@-R5
        FMOV        XD10,@-R5
        FMOV        XD8,@-R5
        FMOV        XD6,@-R5
        FMOV        XD4,@-R5
        FMOV        XD2,@-R5
        FMOV        XD0,@-R5
        FSCHG
        RTS
        MOV         #1,R0
?load:
        FSCHG
        FMOV        @R4+,XD0
        FMOV        @R4+,XD2
        FMOV        @R4+,XD4
        FMOV        @R4+,XD6
        FMOV        @R4+,XD8
        FMOV        @R4+,XD10
        FMOV        @R4+,XD12
        FMOV        @R4+,XD14
        FSCHG
        RTS
        MOV         #1,R0
}
#endif
/* clang-format on */
#endif
