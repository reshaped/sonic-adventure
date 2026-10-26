#include <ninja.h>

/* clang-format off */
#ifndef __clang__
/* @asm epilogue: fill, the body has no return of its own and runs on through
 * SHC's .ALIGN 4 fill, the NOP at 0x8c625c0e, into its RTS at 0x8c625c10, ahead
 * of asmsh's pool of the body's literals */
#pragma inline_asm(njSetLightMaterials)
void njSetLightMaterials(void)
{
        .IMPORT     __nj_light_control_
        .IMPORT     __nj_light_material_
        MOV.L       #__nj_light_material_,R4
        MOV.L       #__nj_light_queue_,R7
        MOVA        #H'40C00000,R0
        FLDI1       FR4
        FMOV.S      @R0,FR3
        MOV         #48,R1
        MOVA        #H'3F000000,R0
        ADD         R4,R1
        FMOV.S      @R0,FR5
        FLDI0       FR6
        MOV.W       #H'0080,R0
        FMOV.S      FR3,@R1
        FMOV.S      FR5,@-R1
        FMOV.S      FR5,@-R1
        FMOV.S      FR5,@-R1
        FMOV.S      FR4,@-R1
        FMOV.S      FR5,@-R1
        FMOV.S      FR5,@-R1
        FMOV.S      FR5,@-R1
        FMOV.S      FR4,@-R1
        FMOV.S      FR6,@-R1
        FMOV.S      FR6,@-R1
        FMOV.S      FR6,@-R1
        FMOV.S      FR4,@-R1
        MOV.L       @(R0,R7),R5
        MOV.L       #__nj_light_control_,R6
        TST         R5,R5
        BT/S        ?done
        MOV         R5,R4
        SHLL2       R4
        ADD         R4,R7
        MOV.W       #H'0088,R3
        MOV.W       #H'00C8,R2
?next:
        ADD         #-4,R7
        MOV.L       @R7,R4
        MOV.W       #H'0124,R0
        SHLL2       R4
        ADD         R6,R4
        MOV.L       @R4,R4
        MOV.L       @(R0,R4),R1
        MOV.W       #H'0074,R0
        CMP/EQ      R3,R1
        BT          ?rate
        CMP/EQ      R2,R1
        BF          ?step
?rate:
        FMOV.S      @(R0,R4),FR3
        ADD         #-4,R0
        FMOV.S      @(R0,R4),FR2
        ADD         #16,R0
        FSUB        FR3,FR2
        FMUL        FR2,FR2
        FSRRA       FR2
        FMOV.S      FR2,@(R0,R4)
?step:
        DT          R5
        BF          ?next
?done:
}

/* light_to_view_matrix, which the body reserves in bss, holds each queued
 * light's mtrx * _nj_view_matrix_ in turn */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c625ccc follows the
 * body's own return at 0x8c625ca0 */
#pragma inline_asm(njCalcLightMatrices)
void njCalcLightMatrices(void)
{
        .IMPORT     __nj_view_matrix_
        .IMPORT     _njCalcPoint
        .IMPORT     _njCalcVector
        .IMPORT     _njMemCopy4
        .IMPORT     _njMultiMatrix
        .SECTION    BSG32,DATA,ALIGN=32
light_to_view_matrix:
        .RES.B      64
        .SECTION    PSG,CODE,ALIGN=4
        MOV.L       #__nj_light_queue_,R2
        MOV.W       #H'0080,R0
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       @(R0,R2),R13
        MOV.L       R10,@-R15
        MOV.L       R8,@-R15
        MOV         R13,R14
        STS.L       PR,@-R15
        TST         R13,R13
        BT/S        ?done
        SHLL2       R14
        MOV.L       #__nj_view_matrix_,R11
        MOV.L       #__nj_light_control_,R10
        ADD         R2,R14
        MOV.L       #light_to_view_matrix,R8
?next:
        ADD         #-4,R14
        MOV         #16,R6
        MOV.L       @R14,R0
        MOV.L       #_njMemCopy4,R3
        MOV         R11,R5
        SHLL2       R0
        MOV.L       @(R0,R10),R12
        JSR         @R3
        MOV         R8,R4
        MOV.L       #_njMultiMatrix,R2
        MOV         R12,R5
        ADD         #0,R5
        JSR         @R2
        MOV         R8,R4
        MOV.W       #H'0090,R6
        MOV.L       #_njCalcPoint,R3
        MOV         R12,R5
        ADD         R12,R6
        ADD         #64,R5
        JSR         @R3
        MOV         R8,R4
        MOV.W       #H'009C,R6
        MOV.L       #_njCalcVector,R2
        MOV         R12,R5
        ADD         R12,R6
        ADD         #76,R5
        JSR         @R2
        MOV         R8,R4
        DT          R13
        BF          ?next
?done:
        LDS.L       @R15+,PR
        MOV.L       @R15+,R8
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        RTS
        MOV.L       @R15+,R14
}
#endif
/* clang-format on */

void njResetQueue(void)
{
    _nj_light_queue_.count = 0;
}

void njSetLightSystem(void)
{
}
