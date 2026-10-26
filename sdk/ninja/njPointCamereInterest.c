#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* aim c at the point x, y, z. c's view direction, vx to vz, becomes the
 * vector from its position toward the point, scaled to unit length by FSRRA's
 * approximate 1 / sqrt. _nj_camera_matrix_ becomes the unit matrix turned by
 * njRotateY with that direction's yaw, by njRotateX with its pitch, each from
 * acosf, and by njRotateZ with c->roll. c->lx and c->ly become (-1, 0, 0) and
 * (0, 1, 0) through that matrix, scaled to unit length by njUnitVector */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c668698 follows the
 * body's own return at 0x8c668660 */
#pragma inline_asm(njPointCameraInterest)
void njPointCameraInterest(NJS_CAMERA *c, Float x, Float y, Float z)
{
        .IMPORT     __nj_camera_matrix_
        .IMPORT     __unit_matrix_
        .IMPORT     _acosf
        .IMPORT     _njCalcVector
        .IMPORT     _njRotateX
        .IMPORT     _njRotateY
        .IMPORT     _njRotateZ
        .IMPORT     _njUnitVector
        MOV.L       R14,@-R15
        MOV.L       R12,@-R15
        MOV         R4,R14
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        STS.L       PR,@-R15
        ADD         #-24,R15
        MOV.L       ?matrix,R12
        FMOV.S      @R4+,FR0
        FMOV.S      FR4,FR12
        FMOV.S      @R4+,FR1
        FMOV.S      FR5,FR13
        FMOV.S      @R4+,FR2
        FMOV.S      FR6,FR14
        FLDI0       FR15
        FSUB        FR0,FR12
        FSUB        FR1,FR13
        FSUB        FR2,FR14
        FIPR        FV12,FV12
        MOV         R12,R4
        MOV.L       ?unit,R5
        FSRRA       FR15
        MOV         R5,R6
        ADD         #64,R4
        FMUL        FR15,FR12
        MOV.L       ?offset_vx,R0
        FMUL        FR15,FR13
        FMOV.S      FR12,@(R0,R14)
        FMUL        FR15,FR14
        MOV.L       ?offset_vy,R0
        FMOV.S      FR13,@(R0,R14)
        MOV.L       ?offset_vz,R0
        FMOV.S      FR14,@(R0,R14)
        ADD         #32,R6
        FSCHG
        FMOV        @R6+,DR0
        FMOV        @R6+,DR2
        FMOV        @R6+,DR4
        FMOV        @R6+,DR6
        FMOV        @R5+,DR8
        FMOV        DR6,@-R4
        FMOV        @R5+,DR10
        FMOV        DR4,@-R4
        FMOV        @R5+,DR6
        FMOV        DR2,@-R4
        FMOV        @R5+,DR4
        FMOV        DR0,@-R4
        FMOV        DR4,@-R4
        FMOV        DR6,@-R4
        FMOV        DR10,@-R4
        FMOV        DR8,@-R4
        FSCHG
        FMOV.S      FR12,FR15
        FMOV.S      FR14,FR0
        FMUL        FR12,FR15
        FMAC        FR0,FR14,FR15
        MOV.L       ?acosf,R3
        FMOV.S      FR15,FR4
        FSRRA       FR4
        JSR         @R3
        FMUL        FR14,FR4
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
        MOV         R12,R4
        JSR         @R3
        MOV         R1,R5
        FSQRT       FR15
        FMOV.S      FR13,FR4
        FMOV.S      FR15,FR0
        FMUL        FR13,FR4
        FMAC        FR0,FR15,FR4
        MOV.L       ?acosf,R3
        FSRRA       FR4
        JSR         @R3
        FMUL        FR15,FR4
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
        MOV         R12,R4
        JSR         @R3
        MOV         R1,R5
        MOV.L       ?rotate_z,R3
        MOV         R12,R4
        JSR         @R3
        MOV.L       @(24,R14),R5
        FLDI0       FR0
        ADD         #24,R15
        FLDI1       FR1
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR0,@-R15
        FMOV.S      FR0,@-R15
        FNEG        FR1
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        MOV.L       ?calc_vector,R3
        MOV         R14,R6
        MOV         R15,R5
        MOV         R12,R4
        JSR         @R3
        ADD         #40,R6
        MOV.L       ?unit_vector,R3
        MOV         R14,R4
        JSR         @R3
        ADD         #40,R4
        MOV.L       ?calc_vector,R3
        MOV         R14,R6
        MOV         R15,R5
        MOV         R12,R4
        ADD         #12,R5
        JSR         @R3
        ADD         #52,R6
        MOV.L       ?unit_vector,R3
        MOV         R14,R4
        JSR         @R3
        ADD         #52,R4
        ADD         #24,R15
        LDS.L       @R15+,PR
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR15
        MOV.L       @R15+,R12
        RTS
        MOV.L       @R15+,R14
?matrix:
        .DATA.L     __nj_camera_matrix_
?unit:
        .DATA.L     __unit_matrix_
?offset_vx:
        .DATA.L     12
?offset_vy:
        .DATA.L     16
?offset_vz:
        .DATA.L     20
?acosf:
        .DATA.L     _acosf
?angle_mask:
        .DATA.L     H'0000FFFF
?rotate_y:
        .DATA.L     _njRotateY
?rotate_x:
        .DATA.L     _njRotateX
?rotate_z:
        .DATA.L     _njRotateZ
?calc_vector:
        .DATA.L     _njCalcVector
?unit_vector:
        .DATA.L     _njUnitVector
?radian_to_angle:
        .DATA.L     H'4622F986
}
#endif
/* clang-format on */
