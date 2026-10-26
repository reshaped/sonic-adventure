/* Ninja's njRotateViewYAbsolute, which MOVIE.BIN links and 1ST_READ.BIN
 * doesn't */
#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set v's direction (vx, vy, vz) to its absolute direction (avx, avy, avz)
 * turned by ang about y, from z toward x. a null v selects the current view */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8ced1354 follows the
 * body's own return at 0x8ced134a */
#pragma inline_asm(njRotateViewYAbsolute)
void njRotateViewYAbsolute(NJS_VIEW *v, Angle ang)
{
        .IMPORT     __nj_view_
        TST         R4,R4
        LDS         R5,FPUL
        BF/S        ?have_view
        FSCA        FPUL,DR0
        MOV.L       #__nj_view_,R2
        MOV.L       @R2,R4
?have_view:
        ADD         #104,R4
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR3
        ADD         #-36,R4
        FMOV.S      FR4,@R4
        FMOV        FR2,FR4
        FMUL        FR1,FR4
        FMAC        FR0,FR3,FR4
        ADD         #-4,R4
        FMOV.S      FR4,@R4
        FMUL        FR1,FR3
        FNEG        FR0
        FMAC        FR0,FR2,FR3
        ADD         #8,R4
        RTS
        FMOV.S      FR3,@R4
}
#endif
/* clang-format on */
