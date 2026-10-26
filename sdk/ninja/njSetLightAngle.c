#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set the inner and outer angles of l's spotlight cone, attr.iang and
 * attr.oang, to iang and oang. ltcal.cosi and ltcal.cose take the squares of
 * their cosines. ltcal.reserve takes the two angles' low 16 bits, iang's in
 * its high half and oang's in its low half. ltcal.rate keeps its value until
 * njSetLightMaterials or njCalcLightMaterials derives it again */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c627bc4 follows the
 * body's own return at 0x8c627bb0 */
#pragma inline_asm(njSetLightAngle)
void njSetLightAngle(NJS_LIGHT *l, Angle iang, Angle oang)
{
        MOV.W       #H'0140,R0
        MOV.L       R5,@(R0,R4)
        MOV.W       #H'0144,R0
        MOV.L       R6,@(R0,R4)
        LDS         R5,FPUL
        FSCA        FPUL,DR0
        MOV.W       #H'0070,R0
        LDS         R6,FPUL
        FSCA        FPUL,DR2
        FMUL        FR1,FR1
        FMUL        FR3,FR3
        FMOV.S      FR1,@(R0,R4)
        MOV.W       #H'0074,R0
        FMOV.S      FR3,@(R0,R4)
        MOV.W       #H'0140,R0
        MOV.L       @(R0,R4),R3
        MOV.L       #H'00010000,R2
        MOV.W       #H'0144,R0
        ADD         R2,R3
        MOV.L       @(R0,R4),R5
        EXTU.W      R3,R3
        ADD         R2,R5
        SHLL16      R3
        EXTU.W      R5,R5
        MOV.W       #H'008C,R0
        OR          R5,R3
        RTS
        MOV.L       R3,@(R0,R4)
}
#endif
/* clang-format on */
