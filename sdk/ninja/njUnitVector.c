#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* scale v to unit length and return its former length */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c62984c follows the
 * body's own return at 0x8c629848 */
#pragma inline_asm(njUnitVector)
Float njUnitVector(NJS_VECTOR *v)
{
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FLDI0       FR3
        FIPR        FV0,FV0
        FMOV.S      FR0,FR4
        FMOV.S      FR3,FR0
        FSRRA       FR3
        FMUL        FR3,FR2
        FMUL        FR3,FR1
        FMUL        FR3,FR4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR4,@-R4
        RTS
        FMUL        FR3,FR0
}
#endif
/* clang-format on */
