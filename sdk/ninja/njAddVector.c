#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* add vs to vd */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627dd8 follows the
 * body's own return at 0x8c627dd2 */
#pragma inline_asm(njAddVector)
void njAddVector(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
        FMOV.S      @R4+,FR4
        FMOV.S      @R5+,FR1
        FMOV.S      @R4+,FR5
        FADD        FR4,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R4,FR6
        FADD        FR5,FR2
        FMOV.S      @R5,FR3
        FADD        FR6,FR3
        FMOV.S      FR3,@R4
        FMOV.S      FR2,@-R4
        RTS
        FMOV.S      FR1,@-R4
}
#endif
/* clang-format on */
