#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set v's position, px to pz, to its absolute position, apx to apz, moved by
 * x, y and z. a null v selects the current view */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b4f0 follows the
 * body's own return at 0x8c65b4e8 */
#pragma inline_asm(njTranslateViewAbsolute)
void njTranslateViewAbsolute(NJS_VIEW *v, Float x, Float y, Float z)
{
        .IMPORT     __nj_view_
        TST         R4,R4
        BF          ?have_view
        MOV.L       #__nj_view_,R2
        MOV.L       @R2,R4
?have_view:
        ADD         #92,R4
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FADD        FR0,FR4
        FMOV.S      @R4+,FR2
        FADD        FR1,FR5
        FADD        FR2,FR6
        ADD         #-28,R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        RTS
        FMOV.S      FR4,@-R4
}
#endif
/* clang-format on */
