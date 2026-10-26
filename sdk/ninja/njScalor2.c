#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* the square of v's length: x * x + y * y + z * z */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c629638 follows the
 * body's own return at 0x8c629632 */
#pragma inline_asm(njScalor2)
Float njScalor2(NJS_VECTOR *v)
{
        FLDI0       FR7
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR5
        FMOV.S      @R4+,FR6
        FIPR        FV4,FV4
        RTS
        FMOV.S      FR7,FR0
}
#endif
/* clang-format on */
