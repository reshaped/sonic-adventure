#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set m to Rx * Ry * Rz * m, where Rx rotates by angx about x, Ry by angy
 * about y and Rz by angz about z. a null m selects the current matrix, in
 * XMTRX */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c628da4 follows the
 * body's own return at 0x8c628d9e */
#pragma inline_asm(njRotateXYZ)
void njRotateXYZ(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
        TST         R4,R4
        BF          ?memory
        TST         R7,R7
        BT/S        ?turn_y
        LDS         R7,FPUL
        FLDI0       FR3
        FSCA        FPUL,DR0
        FLDI0       FR6
        FLDI0       FR7
        FMOV.S      FR0,FR2
        FMOV.S      FR1,FR0
        FMOV.S      FR2,FR1
        FLDI0       FR2
        FMOV.S      FR0,FR5
        FMOV.S      FR1,FR4
        FNEG        FR4
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FSCHG
        FMOV        DR0,XD0
        FMOV        DR2,XD2
        FMOV        DR4,XD4
        FMOV        DR6,XD6
        FSCHG
?turn_y:
        TST         R6,R6
        BT/S        ?turn_x
        LDS         R6,FPUL
        FLDI0       FR3
        FSCA        FPUL,DR0
        FLDI0       FR5
        FLDI0       FR7
        FMOV.S      FR0,FR2
        FMOV.S      FR1,FR0
        FLDI0       FR1
        FMOV.S      FR0,FR6
        FMOV.S      FR2,FR4
        FNEG        FR2
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FSCHG
        FMOV        DR0,XD0
        FMOV        DR2,XD2
        FMOV        DR4,XD8
        FMOV        DR6,XD10
        FSCHG
?turn_x:
        TST         R5,R5
        BT/S        ?done
        LDS         R5,FPUL
        FLDI0       FR3
        FSCA        FPUL,DR0
        FLDI0       FR4
        FLDI0       FR7
        FMOV.S      FR0,FR2
        FLDI0       FR0
        FMOV.S      FR2,FR5
        FMOV.S      FR1,FR6
        FNEG        FR5
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FSCHG
        FMOV        DR0,XD4
        FMOV        DR2,XD6
        FMOV        DR4,XD8
        FMOV        DR6,XD10
        FSCHG
?done:
        RTS
        NOP
?memory:
        TST         R7,R7
        BT/S        ?memory_y
        LDS         R7,FPUL
        FLDI0       FR3
        FSCA        FPUL,DR0
        FLDI0       FR6
        FLDI0       FR7
        FMOV.S      FR0,FR2
        FMOV.S      FR1,FR0
        FMOV.S      FR2,FR1
        FLDI0       FR2
        FMOV.S      FR0,FR5
        FMOV.S      FR1,FR4
        FNEG        FR4
        FMOV.S      FR12,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR15,@-R15
        FSCHG
        FMOV        XD0,DR8
        FMOV        XD2,DR10
        FMOV        XD4,DR12
        FMOV        XD6,DR14
        FSCHG
        FRCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR3
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR5
        FMOV.S      @R4+,FR6
        FMOV.S      @R4+,FR7
        FRCHG
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FSCHG
        FMOV        DR8,XD0
        FMOV        DR10,XD2
        FMOV        DR12,XD4
        FMOV        DR14,XD6
        FSCHG
        FMOV.S      @R15+,FR15
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR12
?memory_y:
        TST         R6,R6
        BT/S        ?memory_x
        LDS         R6,FPUL
        FLDI0       FR3
        FSCA        FPUL,DR0
        FLDI0       FR5
        FLDI0       FR7
        FMOV.S      FR0,FR2
        FMOV.S      FR1,FR0
        FLDI0       FR1
        FMOV.S      FR0,FR6
        FMOV.S      FR2,FR4
        FNEG        FR2
        FMOV.S      FR12,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR15,@-R15
        FSCHG
        FMOV        XD0,DR8
        FMOV        XD2,DR10
        FMOV        XD8,DR12
        FMOV        XD10,DR14
        FSCHG
        FRCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR3
        ADD         #16,R4
        FMOV.S      @R4+,FR8
        FMOV.S      @R4+,FR9
        FMOV.S      @R4+,FR10
        FMOV.S      @R4+,FR11
        FRCHG
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        ADD         #-16,R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FSCHG
        FMOV        DR8,XD0
        FMOV        DR10,XD2
        FMOV        DR12,XD8
        FMOV        DR14,XD10
        FSCHG
        FMOV.S      @R15+,FR15
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR12
?memory_x:
        TST         R5,R5
        BT/S        ?memory_done
        LDS         R5,FPUL
        FLDI0       FR3
        FSCA        FPUL,DR0
        FLDI0       FR4
        FLDI0       FR7
        FMOV.S      FR0,FR2
        FLDI0       FR0
        FMOV.S      FR2,FR5
        FMOV.S      FR1,FR6
        FNEG        FR5
        FMOV.S      FR12,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR15,@-R15
        FSCHG
        FMOV        XD4,DR8
        FMOV        XD6,DR10
        FMOV        XD8,DR12
        FMOV        XD10,DR14
        FSCHG
        FRCHG
        ADD         #16,R4
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR5
        FMOV.S      @R4+,FR6
        FMOV.S      @R4+,FR7
        FMOV.S      @R4+,FR8
        FMOV.S      @R4+,FR9
        FMOV.S      @R4+,FR10
        FMOV.S      @R4+,FR11
        FRCHG
        FTRV        XMTRX,FV0
        FTRV        XMTRX,FV4
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FSCHG
        FMOV        DR8,XD4
        FMOV        DR10,XD6
        FMOV        DR12,XD8
        FMOV        DR14,XD10
        FSCHG
        FMOV.S      @R15+,FR15
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR12
?memory_done:
        RTS
        NOP
}
#endif
/* clang-format on */
