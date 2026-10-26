#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* the determinant of m, or of the current matrix, in XMTRX, when m is null. it
 * expands along the first row, and each cofactor of that row along the second,
 * from the six 2 by 2 minors of the last two rows */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c668eec follows the
 * body's own returns at 0x8c668e38 and 0x8c668ee8 */
#pragma inline_asm(njDetMatrix)
Float njDetMatrix(NJS_MATRIX *m)
{
        TST         R4,R4
        BF          ?memory
        FSCHG
        FMOV        XD8,DR0
        FMOV        XD10,DR2
        FMOV        XD12,DR4
        FMOV        XD14,DR6
        FMOV        XD8,DR8
        FMOV        XD10,DR10
        FSCHG
        FMUL        FR7,FR10
        FMUL        FR6,FR3
        FSUB        FR3,FR10
        FMUL        FR6,FR9
        FMUL        FR5,FR2
        FSUB        FR2,FR9
        FMUL        FR4,FR11
        FMUL        FR7,FR0
        FSUB        FR0,FR11
        FMUL        FR5,FR8
        FMUL        FR4,FR1
        FSUB        FR1,FR8
        FSCHG
        FMOV        XD8,DR0
        FMOV        XD10,DR2
        FMOV        XD12,DR4
        FMOV        XD14,DR6
        FSCHG
        FMUL        FR3,FR5
        FMUL        FR0,FR6
        FMUL        FR1,FR7
        FMUL        FR2,FR4
        FMOV        FR5,FR0
        FMOV        FR6,FR1
        FMOV        FR7,FR2
        FMOV        FR4,FR3
        FSUB        FR2,FR5
        FSUB        FR3,FR6
        FSUB        FR0,FR7
        FSUB        FR1,FR4
        FSCHG
        FMOV        XD4,DR0
        FMOV        XD4,DR2
        FSCHG
        FMUL        FR2,FR7
        FMUL        FR10,FR2
        FMUL        FR9,FR0
        FMUL        FR3,FR10
        FMUL        FR11,FR1
        FMUL        FR4,FR3
        FADD        FR1,FR7
        FADD        FR0,FR3
        FSCHG
        FMOV        XD6,DR0
        FSCHG
        FMOV        FR0,FR1
        FMUL        FR1,FR11
        FMUL        FR0,FR5
        FMUL        FR8,FR1
        FADD        FR10,FR5
        FADD        FR11,FR2
        FADD        FR1,FR3
        FSCHG
        FMOV        XD6,DR0
        FSCHG
        FMOV        FR1,FR0
        FMUL        FR0,FR9
        FMUL        FR1,FR6
        FMUL        FR8,FR0
        FADD        FR9,FR5
        FADD        FR6,FR2
        FADD        FR7,FR0
        FSCHG
        FMOV        XD0,DR6
        FMOV        XD2,DR8
        FSCHG
        FMUL        FR6,FR5
        FMUL        FR7,FR2
        FMUL        FR8,FR0
        FMUL        FR9,FR3
        FSUB        FR2,FR5
        FSUB        FR3,FR0
        RTS
        FADD        FR5,FR0
?memory:
        MOV         R4,R1
        ADD         #32,R1
        FMOV.S      @R1+,FR0
        FMOV.S      @R1+,FR1
        FMOV.S      @R1+,FR2
        FMOV.S      @R1+,FR3
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1+,FR6
        FMOV.S      @R1+,FR7
        FMOV        FR0,FR8
        FMOV        FR1,FR9
        FMOV        FR2,FR10
        FMOV        FR3,FR11
        FMUL        FR7,FR10
        FMUL        FR6,FR3
        FSUB        FR3,FR10
        FMUL        FR6,FR9
        FMUL        FR5,FR2
        FSUB        FR2,FR9
        FMUL        FR4,FR11
        FMUL        FR7,FR0
        FSUB        FR0,FR11
        FMUL        FR5,FR8
        FMUL        FR4,FR1
        FSUB        FR1,FR8
        ADD         #-32,R1
        FMOV.S      @R1+,FR0
        FMOV.S      @R1+,FR1
        FMOV.S      @R1+,FR2
        FMOV.S      @R1+,FR3
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1+,FR6
        FMOV.S      @R1+,FR7
        FMUL        FR3,FR5
        FMUL        FR0,FR6
        FMUL        FR1,FR7
        FMUL        FR2,FR4
        FMOV        FR5,FR0
        FMOV        FR6,FR1
        FMOV        FR7,FR2
        FMOV        FR4,FR3
        FSUB        FR2,FR5
        FSUB        FR3,FR6
        FSUB        FR0,FR7
        FSUB        FR1,FR4
        ADD         #-48,R1
        FMOV.S      @R1+,FR0
        FMOV.S      @R1+,FR1
        FMOV        FR0,FR2
        FMOV        FR1,FR3
        FMUL        FR2,FR7
        FMUL        FR10,FR2
        FMUL        FR9,FR0
        FMUL        FR3,FR10
        FMUL        FR11,FR1
        FMUL        FR4,FR3
        FADD        FR1,FR7
        FADD        FR0,FR3
        FMOV.S      @R1+,FR0
        FMOV        FR0,FR1
        FMUL        FR1,FR11
        FMUL        FR0,FR5
        FMUL        FR8,FR1
        FADD        FR10,FR5
        FADD        FR11,FR2
        FADD        FR1,FR3
        FMOV.S      @R1,FR0
        FMOV        FR0,FR1
        FMUL        FR0,FR9
        FMUL        FR1,FR6
        FMUL        FR8,FR0
        FADD        FR9,FR5
        FADD        FR6,FR2
        FADD        FR7,FR0
        FMOV.S      @R4+,FR6
        FMOV.S      @R4+,FR7
        FMOV.S      @R4+,FR8
        FMOV.S      @R4+,FR9
        FMUL        FR6,FR5
        FMUL        FR7,FR2
        FMUL        FR8,FR0
        FMUL        FR9,FR3
        FSUB        FR2,FR5
        FSUB        FR3,FR0
        RTS
        FADD        FR5,FR0
}
#endif
/* clang-format on */
