/* the matrix stack's push that starts the new top as a unit matrix */
#include <sg_xpt.h>

#include "system/vector.h"

/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static void push_body(void);

/* clang-format off */
#ifndef __clang__
#ifndef GCC
/* expanded into matrix_push_unit. when the stack is full, its RTS returns
 * FALSE from matrix_push_unit at once, and it pushes nothing. otherwise it
 * writes XMTRX, the current matrix, back to the top's slot, and moves the
 * top up a slot. it then makes XMTRX a unit matrix and stores it in the new
 * top's slot too. the unit matrix is made of three pairs: DR0 holds 1 and
 * 0, DR2 0 and 1, and DR4 0 and 0 */
/* @asm open: no inspect rule reads its FSCHG under SHC 5.0 R10 */
#pragma inline_asm(push_body)
static void push_body(void)
{
        .IMPORT     __max_stack_size_
        .IMPORT     __stack_size_
        .IMPORT     __nj_current_matrix_ptr_
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
        ADD         #64,R5
        MOV.L       R5,@R1
        MOV         R5,R4
        FLDI1       FR0
        FLDI0       FR1
        FLDI0       FR2
        FLDI1       FR3
        FLDI0       FR4
        FLDI0       FR5
        FSCHG
        FMOV        XD14,@-R5
        FMOV        XD12,@-R5
        FMOV        XD10,@-R5
        FMOV        XD8,@-R5
        FMOV        XD6,@-R5
        FMOV        XD4,@-R5
        FMOV        XD2,@-R5
        FMOV        XD0,@-R5
        ADD         #64,R4
        FMOV        DR0,XD0
        FMOV        DR4,XD2
        FMOV        DR2,XD4
        FMOV        DR4,XD6
        FMOV        DR4,XD8
        FMOV        DR0,XD10
        FMOV        DR4,XD12
        FMOV        DR2,XD14
        FMOV        XD14,@-R4
        FMOV        XD12,@-R4
        FMOV        XD10,@-R4
        FMOV        XD8,@-R4
        FMOV        XD6,@-R4
        FMOV        XD4,@-R4
        FMOV        XD2,@-R4
        FMOV        XD0,@-R4
        FSCHG
}
#endif
#endif
/* clang-format on */

Bool matrix_push_unit(void)
{
#ifdef GCC
    /* SHC expands push_body here, so its RTS returns FALSE from
     * matrix_push_unit itself. GCC makes an inline body a function of its
     * own, whose RTS would return to matrix_push_unit, so GCC pushes in C.
     * njPushMatrix pushes nothing on a full stack and returns FALSE. after
     * a push, XMTRX and the new top's slot each take the unit matrix */
    if (!njPushMatrix(NULL)) {
        return FALSE;
    }
    njUnitMatrix(NULL);
    njUnitMatrix(_nj_current_matrix_ptr_);
#else
    push_body();
#endif
    return TRUE;
}
