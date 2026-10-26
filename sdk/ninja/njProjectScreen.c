#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* project p3, transformed by m, onto the screen into p2. with x, y and z the
 * transformed point, p2->x = _nj_screen_.cx + _nj_screen_.dist * x / z *
 * _nj_scr_aspect_x_, and p2->y likewise from y and _nj_scr_aspect_y_. a null m
 * selects the current matrix, in XMTRX */
/* @asm operand: fipr-registers, FIPR FV8,FV0 at 0x8c628b2a, 0x8c628b3e and
 * 0x8c628b52 */
#pragma inline_asm(njProjectScreen)
void njProjectScreen(NJS_MATRIX *m, NJS_POINT3 *p3, NJS_POINT2 *p2)
{
        .IMPORT     __nj_scr_aspect_x_
        .IMPORT     __nj_scr_aspect_y_
        .IMPORT     __nj_screen_
        MOV.L       #__nj_screen_,R3
        TST         R4,R4
        BF          ?memory
        FMOV.S      @R5+,FR4
        FLDI1       FR7
        FMOV.S      @R5+,FR5
        FMOV.S      @R5,FR6
        FTRV        XMTRX,FV4
?divide:
        FLDI1       FR7
        FDIV        FR6,FR7
        FMOV.S      @R3,FR2
        FMUL        FR4,FR2
        FMUL        FR7,FR2
        MOV.L       #__nj_scr_aspect_x_,R2
        FMOV.S      @R2,FR1
        FMUL        FR1,FR2
        MOV         #12,R0
        FMOV.S      @(R0,R3),FR1
        FADD        FR1,FR2
        FMOV.S      @R3,FR3
        FMUL        FR5,FR3
        FMUL        FR7,FR3
        MOV.L       #__nj_scr_aspect_y_,R2
        FMOV.S      @R2,FR1
        FMUL        FR1,FR3
        MOV         #16,R0
        FMOV.S      @(R0,R3),FR1
        FADD        FR1,FR3
        MOV         #4,R0
        FMOV.S      FR2,@R6
        RTS
        FMOV.S      FR3,@(R0,R6)
?memory:
        FMOV.S      @R5+,FR8
        FLDI1       FR11
        FMOV.S      @R5+,FR9
        FMOV.S      @R5,FR10
        MOV         #16,R0
        FMOV.S      @R4,FR0
        FMOV.S      @(R0,R4),FR1
        MOV         #32,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #48,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV8,FV0
        FMOV.S      FR3,FR4
        MOV         #4,R0
        FMOV.S      @(R0,R4),FR0
        MOV         #20,R0
        FMOV.S      @(R0,R4),FR1
        MOV         #36,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #52,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV8,FV0
        FMOV.S      FR3,FR5
        MOV         #8,R0
        FMOV.S      @(R0,R4),FR0
        MOV         #24,R0
        FMOV.S      @(R0,R4),FR1
        MOV         #40,R0
        FMOV.S      @(R0,R4),FR2
        MOV         #56,R0
        FMOV.S      @(R0,R4),FR3
        FIPR        FV8,FV0
        BRA         ?divide
        FMOV.S      FR3,FR6
}
#endif
/* clang-format on */
