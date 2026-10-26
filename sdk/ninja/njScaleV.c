#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set m to S * m, where S scales by v's x, y and z. a null m selects the
 * current matrix, in XMTRX */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c629578 follows the
 * body's own return at 0x8c629574 */
#pragma inline_asm(njScaleV)
void njScaleV(NJS_MATRIX *m, NJS_VECTOR *v)
{
        TST         R4,R4
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        BF/S        ?memory
        FMOV.S      @R5+,FR6
        FSCHG
        FMOV        XD0,DR0
        FMOV        XD2,DR2
        FMOV        XD4,DR8
        FMOV        XD6,DR10
        FSCHG
        FMUL        FR4,FR0
        FMUL        FR4,FR1
        FMUL        FR4,FR2
        FMUL        FR4,FR3
        FMUL        FR5,FR8
        FMUL        FR5,FR9
        FMUL        FR5,FR10
        FMUL        FR5,FR11
        FSCHG
        FMOV        DR0,XD0
        FMOV        DR2,XD2
        FMOV        DR8,XD4
        FMOV        DR10,XD6
        FSCHG
        FSCHG
        FMOV        XD8,DR0
        FMOV        XD10,DR2
        FSCHG
        FMUL        FR6,FR0
        FMUL        FR6,FR1
        FMUL        FR6,FR2
        FMUL        FR6,FR3
        FSCHG
        FMOV        DR0,XD8
        FMOV        DR2,XD10
        RTS
        FSCHG
?memory:
        MOV         R4,R5
        FMOV.S      @R4+,FR3
        FMUL        FR4,FR3
        FMOV.S      @R4+,FR2
        MOV         #4,R0
        FMUL        FR4,FR2
        FMOV.S      FR3,@R5
        FMOV.S      @R4+,FR1
        FMOV.S      FR2,@(R0,R5)
        FMUL        FR4,FR1
        FMOV.S      @R4+,FR3
        MOV         #8,R0
        FMUL        FR4,FR3
        FMOV.S      FR1,@(R0,R5)
        FMOV.S      @R4+,FR2
        MOV         #12,R0
        FMUL        FR5,FR2
        FMOV.S      FR3,@(R0,R5)
        FMOV.S      @R4+,FR1
        MOV         #16,R0
        FMUL        FR5,FR1
        FMOV.S      FR2,@(R0,R5)
        FMOV.S      @R4+,FR3
        MOV         #20,R0
        FMUL        FR5,FR3
        FMOV.S      FR1,@(R0,R5)
        FMOV.S      @R4+,FR2
        MOV         #24,R0
        FMUL        FR5,FR2
        FMOV.S      FR3,@(R0,R5)
        FMOV.S      @R4+,FR1
        MOV         #28,R0
        FMUL        FR6,FR1
        FMOV.S      FR2,@(R0,R5)
        FMOV.S      @R4+,FR3
        MOV         #32,R0
        FMUL        FR6,FR3
        FMOV.S      FR1,@(R0,R5)
        FMOV.S      @R4+,FR2
        MOV         #36,R0
        FMUL        FR6,FR2
        FMOV.S      FR3,@(R0,R5)
        FMOV.S      @R4,FR1
        MOV         #40,R0
        FMUL        FR6,FR1
        FMOV.S      FR2,@(R0,R5)
        RTS
        FMOV.S      FR1,@R4
}
#endif
/* clang-format on */
