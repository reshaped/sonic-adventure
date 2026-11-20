#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* the sine of n, to FSCA's precision. only n's low 16 bits count, as 65536 is
 * a full turn */
#ifdef DREAMCAST
#ifdef NONMATCHING
#undef njSin
Float njSin(Angle n)
{
    return zam_njSin(n);
}
#else
/* clang-format off */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627d78 follows the
 * body's own return at 0x8c627d74 */
#pragma inline_asm(njSin)
Float njSin(Angle n)
{
        EXTU.W      R4,R4
        LDS         R4,FPUL
        RTS
        FSCA        FPUL,DR0
}
/* clang-format on */
#endif
#endif
