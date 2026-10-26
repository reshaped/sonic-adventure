#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627d38 follows the
 * body's own return at 0x8c627d2c */
#pragma inline_asm(njSetLightRange)
void njSetLightRange(NJS_LIGHT *l, Float nrang, Float frang)
{
    MOV.W       #H'0138,R3
    MOV.W       #H'006C,R2
    FMOV.S      FR5,FR3
    FMUL        FR3,FR3
    ADD         R4,R3
    FMOV.S      FR4,FR2
    FMUL        FR2,FR2
    ADD         R4,R2
    FMOV.S      FR5,@R3
    FMOV.S      FR4,@-R3
    FMOV.S      FR3,@R2
    FMOV.S      FR2,@-R2
    FMOV.S      FR5,FR6
    FLDI1       FR1
    FSUB        FR4,FR5
    MOV.W       #H'0078,R0
    FDIV        FR5,FR1
    FMOV.S      FR1,@(R0,R4)
    MOV.W       #H'007C,R0
    FMUL        FR1,FR6
    RTS
    FMOV.S      FR6,@(R0,R4)
}
#endif
/* clang-format on */
