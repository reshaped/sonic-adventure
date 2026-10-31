#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* the square root of n, or -sqrt(-n) for a negative n */
#ifdef GCC
#undef njSqrt
Float njSqrt(Float n)
{
    return zam_njSqrt(n);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627d90 follows the
 * body's own returns at 0x8c627d88 and 0x8c627d8c */
#pragma inline_asm(njSqrt)
Float njSqrt(Float n)
{
        FLDI0       FR0
        FCMP/GT     FR4,FR0
        BF/S        ?positive
        FMOV        FR4,FR0
        FABS        FR0
        FSQRT       FR0
        RTS
        FNEG        FR0
?positive:
        RTS
        FSQRT       FR0
}
#endif
/* clang-format on */
#endif

/* 1 / sqrt(n), or -1 / sqrt(-n) for a negative n, to FSRRA's precision */
#ifdef GCC
#undef njInvertSqrt
Float njInvertSqrt(Float n)
{
    return zam_njInvertSqrt(n);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627da8 follows the
 * body's own returns at 0x8c627da0 and 0x8c627da4 */
#pragma inline_asm(njInvertSqrt)
Float njInvertSqrt(Float n)
{
        FLDI0       FR0
        FCMP/GT     FR4,FR0
        BF/S        ?positive
        FMOV        FR4,FR0
        FABS        FR0
        FSRRA       FR0
        RTS
        FNEG        FR0
?positive:
        RTS
        FSRRA       FR0
}
#endif
/* clang-format on */
#endif
