#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* v1.x * v2.x + v1.y * v2.y + v1.z * v2.z, as FIPR computes it */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627f98 follows the
 * body's own return at 0x8c627f92 */
#pragma inline_asm(njInnerProduct)
Float njInnerProduct(NJS_VECTOR *v1, NJS_VECTOR *v2)
{
        MOV         #4,R0
        FMOV.S      @R4,FR0
        FMOV.S      @R5,FR4
        FMOV.S      @(R0,R4),FR1
        FMOV.S      @(R0,R5),FR5
        MOV         #8,R0
        FLDI0       FR7
        FMOV.S      @(R0,R4),FR2
        FMOV.S      @(R0,R5),FR6
        FLDI0       FR3
        FIPR        FV4,FV0
        RTS
        FMOV        FR3,FR0
}
#endif
/* clang-format on */
