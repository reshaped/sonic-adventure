#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* @asm open: its dead RTS at 0x8c615158 follows a tail jump at 0x8c615154,
 * which plain C leaves too */
#pragma inline_asm(njRoundOff)
Float njRoundOff(Float n)
{
        .IMPORT     _floorf
        MOV.L       #_floorf,R0
        FLDI0       FR0
        FCMP/GT     FR4,FR0
        BF/S        ?positive
        FABS        FR4
        STS.L       PR,@-R15
        JSR         @R0
        NOP
        LDS.L       @R15+,PR
        RTS
        FNEG        FR0
?positive:
        JMP         @R0
        NOP
}
#endif
/* clang-format on */
