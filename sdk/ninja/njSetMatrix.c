#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* md = ms. a null md selects the current matrix, in XMTRX */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c6296c0 follows the
 * body's own return at 0x8c6296bc */
#pragma inline_asm(njSetMatrix)
void njSetMatrix(NJS_MATRIX *md, NJS_MATRIX *ms)
{
        TST         R4,R4
        BF          ?memory
        FRCHG
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FMOV.S      @R5+,FR8
        FMOV.S      @R5+,FR9
        FMOV.S      @R5+,FR10
        FMOV.S      @R5+,FR11
        FMOV.S      @R5+,FR12
        FMOV.S      @R5+,FR13
        FMOV.S      @R5+,FR14
        FMOV.S      @R5+,FR15
        RTS
        FRCHG
?memory:
        ADD         #32,R4
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        ADD         #64,R4
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        RTS
        FMOV.S      FR0,@-R4
}
#endif
/* clang-format on */
