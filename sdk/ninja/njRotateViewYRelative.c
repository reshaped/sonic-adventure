#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* rotate v's direction, vx to vz, by ang about y. a null v selects the
 * current view */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b334 follows the
 * body's own return at 0x8c65b322 */
#pragma inline_asm(njRotateViewYRelative)
void njRotateViewYRelative(NJS_VIEW *v, Angle ang)
{
        .IMPORT     __nj_view_
        TST         R4,R4
        LDS         R5,FPUL
        MOV.L       #76,R0
        BF/S        ?have_view
        FSCA        FPUL,DR0
        MOV.L       #__nj_view_,R2
        MOV.L       @R2,R4
?have_view:
        FMOV.S      @(R0,R4),FR2
        MOV.L       #84,R0
        FMOV.S      @(R0,R4),FR3
        FMOV        FR2,FR4
        FMUL        FR1,FR4
        FMAC        FR0,FR3,FR4
        MOV.L       #76,R0
        FMOV.S      FR4,@(R0,R4)
        FMUL        FR1,FR3
        FNEG        FR0
        FMAC        FR0,FR2,FR3
        MOV.L       #84,R0
        RTS
        FMOV.S      FR3,@(R0,R4)
}
#endif
/* clang-format on */
