#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set m to R * m, where R rotates by ang about z. a null m selects the
 * current matrix, in XMTRX */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c629144 follows the
 * body's own return at 0x8c62913e */
#pragma inline_asm(njRotateZ)
void njRotateZ(NJS_MATRIX *m, Angle ang)
{
        TST         R4,R4
        BF/S        ?memory
        LDS         R5,FPUL
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
        RTS
        FSCHG
?memory:
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
        RTS
        FMOV.S      @R15+,FR12
}
#endif
/* clang-format on */
