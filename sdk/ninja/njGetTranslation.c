#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set *p to the translation of m, its last row. a null m selects the current
 * matrix, in XMTRX */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c61749c follows the
 * body's own return at 0x8c617498 */
#pragma inline_asm(njGetTranslation)
void njGetTranslation(NJS_MATRIX *m, NJS_POINT3 *p)
{
        ADD         #12,R5
        TST         R4,R4
        BF          ?memory
        FSCHG
        FMOV        XD12,DR0
        FMOV        XD14,DR2
        BRA         ?store
        FSCHG
?memory:
        ADD         #48,R4
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
?store:
        FMOV.S      FR2,@-R5
        FMOV.S      FR1,@-R5
        RTS
        FMOV.S      FR0,@-R5
}
#endif
/* clang-format on */
