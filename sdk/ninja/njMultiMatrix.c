#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* md = ms * md, for row-major matrices. a null md selects the current matrix,
 * in XMTRX. ms is read whole before md is written, so the two may overlap.
 * FPSCR.SZ and FPSCR.PR must be 0 on entry: the body's FMOV.S moves need SZ
 * 0, and its FIPR and FTRV need PR 0 */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c628a34 follows the
 * body's own return at 0x8c628a30 */
#pragma inline_asm(njMultiMatrix)
void njMultiMatrix(NJS_MATRIX *md, NJS_MATRIX *ms)
{
        TST         R4,R4
        BF          ?STORED_MATRIX
        ; ms rows 0 and 1 through XMTRX, kept on the stack
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FMOV.S      FR7,@-R15
        FMOV.S      FR6,@-R15
        FMOV.S      FR5,@-R15
        FMOV.S      FR4,@-R15
        FMOV.S      FR3,@-R15
        FMOV.S      FR2,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR0,@-R15
        ; rows 2 and 3, then all four products into XMTRX as pairs
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FSCHG
        FMOV        DR6,XD14
        FMOV        DR4,XD12
        FMOV        DR2,XD10
        FMOV        DR0,XD8
        FSCHG
        FMOV.S      @R15+,FR0
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR6
        FMOV.S      @R15+,FR7
        FSCHG
        FMOV        DR6,XD6
        FMOV        DR4,XD4
        FMOV        DR2,XD2
        FMOV        DR0,XD0
        RTS
        FSCHG
?STORED_MATRIX:
        ; column 0 of md dotted with each row of ms, the products pushed
        MOV         R5,R6
        MOV         #16,R0
        FMOV.S      @R4,FR8
        FMOV.S      @(R0,R4),FR9
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR10
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR11
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        ; column 1
        MOV         R6,R5
        MOV         #4,R0
        FMOV.S      @(R0,R4),FR8
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR9
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR10
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR11
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        ; column 2
        MOV         R6,R5
        MOV         #8,R0
        FMOV.S      @(R0,R4),FR8
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR9
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR10
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR11
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        ; column 3
        MOV         R6,R5
        MOV         #12,R0
        FMOV.S      @(R0,R4),FR8
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR9
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR10
        ADD         #16,R0
        FMOV.S      @(R0,R4),FR11
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR3
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      @R5+,FR7
        FIPR        FV8,FV0
        FIPR        FV8,FV4
        FMOV.S      FR3,@-R15
        FMOV.S      FR7,@-R15
        ; pop the products of columns 3 and 2 into rows 3 down to 0
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR7
        FMOV.S      @R15+,FR0
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR6
        MOV         #60,R0
        FMOV.S      FR1,@(R0,R4)
        MOV         #56,R0
        FMOV.S      FR0,@(R0,R4)
        MOV         #44,R0
        FMOV.S      FR3,@(R0,R4)
        MOV         #40,R0
        FMOV.S      FR2,@(R0,R4)
        MOV         #28,R0
        FMOV.S      FR5,@(R0,R4)
        MOV         #24,R0
        FMOV.S      FR4,@(R0,R4)
        MOV         #12,R0
        FMOV.S      FR7,@(R0,R4)
        MOV         #8,R0
        FMOV.S      FR6,@(R0,R4)
        ; then columns 1 and 0
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR7
        FMOV.S      @R15+,FR0
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR6
        MOV         #52,R0
        FMOV.S      FR1,@(R0,R4)
        MOV         #48,R0
        FMOV.S      FR0,@(R0,R4)
        MOV         #36,R0
        FMOV.S      FR3,@(R0,R4)
        MOV         #32,R0
        FMOV.S      FR2,@(R0,R4)
        MOV         #20,R0
        FMOV.S      FR5,@(R0,R4)
        MOV         #16,R0
        FMOV.S      FR4,@(R0,R4)
        MOV         #4,R0
        FMOV.S      FR7,@(R0,R4)
        RTS
        FMOV.S      FR6,@R4
}
#endif
/* clang-format on */

/* the body only returns, so the image can't show what parameters the SDK
 * gave this function, and this definition takes none */
void njMultiMatrix_old(void)
{
}
