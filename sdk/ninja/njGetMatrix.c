#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* copy the current matrix, in XMTRX, to m */
/* @asm epilogue: fill, the body has no return of its own and runs on through
 * SHC's .ALIGN 4 fill, the NOP at 0x8c627eca, into its RTS at 0x8c627ecc */
#pragma inline_asm(njGetMatrix)
void njGetMatrix(NJS_MATRIX *m)
{
        ADD         #64,R4
        FRCHG
        FMOV.S      FR15,@-R4
        FMOV.S      FR14,@-R4
        FMOV.S      FR13,@-R4
        FMOV.S      FR12,@-R4
        FMOV.S      FR11,@-R4
        FMOV.S      FR10,@-R4
        FMOV.S      FR9,@-R4
        FMOV.S      FR8,@-R4
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FRCHG
}
#endif
/* clang-format on */
