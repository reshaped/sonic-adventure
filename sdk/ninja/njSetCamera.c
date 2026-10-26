#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set the view to camera c. _nj_camera_matrix_ becomes the matrix that moves
 * c's position to the origin, turns c's direction onto +z, about y and then x,
 * and turns by -c->roll about z. the matrix stack is reset to its base, which
 * becomes _nj_camera_matrix_ followed by a scale by -1 in x and y, and is
 * copied to _nj_view_matrix_ and loaded into XMTRX. _nj_screen_.dist becomes
 * _nj_screen_.w / _nj_scr_aspect_x_ / (2 tan(c->ang / 2)). _nj_near_clip_ and
 * _nj_far_clip_ become -c->n_clip and -c->f_clip */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c668938 follows the
 * body's own return at 0x8c6688e2 */
#pragma inline_asm(njSetCamera)
void njSetCamera(NJS_CAMERA *c)
{
        .IMPORT     __nj_base_matrix_ptr_
        .IMPORT     __nj_camera_matrix_
        .IMPORT     __nj_current_matrix_ptr_
        .IMPORT     __nj_near_clip_
        .IMPORT     __nj_scr_aspect_x_
        .IMPORT     __nj_screen_
        .IMPORT     __nj_far_clip_
        .IMPORT     __nj_view_matrix_
        .IMPORT     __stack_size_
        .IMPORT     __unit_matrix_
        .IMPORT     _acosf
        .IMPORT     _njMultiMatrix
        .IMPORT     _njRotateX
        .IMPORT     _njRotateY
        .IMPORT     _njRotateZ
        .IMPORT     _njScale
        .IMPORT     _njTan
        .IMPORT     _njTranslate
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        FMOV.S      FR15,@-R15
        MOV         R4,R14
        FMOV.S      FR14,@-R15
        MOV.L       ?unit,R5
        FMOV.S      FR13,@-R15
        MOV.L       ?camera_matrix,R12
        FMOV.S      FR12,@-R15
        MOV         #24,R0
        STS.L       PR,@-R15
        MOV         R12,R4
        MOV         R5,R6
        ADD         #64,R4
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
        MOV.L       ?rotate_z,R3
        MOV.L       @(R0,R14),R2
        JSR         @R3
        NEG         R2,R5
        MOV         R14,R3
        ADD         #12,R3
        FMOV.S      @R3+,FR12
        FMOV.S      @R3+,FR13
        FMOV.S      FR12,FR15
        FMOV.S      @R3+,FR14
        FMUL        FR12,FR15
        FMOV.S      FR14,FR0
        FMAC        FR0,FR14,FR15
        FMOV.S      FR13,FR11
        FMOV.S      FR15,FR4
        FSQRT       FR4
        FMUL        FR13,FR11
        FMOV.S      FR4,FR0
        FMAC        FR0,FR4,FR11
        MOV.L       ?acosf,R3
        FSRRA       FR11
        JSR         @R3
        FMUL        FR11,FR4
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
        NEG         R1,R5
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
        NEG         R1,R5
        MOV.L       ?translate,R3
        MOV         #8,R0
        MOV         R12,R4
        FMOV.S      @(R0,R14),FR6
        MOV         #4,R0
        FNEG        FR6
        FMOV.S      @(R0,R14),FR5
        FMOV.S      @R14,FR4
        FNEG        FR5
        JSR         @R3
        FNEG        FR4
        MOV         #1,R1
        MOV.L       ?stack_size,R0
        MOV.L       ?base_matrix,R13
        MOV.L       ?unit,R5
        MOV.L       @R13,R13
        MOV.L       R1,@R0
        FSCHG
        FMOV        @R13+,XD0
        FMOV        @R13+,XD2
        FMOV        @R13+,XD4
        FMOV        @R13+,XD6
        FMOV        @R13+,XD8
        FMOV        @R13+,XD10
        FMOV        @R13+,XD12
        FMOV        @R13+,XD14
        ADD         #32,R5
        FMOV        @R5+,DR0
        FMOV        @R5+,DR2
        FMOV        @R5+,DR4
        FMOV        @R5+,DR6
        FMOV        DR6,@-R13
        ADD         #-64,R5
        FMOV        DR4,@-R13
        FMOV        DR2,@-R13
        FMOV        DR0,@-R13
        FMOV        @R5+,DR0
        FMOV        @R5+,DR2
        FMOV        @R5+,DR4
        FMOV        @R5+,DR6
        FMOV        DR6,@-R13
        FMOV        DR4,@-R13
        FMOV        DR2,@-R13
        FMOV        DR0,@-R13
        FSCHG
        MOV.L       ?current_matrix,R2
        MOV.L       R13,@R2
        FLDI1       FR4
        MOV.L       ?scale,R3
        FNEG        FR4
        FLDI1       FR6
        FMOV.S      FR4,FR5
        JSR         @R3
        MOV         R13,R4
        MOV.L       ?multi_matrix,R2
        MOV         R12,R5
        JSR         @R2
        MOV         R13,R4
        MOV.L       ?view_matrix,R2
        MOV         R13,R5
        ADD         #64,R2
        FSCHG
        FMOV        @R5+,XD0
        FMOV        @R5+,XD2
        FMOV        @R5+,XD4
        FMOV        @R5+,XD6
        FMOV        @R5+,XD8
        FMOV        @R5+,XD10
        FMOV        @R5+,XD12
        FMOV        @R5+,XD14
        FMOV        XD14,@-R2
        FMOV        XD12,@-R2
        FMOV        XD10,@-R2
        FMOV        XD8,@-R2
        FMOV        XD6,@-R2
        FMOV        XD4,@-R2
        FMOV        XD2,@-R2
        FMOV        XD0,@-R2
        FSCHG
        MOV.L       ?tan,R3
        MOV.L       @(28,R14),R4
        JSR         @R3
        SHAR        R4
        MOV.L       ?screen_width,R3
        FMOV.S      FR0,FR3
        FMOV.S      @R3,FR2
        FADD        FR3,FR0
        MOV.L       ?aspect_x,R12
        FMOV.S      @R12,FR12
        FDIV        FR12,FR2
        ADD         #-4,R3
        FDIV        FR0,FR2
        FMOV.S      FR2,@R3
        ADD         #32,R14
        MOV.L       ?near_clip,R1
        FMOV.S      @R14+,FR2
        FNEG        FR2
        FMOV.S      FR2,@R1
        FMOV.S      @R14,FR3
        MOV.L       ?far_clip,R1
        FNEG        FR3
        FMOV.S      FR3,@R1
        LDS.L       @R15+,PR
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR15
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        RTS
        MOV.L       @R15+,R14
        .DATA.W     H'0000
?unit:
        .DATA.L     __unit_matrix_
?camera_matrix:
        .DATA.L     __nj_camera_matrix_
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
?stack_size:
        .DATA.L     __stack_size_
?base_matrix:
        .DATA.L     __nj_base_matrix_ptr_
?current_matrix:
        .DATA.L     __nj_current_matrix_ptr_
?scale:
        .DATA.L     _njScale
?multi_matrix:
        .DATA.L     _njMultiMatrix
?view_matrix:
        .DATA.L     __nj_view_matrix_
?tan:
        .DATA.L     _njTan
?screen_width:
        .DATA.L     __nj_screen_+4
?aspect_x:
        .DATA.L     __nj_scr_aspect_x_
?near_clip:
        .DATA.L     __nj_near_clip_
?far_clip:
        .DATA.L     __nj_far_clip_
?radian_to_angle:
        .DATA.L     H'4622F986
}
#endif
/* clang-format on */
