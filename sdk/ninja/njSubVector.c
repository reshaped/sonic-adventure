#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* subtract vs from vd */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c6296e0 follows the
 * body's own return at 0x8c6296da */
#pragma inline_asm(njSubVector)
void njSubVector(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
        FMOV.S      @R5+,FR1
        FMOV.S      @R4+,FR4
        FMOV.S      @R5+,FR2
        FSUB        FR1,FR4
        FMOV.S      @R4+,FR5
        FMOV.S      @R5,FR3
        FSUB        FR2,FR5
        FMOV.S      @R4,FR6
        FSUB        FR3,FR6
        FMOV.S      FR6,@R4
        FMOV.S      FR5,@-R4
        RTS
        FMOV.S      FR4,@-R4
}
#endif
/* clang-format on */
