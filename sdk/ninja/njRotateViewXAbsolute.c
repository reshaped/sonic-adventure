#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set v's direction, vx to vz, to its absolute direction, avx to avz, rotated
 * by ang about x. a null v selects the current view */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b2c0 follows the
 * body's own return at 0x8c65b2b6 */
#pragma inline_asm(njRotateViewXAbsolute)
void njRotateViewXAbsolute(NJS_VIEW *v, Angle ang)
{
        .IMPORT     __nj_view_
        LDS         R5,FPUL
        TST         R4,R4
        BF/S        ?have_view
        FSCA        FPUL,DR0
        MOV.L       #__nj_view_,R2
        MOV.L       @R2,R4
?have_view:
        ADD         #104,R4
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR3
        ADD         #-40,R4
        FMOV.S      FR4,@R4
        FMOV        FR3,FR4
        FMUL        FR1,FR4
        FMAC        FR0,FR2,FR4
        ADD         #8,R4
        FMOV.S      FR4,@R4
        FMUL        FR1,FR2
        FNEG        FR0
        FMAC        FR0,FR3,FR2
        ADD         #-4,R4
        RTS
        FMOV.S      FR2,@R4
}
#endif
/* clang-format on */
