#include <ninja.h>
#include <sg_xpt.h>

Float FracInt;

/* clang-format off */
#ifndef __clang__
/* @asm open: its dead RTS at 0x8c627d6c follows a tail jump at 0x8c627d60,
 * which plain C leaves too */
#pragma inline_asm(njFraction)
Float njFraction(Float n)
{
        .IMPORT     _modff
        MOV.L       #_modff,R0
        MOV.L       #_FracInt,R4
        JMP         @R0
        NOP
}
#endif
/* clang-format on */
