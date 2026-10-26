#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* the tangent of n, as FSCA's sine of n over its cosine. only n's low 16 bits
 * count, as 65536 is a full turn */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627db8 follows the
 * body's own return at 0x8c627db2 */
#pragma inline_asm(njTan)
Float njTan(Angle n)
{
        EXTU.W      R4,R4
        LDS         R4,FPUL
        FSCA        FPUL,DR0
        RTS
        FDIV        FR1,FR0
}
#endif
/* clang-format on */
