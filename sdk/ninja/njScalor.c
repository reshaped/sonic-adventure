#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* the length of v: the square root of x * x + y * y + z * z */
#ifdef GCC
#undef njScalor
Float njScalor(NJS_VECTOR *v)
{
    return zam_njScalor(v);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c62964c follows the
 * body's own return at 0x8c629648 */
#pragma inline_asm(njScalor)
Float njScalor(NJS_VECTOR *v)
{
        FLDI0       FR7
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR5
        FMOV.S      @R4+,FR6
        FIPR        FV4,FV4
        FSQRT       FR7
        RTS
        FMOV.S      FR7,FR0
}
#endif
/* clang-format on */
#endif
