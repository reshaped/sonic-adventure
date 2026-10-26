#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set m to a reflection in pl followed by m. pl's normal must be unit length,
 * and its z above 0, or 0 with y not 0, unless the normal is (0, 0, -1). a
 * null m selects the current matrix, in XMTRX
 *
 * the body moves pl's point to the origin, turns the normal onto z, about x by
 * asin(y / sqrt(y^2 + z^2)) and then about y by asin(x), scales z by -1, and
 * undoes the turn and the move. the turn about x zeroes y only where z is 0 or
 * more, since an arcsine's cosine is never negative, and with y and z both 0
 * its argument is 0 times FSRRA(0), an infinity. (0, 0, -1) needs no turn */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c628864 follows the
 * body's own return at 0x8c628846 */
#pragma inline_asm(njMirror)
void njMirror(NJS_MATRIX *m, NJS_PLANE *pl)
{
        .IMPORT     _asinf
        .IMPORT     _njRotateX
        .IMPORT     _njRotateY
        .IMPORT     _njScale
        .IMPORT     _njTranslate
        MOV.L       R11,@-R15
        MOV.L       R12,@-R15
        MOV.L       R13,@-R15
        MOV.L       R14,@-R15
        FMOV.S      FR12,@-R15
        MOV         R5,R11
        STS.L       PR,@-R15
        ADD         #12,R5
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FMOV        FR1,FR4
        FMUL        FR1,FR1
        FMUL        FR2,FR2
        FADD        FR2,FR1
        MOVA        ?to_angle,R0
        FSRRA       FR1
        FMOV.S      @R0,FR2
        FMUL        FR1,FR4
        MOV.L       #_asinf,R3
        FMOV        FR2,FR12
        JSR         @R3
        MOV         R4,R12
        FMUL        FR12,FR0
        MOV.L       #_asinf,R2
        FTRC        FR0,FPUL
        MOV         R11,R1
        STS         FPUL,R13
        ADD         #12,R1
        EXTU.W      R13,R13
        JSR         @R2
        FMOV.S      @R1,FR4
        FMUL        FR12,FR0
        FTRC        FR0,FPUL
        TST         R12,R12
        STS         FPUL,R14
        MOV         R11,R1
        EXTU.W      R14,R14
        FMOV.S      @R1+,FR4
        MOV.L       #_njTranslate,R3
        FMOV.S      @R1+,FR5
        FMOV.S      @R1+,FR6
        JSR         @R3
        MOV         R12,R4
        MOV.L       #_njRotateX,R2
        NEG         R13,R5
        JSR         @R2
        MOV         R12,R4
        MOV.L       #_njRotateY,R3
        MOV         R14,R5
        JSR         @R3
        MOV         R12,R4
        FLDI1       FR5
        FLDI1       FR4
        FMOV        FR5,FR6
        MOV.L       #_njScale,R3
        FNEG        FR6
        JSR         @R3
        MOV         R12,R4
        NEG         R14,R5
        MOV.L       #_njRotateY,R2
        JSR         @R2
        MOV         R12,R4
        MOV         R13,R5
        MOV.L       #_njRotateX,R3
        JSR         @R3
        MOV         R12,R4
        FMOV.S      @R11+,FR4
        FMOV.S      @R11+,FR5
        FNEG        FR4
        FMOV.S      @R11+,FR6
        FNEG        FR5
        MOV.L       #_njTranslate,R3
        FNEG        FR6
        JSR         @R3
        MOV         R12,R4
        LDS.L       @R15+,PR
        FMOV.S      @R15+,FR12
        MOV.L       @R15+,R14
        MOV.L       @R15+,R13
        MOV.L       @R15+,R12
        RTS
        MOV.L       @R15+,R11
?to_angle:
        .FDATA.S    F'10430.380859375
}
#endif
/* clang-format on */
