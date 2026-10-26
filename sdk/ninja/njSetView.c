#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set v->m from v's position, direction and roll, make v the current view,
 * and then call njClearMatrix. v->m starts as the unit matrix. njRotateZ
 * turns it by -roll, njRotateX by the pitch and njRotateY by the yaw, in that
 * order. njTranslate then moves it by -px, -py and -pz.
 *
 * h is sqrt(vx * vx + vz * vz), and d is the direction's length. the pitch is
 * acosf(h / d), negated where vy is negative. the yaw is acosf(vz / h),
 * negated unless vx is negative. each angle goes from radians to an Angle
 * truncated toward zero. 1 / d is FSRRA's estimate */
/* @asm operand: fschg-pairs, FMOV @R5+,DR8 at 0x8c65b3d4 and FMOV @R5+,DR10 at
 * 0x8c65b3d6 load DR8 and DR10 in the run FSCHG at 0x8c65b3ca opens */
#pragma inline_asm(njSetView)
void njSetView(NJS_VIEW *v)
{
        .IMPORT     __nj_view_
        .IMPORT     __unit_matrix_
        .IMPORT     _acosf
        .IMPORT     _njClearMatrix
        .IMPORT     _njRotateX
        .IMPORT     _njRotateY
        .IMPORT     _njRotateZ
        .IMPORT     _njTranslate
        MOV.L       R14,@-R15
        MOV.L       ?unit,R5
        MOV         R4,R14
        FMOV.S      FR15,@-R15
        MOV         #88,R0
        FMOV.S      FR14,@-R15
        MOV.L       @(R0,R14),R2
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        STS.L       PR,@-R15
        MOV         R5,R6
        ADD         #64,R4
        ADD         #32,R6
        FSCHG
        FMOV        @R6+,DR0
        FMOV        @R6+,DR2
        FMOV        @R6+,DR4
        FMOV        @R6+,DR6
        FMOV        @R5+,DR8
        FMOV        @R5+,DR10
        FSCHG
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FSCHG
        FMOV        @R5+,DR6
        FMOV        @R5+,DR4
        FSCHG
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR11,@-R4
        FMOV.S      FR10,@-R4
        FMOV.S      FR9,@-R4
        FMOV.S      FR8,@-R4
        MOV.L       ?rotate_z,R3
        NEG         R2,R5
        JSR         @R3
        MOV         R14,R4
        MOV         R14,R3
        ADD         #76,R3
        FMOV.S      @R3+,FR12
        FMOV.S      @R3+,FR13
        FMOV.S      FR12,FR15
        FMOV.S      @R3+,FR14
        FMUL        FR12,FR15
        FMOV.S      FR14,FR0
        FMOV.S      FR13,FR11
        FMAC        FR0,FR14,FR15
        FSQRT       FR15
        FMUL        FR13,FR11
        FMOV.S      FR15,FR0
        MOV.L       ?acosf,R3
        FMAC        FR0,FR15,FR11
        FSRRA       FR11
        FMUL        FR15,FR11
        JSR         @R3
        FMOV.S      FR11,FR4
        MOVA        ?radian_to_angle,R0
        FMOV.S      @R0,FR3
        MOV.L       ?angle_mask,R2
        FMUL        FR0,FR3
        FLDI0       FR2
        FTRC        FR3,FPUL
        FCMP/GT     FR13,FR2
        STS         FPUL,R1
        AND         R2,R1
        BT          ?pitch_sign
        NEG         R1,R1
?pitch_sign:
        MOV.L       ?rotate_x,R3
        NEG         R1,R5
        JSR         @R3
        MOV         R14,R4
        MOV.L       ?acosf,R3
        FMOV.S      FR14,FR4
        JSR         @R3
        FDIV        FR15,FR4
        MOVA        ?radian_to_angle,R0
        FMOV.S      @R0,FR3
        MOV.L       ?angle_mask,R2
        FMUL        FR0,FR3
        FLDI0       FR2
        FTRC        FR3,FPUL
        FCMP/GT     FR12,FR2
        STS         FPUL,R1
        AND         R2,R1
        BF          ?yaw_sign
        NEG         R1,R1
?yaw_sign:
        MOV.L       ?rotate_y,R3
        NEG         R1,R5
        JSR         @R3
        MOV         R14,R4
        MOV.L       ?translate,R3
        MOV         #72,R0
        FMOV.S      @(R0,R14),FR6
        MOV         #68,R0
        FMOV.S      @(R0,R14),FR5
        FNEG        FR6
        FNEG        FR5
        MOV         #64,R0
        FMOV.S      @(R0,R14),FR4
        FNEG        FR4
        JSR         @R3
        MOV         R14,R4
        MOV.L       ?current_view,R2
        MOV.L       R14,@R2
        MOV.L       ?clear_matrix,R3
        LDS.L       @R15+,PR
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR15
        JMP         @R3
        MOV.L       @R15+,R14
?unit:
        .DATA.L     __unit_matrix_
?rotate_z:
        .DATA.L     _njRotateZ
?acosf:
        .DATA.L     _acosf
?angle_mask:
        .DATA.L     H'0000FFFF
?rotate_x:
        .DATA.L     _njRotateX
?rotate_y:
        .DATA.L     _njRotateY
?translate:
        .DATA.L     _njTranslate
?current_view:
        .DATA.L     __nj_view_
?clear_matrix:
        .DATA.L     _njClearMatrix
?radian_to_angle:
        .DATA.L     H'4622F986
}
#endif
/* clang-format on */
