#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* rotate v's direction, vx to vz, by ang about x. a null v selects the
 * current view */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b2f8 follows the
 * body's own return at 0x8c65b2e8 */
#pragma inline_asm(njRotateViewXRelative)
void njRotateViewXRelative(NJS_VIEW *v, Angle ang)
{
        .IMPORT     __nj_view_
        LDS         R5,FPUL
        TST         R4,R4
        MOV.L       #80,R0
        BF/S        ?have_view
        FSCA        FPUL,DR0
        MOV.L       #__nj_view_,R2
        MOV.L       @R2,R4
?have_view:
        FMOV.S      @(R0,R4),FR2
        MOV.L       #84,R0
        FMOV.S      @(R0,R4),FR3
        FMOV        FR3,FR4
        FMUL        FR1,FR4
        FMAC        FR0,FR2,FR4
        FMUL        FR1,FR2
        FNEG        FR0
        FMOV.S      FR4,@(R0,R4)
        FMAC        FR0,FR3,FR2
        MOV.L       #80,R0
        RTS
        FMOV.S      FR2,@(R0,R4)
}
#endif
/* clang-format on */
