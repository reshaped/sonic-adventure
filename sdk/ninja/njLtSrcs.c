/* light a point by the queued lights. njSetQueue queues the lights that are on
 * and picks a calculator for each by its type. of the ltcal fields the
 * calculators read, njCalcLightMaterials sets mamb, mdif, mspc, exp and rate,
 * njCheckLightMaterials can set mamb from the material's diffuse instead, and
 * njGetLightDistance sets lmvctr, ipd and ratten for a point.
 * njCalculateLightsRGB and its three variants run the calculators in queue
 * order */
#include <ninja.h>
#include <sg_xpt.h>

/* a calculator: add light's share at point, with normal, to color. color[0]
 * takes the diffuse and the specular's alpha, and color[1] the specular's r, g
 * and b */
typedef void lt_calc(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                     NJS_VECTOR *normal);

/* the color, point and normal that lt_rgb_lights passes to the calculators.
 * njCalculateLightsRGB and its three variants set them. a call with reverse
 * set points _nj_ptr_rgb_lt_nml_ into its own frame, and a call to a variant
 * points _nj_ptr_rgb_lt_argb_ there, so each dangles once the call returns */
NJS_ARGB *_nj_ptr_rgb_lt_argb_;
NJS_POINT3 *_nj_ptr_rgb_lt_pnt_;
NJS_VECTOR *_nj_ptr_rgb_lt_nml_;
/* the queued lights, in queue order, and the calculator njSetLightQueue picks
 * for each */
NJS_LIGHT *_nj_ptr_rgb_lt_[NJD_LIGHT_MAX];
lt_calc *_nj_rgb_lt_func_[NJD_LIGHT_MAX] = {0};

/* clang skips the inline bodies below, so it sees these declared but never
 * defined */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static Float lt_power_body(Float value, Int step);
static lt_calc lt_rgb_01, lt_rgb_02, lt_rgb_82, lt_rgb_04, lt_rgb_c4, lt_rgb_08,
    lt_rgb_88, lt_rgb_c8, lt_rgb_40, lt_rgb_10, lt_rgb_90, lt_rgb_20, lt_rgb_a0,
    lt_rgb_e0, lt_rgb_03, lt_rgb_83, lt_rgb_05, lt_rgb_c5, lt_rgb_13, lt_rgb_93,
    lt_rgb_25, lt_rgb_a5, lt_rgb_e5;
static void lt_rgb_lights_body(void);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

/* clang-format off */
#ifndef __clang__
/* lt_power's body. SHC gives lt_power, a static function, a numbered label,
 * so the body names its own entry, _lt_power, for the calculators' JSRs */
/* @asm open: no inspect rule reads its table switch on R4, which ends in BRAF
 * R0 */
#pragma inline_asm(lt_power_body)
static Float lt_power_body(Float value, Int step)
{
_lt_power:
        MOV         #17,R1
        CMP/HS      R1,R4
        BT          ?P16
        SHLL        R4
        MOVA        ?STEPS,R0
        MOV.W       @(R0,R4),R0
        BRAF        R0
        NOP
?STEPS:
        .DATA.W     ?P0-?STEPS
        .DATA.W     ?P1-?STEPS
        .DATA.W     ?P2-?STEPS
        .DATA.W     ?P3-?STEPS
        .DATA.W     ?P4-?STEPS
        .DATA.W     ?P5-?STEPS
        .DATA.W     ?P6-?STEPS
        .DATA.W     ?P7-?STEPS
        .DATA.W     ?P8-?STEPS
        .DATA.W     ?P9-?STEPS
        .DATA.W     ?P10-?STEPS
        .DATA.W     ?P11-?STEPS
        .DATA.W     ?P12-?STEPS
        .DATA.W     ?P13-?STEPS
        .DATA.W     ?P14-?STEPS
        .DATA.W     ?P15-?STEPS
        .DATA.W     ?P16-?STEPS
?P0:
        RTS
        FLDI1       FR0
?P1:
        FMOV.S      FR4,FR0
        RTS
        FSQRT       FR0
?P3:
        FMOV.S      FR4,FR0
        FSQRT       FR4
        RTS
        FMUL        FR4,FR0
?P15:
        FMUL        FR4,FR4
?P13:
        FMUL        FR4,FR4
?P11:
        FMUL        FR4,FR4
?P9:
        FMUL        FR4,FR4
?P7:
        FMUL        FR4,FR4
?P5:
        FMOV.S      FR4,FR0
        FMUL        FR4,FR0
        RTS
        FMUL        FR4,FR0
?P16:
        FMUL        FR4,FR4
?P14:
        FMUL        FR4,FR4
?P12:
        FMUL        FR4,FR4
?P10:
        FMUL        FR4,FR4
?P8:
        FMUL        FR4,FR4
?P6:
        FMUL        FR4,FR4
?P4:
        FMUL        FR4,FR4
?P2:
        FMOV.S      FR4,FR0
        RTS
        NOP
}
#endif
/* clang-format on */

/* 1 for step 0. for steps 1 to 16, value to the power 0.5, 1, 1.5, 2, 3, 4,
 * 6, 8, 12, 16, 24, 32, 48, 64, 96 and 128, and for any other step, to the
 * power 128. it writes no general or floating-point register but R0, R1, R4,
 * FR0 and FR4, and the calculators that call it keep values in the others
 * across the call */
static Float lt_power(Float value, Int step)
{
    return lt_power_body(value, step);
}

/* clang-format off */
#ifndef __clang__
/* fpow_tbl[row][(Int)(value * 256)], or 0 where that index passes 255. row
 * must be from 0 to 16, and value above -1 / 256: a lower value indexes
 * before the row */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c625d80 follows the
 * body's own return at 0x8c625d7a */
#pragma inline_asm(njLightPower)
Float njLightPower(Float value, Int row)
{
        .IMPORT     _fpow_tbl
        MOVA        ?scale,R0
        MOV.W       ?rows,R3
        FMOV.S      @R0,FR3
        MOV.L       ?table,R1
        FMUL        FR3,FR4
        FTRC        FR4,FPUL
        STS         FPUL,R2
        CMP/GE      R3,R2
        BT          ?past
        SHLL8       R4
        ADD         R2,R4
        SHLL2       R4
        ADD         R4,R1
        BRA         ?done
        FMOV.S      @R1,FR0
?rows:
        .DATA.W     H'0100
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
?past:
        FLDI0       FR0
?done:
        RTS
        NOP
}
#endif
/* clang-format on */

Float njLightSqrt(Float value)
{
    return _builtin_sqrtf(value);
}

/* the calculators, by the light types they serve. lt_rgb_lights calls each
 * with light, color, point and normal in R4 to R7. it loads R5 to R7 once,
 * before its first call, so each calculator must leave them as it found them.
 *
 * the formulas below use these terms:
 *
 *   cose         light->ltcal.cose, and so for any bare field name
 *   n            normal
 *   d            lpnt - point
 *   u . v        the dot product of u and v
 *   1 / |d|      FSRRA(d . d), the SH-4's approximate reciprocal square root
 *   diffuse k    add k times mdif's r, g and b to color[0]'s
 *   specular s   add s times mspc's r, g and b to color[1]'s, and s times
 *                mspc.a to color[0].a
 *
 * njGetLightDistance sets ratten, ipd and lmvctr from its own point, not from
 * the point the calculator is given */

/* type 0x00: add mdif's a, r, g and b to the diffuse, through
 * _nj_ptr_rgb_lt_argb_ rather than color */
/* @bug memory: the calculators after a type 0x00 light in the queue add
 * their shares to _nj_ptr_rgb_lt_argb_ and the pointers after it in memory, as
 * Floats. lt_rgb_lights loads color into R5 once, at 0x8c626e58, for every
 * call, and this body loads _nj_ptr_rgb_lt_argb_'s address into R5, at
 * 0x8c625d8a, and leaves it there */
static void lt_rgb_00(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
    _nj_ptr_rgb_lt_argb_->a += light->ltcal.mdif.a;
    _nj_ptr_rgb_lt_argb_->r += light->ltcal.mdif.r;
    _nj_ptr_rgb_lt_argb_->g += light->ltcal.mdif.g;
    _nj_ptr_rgb_lt_argb_->b += light->ltcal.mdif.b;
}

/* clang-format off */
#ifndef __clang__
/* type 0x01, NJD_AMBIENT: add mamb's r, g and b to color[0]'s */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c625e00 follows the
 * body's own return at 0x8c625df8 */
#pragma inline_asm(lt_rgb_01)
static void lt_rgb_01(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        RTS
        FMOV.S      FR4,@(R0,R5)
}

/* type 0x02, NJD_DIR_LIGHT: add diffuse -(n . lvctr) where it is positive */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c625e1c */
#pragma inline_asm(lt_rgb_02)
static void lt_rgb_02(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'009C,R2
        MOV         R7,R1
        FLDI0       FR7
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        ADD         R4,R2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FIPR        FV4,FV8
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      @R2,FR3
        FNEG        FR11
        FCMP/GT     FR7,FR11
        BF          ?done
        MOV         R3,R2
        FMOV.S      FR11,FR0
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0x82, and any type njSetLightQueue has no case for: add diffuse
 * (1 - n . lvctr) / 2 */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c625e70 */
#pragma inline_asm(lt_rgb_82)
static void lt_rgb_82(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'009C,R2
        MOV         R7,R1
        FLDI0       FR7
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        ADD         R4,R2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FIPR        FV4,FV8
        MOV         R5,R3
        ADD         R0,R3
        MOVA        ?half,R0
        ADD         R4,R2
        FLDI1       FR0
        FMOV.S      @R0,FR8
        FSUB        FR11,FR0
        MOV         R3,R1
        FMUL        FR8,FR0
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         #4,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        RTS
        FMOV.S      FR6,@R1
        .DATA.W     H'0000
?half:
        .FDATA.S    F'0.5
}

/* types 0x04, NJD_POINT_LIGHT, and 0x84. add nothing unless d . d < frr and
 * n . d > 0. then add diffuse (n . d) / |d|, times nrr / (d . d) where
 * d . d > nrr */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c625ed4 */
#pragma inline_asm(lt_rgb_04)
static void lt_rgb_04(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'0090,R2
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       #H'0068,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR10
        FIPR        FV4,FV0
        MOV         #0,R0
        FCMP/GT     FR11,FR10
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        CMP/EQ      #3,R0
        BF          ?done
        FMOV.S      FR11,FR9
        FSRRA       FR11
        FMOV.S      FR3,FR0
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FCMP/GT     FR8,FR9
        BF          ?diffuse
        FDIV        FR9,FR0
        FMUL        FR8,FR0
?diffuse:
        MOV         R5,R3
        ADD         R4,R2
        ADD         R0,R3
        FMUL        FR11,FR0
        MOV         R3,R1
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0xc4. add nothing unless ratten is not 0 and n . lmvctr > 0. then add
 * diffuse ratten * (n . lmvctr) */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c625f5a */
#pragma inline_asm(lt_rgb_c4)
static void lt_rgb_c4(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'0060,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R3
        MOV.W       #H'00A8,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        ADD         R4,R2
        FMOV.S      @R3,FR3
        FMOV.S      @R1,FR6
        FCMP/EQ     FR7,FR3
        BT          ?done
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FIPR        FV4,FV8
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      FR3,FR10
        FMOV.S      @R2,FR3
        FCMP/GT     FR7,FR11
        BF          ?done
        FMUL        FR10,FR11
        MOV         R3,R2
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      FR11,FR0
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0x08, NJD_SPOT_LIGHT. k is (n . d)^2 / (d . d), times nrr / (d . d)
 * where d . d > nrr, and c is (d . lvctr)^2 / (d . d). add nothing unless
 * n . d > 0, d . d < frr, d . lvctr < 0 and c > cose. then, where c < cosi,
 * add diffuse k * rate * lt_power(c, exp). where c is cosi or more, add
 * diffuse k and specular k */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c625fbe */
#pragma inline_asm(lt_rgb_08)
static void lt_rgb_08(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'0090,R2
        MOV.W       #H'009C,R0
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       #H'0068,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FIPR        FV4,FV0
        ADD         R4,R0
        FLDI1       FR9
        FDIV        FR11,FR9
        MOV.W       #H'0070,R3
        FMOV.S      @R0+,FR0
        FMOV.S      @R0+,FR1
        FMOV.S      @R0,FR2
        FMOV.S      FR3,FR10
        ADD         R4,R3
        FIPR        FV4,FV0
        FMOV.S      FR10,FR0
        FMUL        FR10,FR0
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR2
        FMOV.S      @R3+,FR5
        FMOV.S      @R3,FR6
        FMOV.S      FR3,FR4
        FMUL        FR3,FR4
        FMUL        FR9,FR0
        FNEG        FR3
        FMUL        FR9,FR4
        MOV         #0,R0
        FCMP/GT     FR7,FR10
        ROTCL       R0
        FCMP/GT     FR11,FR2
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        FCMP/GT     FR6,FR4
        ROTCL       R0
        CMP/EQ      #15,R0
        BF          ?done
        FCMP/GT     FR8,FR11
        BF          ?cone
        FMUL        FR8,FR0
        FMUL        FR9,FR0
?cone:
        FCMP/GT     FR4,FR5
        BF          ?inner
        MOV.L       #_lt_power,R3
        MOV.W       #H'0080,R0
        STS.L       PR,@-R15
        FMOV.S      @(R0,R4),FR2
        FMUL        FR2,FR0
        MOV.W       #H'0088,R0
        MOV.L       R4,@-R15
        FMOV.S      FR0,@-R15
        JSR         @R3
        MOV.L       @(R0,R4),R4
        FMOV.S      @R15+,FR1
        MOV.L       @R15+,R4
        FMUL        FR1,FR0
        LDS.L       @R15+,PR
        MOV.W       #H'0108,R2
        BRA         ?diffuse
        MOV         #4,R0
?inner:
        MOV         #4,R0
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        MOV.W       #H'0108,R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?diffuse:
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0x88: as type 0x08, but add diffuse k * rate * (c - cose) where
 * c < cosi */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c6260d6 */
#pragma inline_asm(lt_rgb_88)
static void lt_rgb_88(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'0090,R2
        MOV.W       #H'009C,R0
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       #H'0068,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FIPR        FV4,FV0
        ADD         R4,R0
        FLDI1       FR9
        FDIV        FR11,FR9
        MOV.W       #H'0070,R3
        FMOV.S      @R0+,FR0
        FMOV.S      @R0+,FR1
        FMOV.S      @R0,FR2
        FMOV.S      FR3,FR10
        ADD         R4,R3
        FIPR        FV4,FV0
        FMOV.S      FR10,FR0
        FMUL        FR10,FR0
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR2
        FMOV.S      @R3+,FR5
        FMOV.S      @R3,FR6
        FMOV.S      FR3,FR4
        FMUL        FR3,FR4
        FMUL        FR9,FR0
        FNEG        FR3
        FMUL        FR9,FR4
        MOV         #0,R0
        FCMP/GT     FR7,FR10
        ROTCL       R0
        FCMP/GT     FR11,FR2
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        FCMP/GT     FR6,FR4
        ROTCL       R0
        CMP/EQ      #15,R0
        BF          ?done
        FCMP/GT     FR8,FR11
        BF          ?cone
        FMUL        FR8,FR0
        FMUL        FR9,FR0
?cone:
        FCMP/GT     FR4,FR5
        BF          ?inner
        MOV.W       #H'0080,R0
        FSUB        FR6,FR4
        FMOV.S      @(R0,R4),FR2
        FMUL        FR2,FR0
        FMUL        FR4,FR0
        MOV.W       #H'0108,R2
        BRA         ?diffuse
        MOV         #4,R0
?inner:
        MOV         #4,R0
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        MOV.W       #H'0108,R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?diffuse:
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0xc8. k is ratten * (n . lmvctr)^2. add nothing unless ratten is not 0
 * and n . lmvctr > 0. then add diffuse k, and specular k where ipd > cosi */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c6261d0 */
#pragma inline_asm(lt_rgb_c8)
static void lt_rgb_c8(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'0060,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R3
        MOV.W       #H'00A8,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        ADD         R4,R2
        FMOV.S      @R3+,FR0
        FMOV.S      @R1,FR6
        FCMP/EQ     FR7,FR0
        BT          ?done
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0070,R1
        FIPR        FV4,FV8
        MOV         #4,R0
        ADD         R4,R1
        FMOV.S      @R3,FR10
        FMOV.S      @R1,FR9
        FMUL        FR11,FR0
        FMUL        FR11,FR0
        FCMP/GT     FR7,FR11
        BF          ?done
        FCMP/GT     FR9,FR10
        BF          ?diffuse
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?diffuse:
        MOV.W       #H'0108,R2
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0x40, NJD_USER_LIGHT: call light's attr.func with color, point, normal
 * and light. the body passes them in R4 to R7, so it overwrites R5 to R7
 * itself, and the call may change them too: it saves them first and restores
 * them for lt_rgb_lights */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c626274 follows the
 * body's own return at 0x8c62626e */
#pragma inline_asm(lt_rgb_40)
static void lt_rgb_40(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.L       R7,@-R15
        MOV.L       R6,@-R15
        MOV.L       R5,@-R15
        STS.L       PR,@-R15
        MOV.W       #H'013C,R0
        MOV         R4,R3
        MOV.L       @(R0,R3),R2
        MOV         R5,R4
        MOV         R6,R5
        MOV         R7,R6
        JSR         @R2
        MOV         R3,R7
        LDS.L       @R15+,PR
        MOV.L       @R15+,R5
        MOV.L       @R15+,R6
        RTS
        MOV.L       @R15+,R7
}

/* type 0x10, NJD_SPEC_DIR. k is -(n . lvctr). add nothing unless k > 0. then
 * add diffuse k. where k^2 > cose, also add specular
 * rate * lt_power(k^2, exp) where k^2 < cosi, and specular 1 where k^2 is cosi
 * or more */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626292 */
#pragma inline_asm(lt_rgb_10)
static void lt_rgb_10(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'009C,R2
        MOV.W       #H'0070,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        FMOV.S      @R1,FR10
        ADD         R4,R3
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2,FR6
        MOV.W       #H'0108,R2
        FIPR        FV4,FV8
        MOV         #4,R0
        ADD         R4,R2
        MOV         R5,R1
        ADD         R0,R1
        FMOV.S      FR11,FR0
        FCMP/GT     FR11,FR7
        BF          ?done
        FMUL        FR0,FR11
        FMOV.S      @R3+,FR8
        FNEG        FR0
        FMOV.S      @R3,FR9
        FMOV.S      @R2+,FR1
        MOV         R1,R3
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R1,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR11
        BF          ?done
        FLDI1       FR0
        FCMP/GT     FR11,FR8
        BF          ?specular
        MOV.L       #_lt_power,R3
        MOV.W       #H'0080,R0
        STS.L       PR,@-R15
        FMOV.S      @(R0,R4),FR1
        MOV.W       #H'0088,R0
        MOV.L       R4,@-R15
        MOV.L       @(R0,R4),R4
        JSR         @R3
        FMOV.S      FR11,FR4
        MOV.L       @R15+,R4
        FMUL        FR1,FR0
        LDS.L       @R15+,PR
        MOV         #4,R0
?specular:
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
}

/* type 0x90: as type 0x10, but with fpow_tbl[exp][(Int)(k^2 * 256)] in place
 * of lt_power(k^2, exp), and specular 1 where that index passes 255 */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c62634e */
#pragma inline_asm(lt_rgb_90)
static void lt_rgb_90(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       ?lvctr,R2
        MOV.W       ?cosi,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        FMOV.S      @R1,FR10
        ADD         R4,R3
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2,FR6
        MOV.W       ?mdif_r,R2
        FIPR        FV4,FV8
        MOV         #4,R0
        ADD         R4,R2
        MOV         R5,R1
        ADD         R0,R1
        FMOV.S      FR11,FR0
        FCMP/GT     FR11,FR7
        BF          ?done
        FMUL        FR0,FR11
        FMOV.S      @R3+,FR8
        FNEG        FR0
        FMOV.S      @R3,FR9
        FMOV.S      @R2+,FR1
        MOV         R1,R3
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R1,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR11
        BF          ?done
        FLDI1       FR0
        FCMP/GT     FR11,FR8
        BF          ?specular
        MOVA        ?scale,R0
        FMOV.S      @R0,FR3
        MOV.W       ?rate,R0
        FMUL        FR3,FR11
        FMOV.S      @(R0,R4),FR1
        MOV.W       ?exp,R0
        MOV.L       @(R0,R4),R3
        MOV.L       ?table,R1
        MOV.W       ?rows,R0
        FTRC        FR11,FPUL
        STS         FPUL,R2
        CMP/GE      R0,R2
        BT          ?specular
        SHLL8       R3
        ADD         R2,R3
        SHLL2       R3
        ADD         R3,R1
        FMOV.S      @R1,FR0
        FMUL        FR1,FR0
?specular:
        MOV         #20,R1
        MOV.W       ?mspc,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         #4,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
?lvctr:
        .DATA.W     H'009C
?cosi:
        .DATA.W     H'0070
?mdif_r:
        .DATA.W     H'0108
?rate:
        .DATA.W     H'0080
?exp:
        .DATA.W     H'0088
?rows:
        .DATA.W     H'0100
?mspc:
        .DATA.W     H'0114
        .DATA.W     H'0000
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
}

/* type 0x20, NJD_SPEC_POINT. k is (n . d) / |d|, and a is nrr * (1 / |d|)^2
 * where d . d > nrr, else 1. add nothing unless d . d < frr and n . d > 0.
 * then add diffuse k. where k^2 > cose, also add specular
 * a * rate * lt_power(k^2, exp) where k^2 < cosi, and specular a where k^2 is
 * cosi or more */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626428 */
#pragma inline_asm(lt_rgb_20)
static void lt_rgb_20(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'0090,R2
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       #H'0068,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR10
        FIPR        FV4,FV0
        MOV         #0,R0
        FCMP/GT     FR11,FR10
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        CMP/EQ      #3,R0
        BF          ?done
        FMOV.S      FR11,FR9
        FSRRA       FR11
        MOV.W       #H'0070,R3
        FMOV.S      FR3,FR0
        ADD         R4,R3
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FLDI1       FR10
        FCMP/GT     FR8,FR9
        BF          ?diffuse
        FMOV.S      FR11,FR10
        FMUL        FR11,FR10
        FMUL        FR8,FR10
?diffuse:
        FMOV.S      @R3+,FR8
        FMOV.S      @R3,FR9
        MOV         R5,R3
        ADD         R4,R2
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMUL        FR11,FR0
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        FMOV.S      FR0,FR7
        MOV         R3,R2
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMUL        FR7,FR7
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR10,FR0
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR7
        BF          ?done
        FCMP/GT     FR7,FR8
        BF          ?specular
        MOV.W       #H'0080,R0
        MOV.L       #_lt_power,R3
        STS.L       PR,@-R15
        FMOV.S      @(R0,R4),FR1
        FMUL        FR0,FR1
        MOV.W       #H'0088,R0
        MOV.L       R4,@-R15
        MOV.L       @(R0,R4),R4
        JSR         @R3
        FMOV.S      FR7,FR4
        MOV.L       @R15+,R4
        FMUL        FR1,FR0
        LDS.L       @R15+,PR
?specular:
        MOV         #4,R0
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
}

/* type 0xa0: as type 0x20, but with fpow_tbl[exp][(Int)(k^2 * 256)] in place
 * of lt_power(k^2, exp), and specular a where that index passes 255 */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626524 */
#pragma inline_asm(lt_rgb_a0)
static void lt_rgb_a0(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       ?lpnt,R2
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       ?nrr,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR10
        FIPR        FV4,FV0
        MOV         #0,R0
        FCMP/GT     FR11,FR10
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        CMP/EQ      #3,R0
        BF          ?done
        FMOV.S      FR11,FR9
        FSRRA       FR11
        MOV.W       ?cosi,R3
        FMOV.S      FR3,FR0
        ADD         R4,R3
        MOV.W       ?mdif_r,R2
        MOV         #4,R0
        FLDI1       FR10
        FCMP/GT     FR8,FR9
        BF          ?diffuse
        FMOV.S      FR11,FR10
        FMUL        FR11,FR10
        FMUL        FR8,FR10
?diffuse:
        FMOV.S      @R3+,FR8
        FMOV.S      @R3,FR9
        MOV         R5,R3
        ADD         R4,R2
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMUL        FR11,FR0
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        FMOV.S      FR0,FR7
        MOV         R3,R2
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMUL        FR7,FR7
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR10,FR0
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR7
        BF          ?done
        FCMP/GT     FR7,FR8
        BF          ?specular
        MOVA        ?scale,R0
        FMOV.S      @R0,FR3
        MOV.W       ?rate,R0
        FMUL        FR3,FR7
        FMOV.S      @(R0,R4),FR1
        MOV.W       ?exp,R0
        MOV.L       @(R0,R4),R3
        FMUL        FR0,FR1
        MOV.L       ?table,R1
        MOV.W       ?rows,R0
        FTRC        FR7,FPUL
        STS         FPUL,R2
        CMP/GE      R0,R2
        BT          ?specular
        SHLL8       R3
        ADD         R2,R3
        SHLL2       R3
        ADD         R3,R1
        FMOV.S      @R1,FR0
        FMUL        FR1,FR0
?specular:
        MOV         #4,R0
        MOV         #20,R1
        MOV.W       ?mspc,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
?lpnt:
        .DATA.W     H'0090
?nrr:
        .DATA.W     H'0068
?cosi:
        .DATA.W     H'0070
?mdif_r:
        .DATA.W     H'0108
?rate:
        .DATA.W     H'0080
?exp:
        .DATA.W     H'0088
?rows:
        .DATA.W     H'0100
?mspc:
        .DATA.W     H'0114
        .DATA.W     H'0000
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
}

/* type 0xe0. k is n . lmvctr. add nothing unless ratten is not 0 and k > 0.
 * then add diffuse k. where k^2 > cose, also add specular
 * ratten * rate * fpow_tbl[exp][(Int)(k^2 * 256)] where k^2 < cosi and that
 * index is below 256, and specular ratten where k^2 is cosi or more or the
 * index passes 255 */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626630 */
#pragma inline_asm(lt_rgb_e0)
static void lt_rgb_e0(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       ?ratten,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R3
        MOV.W       ?lmvctr,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        ADD         R4,R2
        FMOV.S      @R3+,FR0
        FMOV.S      @R1,FR6
        FCMP/EQ     FR7,FR0
        BT          ?done
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       ?cosi,R3
        FIPR        FV4,FV8
        MOV         #4,R0
        MOV.W       ?mdif_r,R2
        ADD         R4,R3
        ADD         R4,R2
        MOV         R5,R1
        ADD         R0,R1
        FMOV.S      FR0,FR10
        FMOV.S      FR11,FR0
        FCMP/GT     FR7,FR11
        BF          ?done
        FMUL        FR0,FR11
        FMOV.S      @R3+,FR8
        FMOV.S      @R3,FR9
        FMOV.S      @R2+,FR1
        MOV         R1,R3
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R1,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR11
        BF          ?done
        FMOV.S      FR10,FR0
        FCMP/GT     FR11,FR8
        BF          ?specular
        MOVA        ?scale,R0
        FMOV.S      @R0,FR3
        MOV.W       ?rate,R0
        FMUL        FR3,FR11
        FMOV.S      @(R0,R4),FR1
        MOV.W       ?exp,R0
        MOV.L       @(R0,R4),R3
        FMUL        FR0,FR1
        MOV.L       ?table,R1
        MOV.W       ?rows,R0
        FTRC        FR11,FPUL
        STS         FPUL,R2
        CMP/GE      R0,R2
        BT          ?specular
        SHLL8       R3
        ADD         R2,R3
        SHLL2       R3
        ADD         R3,R1
        FMOV.S      @R1,FR0
        FMUL        FR1,FR0
?specular:
        MOV         #20,R1
        MOV.W       ?mspc,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         #4,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
?ratten:
        .DATA.W     H'0060
?lmvctr:
        .DATA.W     H'00A8
?cosi:
        .DATA.W     H'0070
?mdif_r:
        .DATA.W     H'0108
?rate:
        .DATA.W     H'0080
?exp:
        .DATA.W     H'0088
?rows:
        .DATA.W     H'0100
?mspc:
        .DATA.W     H'0114
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
}

/* type 0x03: add type 0x01's share, then diffuse -(n . lvctr) where
 * n . lvctr is not positive */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626726 */
#pragma inline_asm(lt_rgb_03)
static void lt_rgb_03(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       #H'009C,R2
        MOV         R7,R1
        FLDI0       FR7
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        ADD         R4,R2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FIPR        FV4,FV8
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      @R2,FR3
        FCMP/GT     FR7,FR11
        BT          ?done
        FNEG        FR11
        MOV         R3,R2
        FMOV.S      FR11,FR0
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0x83: add type 0x01's share, then type 0x82's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c62679e */
#pragma inline_asm(lt_rgb_83)
static void lt_rgb_83(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       #H'009C,R2
        MOV         R7,R1
        FLDI0       FR7
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        ADD         R4,R2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FIPR        FV4,FV8
        MOV         R5,R3
        ADD         R0,R3
        MOVA        ?half,R0
        ADD         R4,R2
        FLDI1       FR0
        FMOV.S      @R0,FR8
        FSUB        FR11,FR0
        MOV         R3,R1
        FMUL        FR8,FR0
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         #4,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        RTS
        FMOV.S      FR6,@R1
        .DATA.W     H'0000
?half:
        .FDATA.S    F'0.5
}

/* types 0x05 and 0x85: add type 0x01's share, then type 0x04's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626826 */
#pragma inline_asm(lt_rgb_05)
static void lt_rgb_05(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       #H'0090,R2
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       #H'0068,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR10
        FIPR        FV4,FV0
        MOV         #0,R0
        FCMP/GT     FR11,FR10
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        CMP/EQ      #3,R0
        BF          ?done
        FMOV.S      FR11,FR9
        FSRRA       FR11
        FMOV.S      FR3,FR0
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FCMP/GT     FR8,FR9
        BF          ?diffuse
        FDIV        FR9,FR0
        FMUL        FR8,FR0
?diffuse:
        MOV         R5,R3
        ADD         R4,R2
        ADD         R0,R3
        FMUL        FR11,FR0
        MOV         R3,R1
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0xc5: add type 0x01's share, then type 0xc4's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c6268d0 */
#pragma inline_asm(lt_rgb_c5)
static void lt_rgb_c5(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       #H'0060,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R3
        MOV.W       #H'00A8,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        ADD         R4,R2
        FMOV.S      @R3,FR3
        FMOV.S      @R1,FR6
        FCMP/EQ     FR7,FR3
        BT          ?done
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FIPR        FV4,FV8
        ADD         R4,R2
        MOV         R5,R3
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      FR3,FR10
        FMOV.S      @R2,FR3
        FCMP/GT     FR7,FR11
        BF          ?done
        FMUL        FR10,FR11
        MOV         R3,R2
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      FR11,FR0
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
?done:
        RTS
        NOP
}

/* type 0x13, NJD_PHONG_DIR: add type 0x01's share, then type 0x10's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c62694c */
#pragma inline_asm(lt_rgb_13)
static void lt_rgb_13(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       #H'009C,R2
        MOV.W       #H'0070,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        FMOV.S      @R1,FR10
        ADD         R4,R3
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2,FR6
        MOV.W       #H'0108,R2
        FIPR        FV4,FV8
        MOV         #4,R0
        ADD         R4,R2
        MOV         R5,R1
        ADD         R0,R1
        FMOV.S      FR11,FR0
        FCMP/GT     FR11,FR7
        BF          ?done
        FMUL        FR0,FR11
        FMOV.S      @R3+,FR8
        FNEG        FR0
        FMOV.S      @R3,FR9
        FMOV.S      @R2+,FR1
        MOV         R1,R3
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R1,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR11
        BF          ?done
        FLDI1       FR0
        FCMP/GT     FR11,FR8
        BF          ?specular
        MOV.L       #_lt_power,R3
        MOV.W       #H'0080,R0
        STS.L       PR,@-R15
        FMOV.S      @(R0,R4),FR1
        MOV.W       #H'0088,R0
        MOV.L       R4,@-R15
        MOV.L       @(R0,R4),R4
        JSR         @R3
        FMOV.S      FR11,FR4
        MOV.L       @R15+,R4
        FMUL        FR1,FR0
        LDS.L       @R15+,PR
        MOV         #4,R0
?specular:
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
}

/* type 0x93: add type 0x01's share, then type 0x90's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626a2c */
#pragma inline_asm(lt_rgb_93)
static void lt_rgb_93(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       ?mamb,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       ?lvctr,R2
        MOV.W       ?cosi,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        FMOV.S      @R1,FR10
        ADD         R4,R3
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2,FR6
        MOV.W       ?mdif_r,R2
        FIPR        FV4,FV8
        MOV         #4,R0
        ADD         R4,R2
        MOV         R5,R1
        ADD         R0,R1
        FMOV.S      FR11,FR0
        FCMP/GT     FR11,FR7
        BF          ?done
        FMUL        FR0,FR11
        FMOV.S      @R3+,FR8
        FNEG        FR0
        FMOV.S      @R3,FR9
        FMOV.S      @R2+,FR1
        MOV         R1,R3
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R1,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR11
        BF          ?done
        FLDI1       FR0
        FCMP/GT     FR11,FR8
        BF          ?specular
        MOVA        ?scale,R0
        FMOV.S      @R0,FR3
        MOV.W       ?rate,R0
        FMUL        FR3,FR11
        FMOV.S      @(R0,R4),FR1
        MOV.W       ?exp,R0
        MOV.L       @(R0,R4),R3
        MOV.L       ?table,R1
        MOV.W       ?rows,R0
        FTRC        FR11,FPUL
        STS         FPUL,R2
        CMP/GE      R0,R2
        BT          ?specular
        SHLL8       R3
        ADD         R2,R3
        SHLL2       R3
        ADD         R3,R1
        FMOV.S      @R1,FR0
        FMUL        FR1,FR0
?specular:
        MOV         #20,R1
        MOV.W       ?mspc,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         #4,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
?mamb:
        .DATA.W     H'00F4
?lvctr:
        .DATA.W     H'009C
?cosi:
        .DATA.W     H'0070
?mdif_r:
        .DATA.W     H'0108
?rate:
        .DATA.W     H'0080
?exp:
        .DATA.W     H'0088
?rows:
        .DATA.W     H'0100
?mspc:
        .DATA.W     H'0114
        .DATA.W     H'0000
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
}

/* type 0x25, NJD_PHONG_POINT: add type 0x01's share, then type 0x20's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626b2a */
#pragma inline_asm(lt_rgb_25)
static void lt_rgb_25(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       #H'00F4,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       #H'0090,R2
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       #H'0068,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR10
        FIPR        FV4,FV0
        MOV         #0,R0
        FCMP/GT     FR11,FR10
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        CMP/EQ      #3,R0
        BF          ?done
        FMOV.S      FR11,FR9
        FSRRA       FR11
        MOV.W       #H'0070,R3
        FMOV.S      FR3,FR0
        ADD         R4,R3
        MOV.W       #H'0108,R2
        MOV         #4,R0
        FLDI1       FR10
        FCMP/GT     FR8,FR9
        BF          ?diffuse
        FMOV.S      FR11,FR10
        FMUL        FR11,FR10
        FMUL        FR8,FR10
?diffuse:
        FMOV.S      @R3+,FR8
        FMOV.S      @R3,FR9
        MOV         R5,R3
        ADD         R4,R2
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        FMUL        FR11,FR0
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        FMOV.S      FR0,FR7
        MOV         R3,R2
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMUL        FR7,FR7
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR10,FR0
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR7
        BF          ?done
        FCMP/GT     FR7,FR8
        BF          ?specular
        MOV.W       #H'0080,R0
        MOV.L       #_lt_power,R3
        STS.L       PR,@-R15
        FMOV.S      @(R0,R4),FR1
        FMUL        FR0,FR1
        MOV.W       #H'0088,R0
        MOV.L       R4,@-R15
        MOV.L       @(R0,R4),R4
        JSR         @R3
        FMOV.S      FR7,FR4
        MOV.L       @R15+,R4
        FMUL        FR1,FR0
        LDS.L       @R15+,PR
?specular:
        MOV         #4,R0
        MOV         #20,R1
        MOV.W       #H'0114,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
}

/* type 0xa5: add type 0x01's share, then type 0xa0's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626c4a */
#pragma inline_asm(lt_rgb_a5)
static void lt_rgb_a5(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       ?mamb,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       ?lpnt,R2
        MOV         R6,R1
        FLDI0       FR7
        ADD         R4,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        FSUB        FR4,FR8
        FSUB        FR5,FR9
        FSUB        FR6,FR10
        MOV         R7,R1
        FMOV.S      FR8,FR4
        FMOV.S      FR9,FR5
        FMOV.S      FR10,FR6
        FMOV.S      @R1+,FR0
        FIPR        FV4,FV8
        MOV.W       ?nrr,R2
        FMOV.S      @R1+,FR1
        ADD         R4,R2
        FMOV.S      @R1,FR2
        FMOV.S      @R2+,FR8
        FMOV.S      @R2,FR10
        FIPR        FV4,FV0
        MOV         #0,R0
        FCMP/GT     FR11,FR10
        ROTCL       R0
        FCMP/GT     FR7,FR3
        ROTCL       R0
        CMP/EQ      #3,R0
        BF          ?done
        FMOV.S      FR11,FR9
        FSRRA       FR11
        MOV.W       ?cosi,R3
        FMOV.S      FR3,FR0
        ADD         R4,R3
        MOV.W       ?mdif_r,R2
        MOV         #4,R0
        FLDI1       FR10
        FCMP/GT     FR8,FR9
        BF          ?diffuse
        FMOV.S      FR11,FR10
        FMUL        FR11,FR10
        FMUL        FR8,FR10
?diffuse:
        FMOV.S      @R3+,FR8
        FMOV.S      @R3,FR9
        MOV         R5,R3
        ADD         R4,R2
        ADD         R0,R3
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMUL        FR11,FR0
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        FMOV.S      FR0,FR7
        MOV         R3,R2
        ADD         R0,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        FMUL        FR7,FR7
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR10,FR0
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR7
        BF          ?done
        FCMP/GT     FR7,FR8
        BF          ?specular
        MOVA        ?scale,R0
        FMOV.S      @R0,FR3
        MOV.W       ?rate,R0
        FMUL        FR3,FR7
        FMOV.S      @(R0,R4),FR1
        MOV.W       ?exp,R0
        MOV.L       @(R0,R4),R3
        FMUL        FR0,FR1
        MOV.L       ?table,R1
        MOV.W       ?rows,R0
        FTRC        FR7,FPUL
        STS         FPUL,R2
        CMP/GE      R0,R2
        BT          ?specular
        SHLL8       R3
        ADD         R2,R3
        SHLL2       R3
        ADD         R3,R1
        FMOV.S      @R1,FR0
        FMUL        FR1,FR0
?specular:
        MOV         #4,R0
        MOV         #20,R1
        MOV.W       ?mspc,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        MOV         R3,R1
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
?mamb:
        .DATA.W     H'00F4
?lpnt:
        .DATA.W     H'0090
?nrr:
        .DATA.W     H'0068
?cosi:
        .DATA.W     H'0070
?mdif_r:
        .DATA.W     H'0108
?rate:
        .DATA.W     H'0080
?exp:
        .DATA.W     H'0088
?rows:
        .DATA.W     H'0100
?mspc:
        .DATA.W     H'0114
        .DATA.W     H'0000
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
}

/* type 0xe5: add type 0x01's share, then type 0xe0's */
/* @asm operand: fipr-registers, FIPR FV4,FV8 at 0x8c626d7a */
#pragma inline_asm(lt_rgb_e5)
static void lt_rgb_e5(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                      NJS_VECTOR *normal)
{
        MOV.W       ?mamb,R2
        ADD         R4,R2
        MOV         #4,R0
        FMOV.S      @(R0,R5),FR0
        FMOV.S      @(R0,R2),FR1
        FADD        FR1,FR0
        FMOV.S      FR0,@(R0,R5)
        MOV         #8,R0
        FMOV.S      @(R0,R5),FR2
        FMOV.S      @(R0,R2),FR3
        FADD        FR3,FR2
        FMOV.S      FR2,@(R0,R5)
        MOV         #12,R0
        FMOV.S      @(R0,R5),FR4
        FMOV.S      @(R0,R2),FR5
        FADD        FR5,FR4
        FMOV.S      FR4,@(R0,R5)
        MOV.W       ?ratten,R3
        MOV         R7,R1
        FLDI0       FR7
        ADD         R4,R3
        MOV.W       ?lmvctr,R2
        FMOV.S      @R1+,FR4
        FMOV.S      @R1+,FR5
        ADD         R4,R2
        FMOV.S      @R3+,FR0
        FMOV.S      @R1,FR6
        FCMP/EQ     FR7,FR0
        BT          ?done
        FMOV.S      @R2+,FR8
        FMOV.S      @R2+,FR9
        FMOV.S      @R2,FR10
        MOV.W       ?cosi,R3
        FIPR        FV4,FV8
        MOV         #4,R0
        MOV.W       ?mdif_r,R2
        ADD         R4,R3
        ADD         R4,R2
        MOV         R5,R1
        ADD         R0,R1
        FMOV.S      FR0,FR10
        FMOV.S      FR11,FR0
        FCMP/GT     FR7,FR11
        BF          ?done
        FMUL        FR0,FR11
        FMOV.S      @R3+,FR8
        FMOV.S      @R3,FR9
        FMOV.S      @R2+,FR1
        MOV         R1,R3
        FMOV.S      @R2+,FR2
        FMOV.S      @R2,FR3
        MOV         R1,R2
        FMOV.S      @R1+,FR4
        ADD         R0,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FCMP/GT     FR9,FR11
        BF          ?done
        FMOV.S      FR10,FR0
        FCMP/GT     FR11,FR8
        BF          ?specular
        MOVA        ?scale,R0
        FMOV.S      @R0,FR3
        MOV.W       ?rate,R0
        FMUL        FR3,FR11
        FMOV.S      @(R0,R4),FR1
        MOV.W       ?exp,R0
        MOV.L       @(R0,R4),R3
        FMUL        FR0,FR1
        MOV.L       ?table,R1
        MOV.W       ?rows,R0
        FTRC        FR11,FPUL
        STS         FPUL,R2
        CMP/GE      R0,R2
        BT          ?specular
        SHLL8       R3
        ADD         R2,R3
        SHLL2       R3
        ADD         R3,R1
        FMOV.S      @R1,FR0
        FMUL        FR1,FR0
?specular:
        MOV         #20,R1
        MOV.W       ?mspc,R2
        MOV         R5,R3
        ADD         R4,R2
        ADD         R1,R3
        FMOV.S      @R5,FR8
        FMOV.S      @R2+,FR7
        FMOV.S      @R2+,FR1
        FMOV.S      @R2+,FR2
        MOV         R3,R1
        FMOV.S      @R2,FR3
        MOV         R3,R2
        FMOV.S      @R1+,FR4
        ADD         #4,R2
        FMOV.S      @R1+,FR5
        FMOV.S      @R1,FR6
        FMAC        FR0,FR1,FR4
        FMAC        FR0,FR2,FR5
        FMAC        FR0,FR3,FR6
        FMAC        FR0,FR7,FR8
        FMOV.S      FR4,@R3
        FMOV.S      FR5,@R2
        FMOV.S      FR6,@R1
        FMOV.S      FR8,@R5
?done:
        RTS
        NOP
?mamb:
        .DATA.W     H'00F4
?ratten:
        .DATA.W     H'0060
?lmvctr:
        .DATA.W     H'00A8
?cosi:
        .DATA.W     H'0070
?mdif_r:
        .DATA.W     H'0108
?rate:
        .DATA.W     H'0080
?exp:
        .DATA.W     H'0088
?rows:
        .DATA.W     H'0100
?mspc:
        .DATA.W     H'0114
?scale:
        .FDATA.S    F'256.0
?table:
        .DATA.L     _fpow_tbl
}
#endif
/* clang-format on */

static void lt_clear_color(NJS_ARGB *color)
{
    color->a = 0.0F;
    color->r = 0.0F;
    color->g = 0.0F;
    color->b = 0.0F;
}

/* clang-format off */
#ifndef __clang__
/* lt_rgb_lights's body. it loads _nj_ptr_rgb_lt_argb_, _nj_ptr_rgb_lt_pnt_ and
 * _nj_ptr_rgb_lt_nml_ into R5 to R7 once, then calls each queued light's
 * calculator in queue order. before each call it clears FR3 and FR11: a
 * calculator's FIPR takes FR3 or FR11 as a fourth element and multiplies it by
 * FR7, which holds 0, and an infinity or a NaN there would make the sum a
 * NaN */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c626ea0 follows the
 * body's own return at 0x8c626e82 */
#pragma inline_asm(lt_rgb_lights_body)
static void lt_rgb_lights_body(void)
{
        MOV.L       #__nj_ptr_rgb_lt_argb_,R0
        MOV.L       #__nj_ptr_rgb_lt_pnt_,R1
        MOV.L       #__nj_ptr_rgb_lt_nml_,R2
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        STS.L       PR,@-R15
        MOV.L       @R0,R5
        MOV.L       @R1,R6
        MOV.L       @R2,R7
        MOV.L       #__nj_light_queue_+128,R3
        MOV.L       #__nj_ptr_rgb_lt_,R13
        MOV.L       #__nj_rgb_lt_func_,R12
        MOV.L       @R3,R14
        TST         R14,R14
        BT          ?done
?next:
        FLDI0       FR3
        FLDI0       FR11
        MOV.L       @R12,R3
        JSR         @R3
        MOV.L       @R13,R4
        ADD         #4,R12
        ADD         #4,R13
        DT          R14
        BF          ?next
?done:
        LDS.L       @R15+,PR
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        RTS
        MOV.L       @R15+,R14
}
#endif
/* clang-format on */

static void lt_rgb_lights(void)
{
    lt_rgb_lights_body();
}

/* fade the diffuse alpha, _nj_ptr_rgb_lt_argb_[0].a, by the z of
 * _nj_ptr_rgb_lt_pnt_; point goes unread. with _nj_depth_enable_ set, the
 * alpha is scaled by 1 - (z - _nj_depth_near_) * _nj_depth_range_ where z
 * lies past _nj_depth_near_ and short of _nj_depth_far_, and is 0 from
 * _nj_depth_far_ on */
static void lt_depth_fade(NJS_POINT3 *point)
{
    Float fade;

    if (_nj_depth_enable_) {
        if (_nj_ptr_rgb_lt_pnt_->z > _nj_depth_near_) {
            if (_nj_ptr_rgb_lt_pnt_->z < _nj_depth_far_) {
                fade = -((_nj_ptr_rgb_lt_pnt_->z - _nj_depth_near_) *
                         _nj_depth_range_) +
                       1.0F;
                _nj_ptr_rgb_lt_argb_->a *= fade;
            } else {
                _nj_ptr_rgb_lt_argb_->a = 0.0F;
            }
        }
    }
}

void njCalculateLightsRGB(NJS_ARGB *color, NJS_POINT3 *point,
                          NJS_VECTOR *normal, Int reverse)
{
    NJS_VECTOR flipped;
    Float fade;

    _nj_ptr_rgb_lt_nml_ = normal;
    _nj_ptr_rgb_lt_pnt_ = point;
    _nj_ptr_rgb_lt_argb_ = color;
    _nj_ptr_rgb_lt_argb_->a = _nj_ptr_rgb_lt_argb_->r =
        _nj_ptr_rgb_lt_argb_->g = _nj_ptr_rgb_lt_argb_->b = 0.0F;
    (_nj_ptr_rgb_lt_argb_ + 1)->r = (_nj_ptr_rgb_lt_argb_ + 1)->g =
        (_nj_ptr_rgb_lt_argb_ + 1)->b = 0.0F;
    if (reverse) {
        flipped.x = -_nj_ptr_rgb_lt_nml_->x;
        flipped.y = -_nj_ptr_rgb_lt_nml_->y;
        flipped.z = -_nj_ptr_rgb_lt_nml_->z;
        _nj_ptr_rgb_lt_nml_ = &flipped;
    }
    lt_rgb_lights();
    _nj_ptr_rgb_lt_argb_->a += _nj_light_material_.diffuse.a;
    /* @hack spelling: lt_depth_fade's body, written out in place of a call: a
     * call compiles to a BSR, and the file's code section, PSG, then ends 60
     * bytes short of the image's, and 84 of njCalculateLightsRGB's 103 words
     * differ */
    if (_nj_depth_enable_) {
        if (_nj_ptr_rgb_lt_pnt_->z > _nj_depth_near_) {
            if (_nj_ptr_rgb_lt_pnt_->z < _nj_depth_far_) {
                fade = -((_nj_ptr_rgb_lt_pnt_->z - _nj_depth_near_) *
                         _nj_depth_range_) +
                       1.0F;
                _nj_ptr_rgb_lt_argb_->a *= fade;
            } else {
                _nj_ptr_rgb_lt_argb_->a = 0.0F;
            }
        }
    }
}

void njCalculateLightsRGBVertex(NJS_ARGB *color, NJS_POINT3 *point,
                                NJS_VECTOR *normal, Int reverse)
{
    NJS_VECTOR flipped;
    NJS_ARGB light[2];

    _nj_ptr_rgb_lt_nml_ = normal;
    _nj_ptr_rgb_lt_pnt_ = point;
    _nj_ptr_rgb_lt_argb_ = light;
    lt_clear_color(_nj_ptr_rgb_lt_argb_);
    lt_clear_color(_nj_ptr_rgb_lt_argb_ + 1);
    if (reverse) {
        flipped.x = -_nj_ptr_rgb_lt_nml_->x;
        flipped.y = -_nj_ptr_rgb_lt_nml_->y;
        flipped.z = -_nj_ptr_rgb_lt_nml_->z;
        _nj_ptr_rgb_lt_nml_ = &flipped;
    }
    lt_rgb_lights();
    _nj_ptr_rgb_lt_argb_->a += _nj_light_material_.diffuse.a;
    lt_depth_fade(point);
    color->a *= _nj_ptr_rgb_lt_argb_->a;
    color->r *= _nj_ptr_rgb_lt_argb_->r;
    color->g *= _nj_ptr_rgb_lt_argb_->g;
    color->b *= _nj_ptr_rgb_lt_argb_->b;
    (color + 1)->r *= (_nj_ptr_rgb_lt_argb_ + 1)->r;
    (color + 1)->g *= (_nj_ptr_rgb_lt_argb_ + 1)->g;
    (color + 1)->b *= (_nj_ptr_rgb_lt_argb_ + 1)->b;
}

void njCalculateLightsRGBPacked(Uint32 *color, NJS_POINT3 *point,
                                NJS_VECTOR *normal, Int reverse)
{
    NJS_VECTOR flipped;
    NJS_ARGB light[2];
    Uint32 a;
    Uint32 r;
    Uint32 g;
    Uint32 b;

    _nj_ptr_rgb_lt_nml_ = normal;
    _nj_ptr_rgb_lt_pnt_ = point;
    _nj_ptr_rgb_lt_argb_ = light;
    lt_clear_color(_nj_ptr_rgb_lt_argb_);
    lt_clear_color(_nj_ptr_rgb_lt_argb_ + 1);
    if (reverse) {
        flipped.x = -_nj_ptr_rgb_lt_nml_->x;
        flipped.y = -_nj_ptr_rgb_lt_nml_->y;
        flipped.z = -_nj_ptr_rgb_lt_nml_->z;
        _nj_ptr_rgb_lt_nml_ = &flipped;
    }
    lt_rgb_lights();
    _nj_ptr_rgb_lt_argb_->a += _nj_light_material_.diffuse.a;
    /* @hack redundant: steps of _nj_ptr_rgb_lt_nml_ and _nj_ptr_rgb_lt_argb_
     * that never run, which make SHC truncate a, r, g and b into R6, R7, R5
     * and R13, at the STS FPUL at 0x8c62715a, 0x8c62716a, 0x8c627172 and
     * 0x8c62717c; without it, 44 of 143 words differ */
    if (0) {
        _nj_ptr_rgb_lt_nml_ = _nj_ptr_rgb_lt_nml_ + 1;
        _nj_ptr_rgb_lt_argb_ = _nj_ptr_rgb_lt_argb_ + 1;
        _nj_ptr_rgb_lt_argb_ = _nj_ptr_rgb_lt_argb_ + 1;
    }
    lt_depth_fade(point);
    a = (Sint32)(_nj_ptr_rgb_lt_argb_->a * 255.0F);
    r = (Sint32)(_nj_ptr_rgb_lt_argb_->r * 255.0F);
    g = (Sint32)(_nj_ptr_rgb_lt_argb_->g * 255.0F);
    b = (Sint32)(_nj_ptr_rgb_lt_argb_->b * 255.0F);
    if (a > 255) {
        a = 255;
    }
    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    color[0] = a << 24 | r << 16 | g << 8 | b;
    r = (Sint32)((_nj_ptr_rgb_lt_argb_ + 1)->r * 255.0F);
    g = (Sint32)((_nj_ptr_rgb_lt_argb_ + 1)->g * 255.0F);
    b = (Sint32)((_nj_ptr_rgb_lt_argb_ + 1)->b * 255.0F);
    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    /* @hack redundant: an empty block that never runs, which makes SHC load
     * color into R3 and 0xff000000 into R2 for the specular store, at
     * 0x8c6271d8 and 0x8c6271dc; without it, 4 of 143 words differ */
    if (0) {
    }
    color[1] = r << 16 | g << 8 | b | 0xff000000;
}

void njCalculateLightsRGBPackedVertex(Uint32 *color, NJS_POINT3 *point,
                                      NJS_VECTOR *normal, Int reverse)
{
    NJS_VECTOR flipped;
    NJS_ARGB light[2];
    Uint32 a;
    Uint32 r;
    Uint32 g;
    Uint32 b;

    _nj_ptr_rgb_lt_nml_ = normal;
    _nj_ptr_rgb_lt_pnt_ = point;
    _nj_ptr_rgb_lt_argb_ = light;
    lt_clear_color(_nj_ptr_rgb_lt_argb_);
    lt_clear_color(_nj_ptr_rgb_lt_argb_ + 1);
    if (reverse) {
        flipped.x = -_nj_ptr_rgb_lt_nml_->x;
        flipped.y = -_nj_ptr_rgb_lt_nml_->y;
        flipped.z = -_nj_ptr_rgb_lt_nml_->z;
        _nj_ptr_rgb_lt_nml_ = &flipped;
    }
    lt_rgb_lights();
    _nj_ptr_rgb_lt_argb_->a += _nj_light_material_.diffuse.a;
    lt_depth_fade(point);
    a = (Sint32)(_nj_ptr_rgb_lt_argb_->a * (Float)(*color >> 24 & 0xff));
    r = (Sint32)(_nj_ptr_rgb_lt_argb_->r * (Float)(*color >> 16 & 0xff));
    g = (Sint32)(_nj_ptr_rgb_lt_argb_->g * (Float)(*color >> 8 & 0xff));
    b = (Sint32)(_nj_ptr_rgb_lt_argb_->b * (Float)(*color & 0xff));
    if (a > 255) {
        a = 255;
    }
    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    /* @hack redundant: a store that never runs, which makes SHC truncate a, r,
     * g and b into R5, R13, R6 and R7, at the STS FPUL at 0x8c6272b0,
     * 0x8c6272d6, 0x8c6272f4 and 0x8c627312; without it, 49 of 223 words
     * differ */
    if (0) {
        color[a + a + a + g + g + 255] = (Uint32)color;
    }
    *color = a << 24 | r << 16 | g << 8 | b;
    r = (Sint32)((_nj_ptr_rgb_lt_argb_ + 1)->r *
                 (Float)(color[1] >> 16 & 0xff));
    g = (Sint32)((_nj_ptr_rgb_lt_argb_ + 1)->g * (Float)(color[1] >> 8 & 0xff));
    b = (Sint32)((_nj_ptr_rgb_lt_argb_ + 1)->b * (Float)(color[1] & 0xff));
    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    /* @hack redundant: an empty block that never runs, which makes SHC load
     * color + 1 into R3 and 0xff000000 into R2 for the specular store, at
     * 0x8c6273d4 and 0x8c6273d8; without it, 4 of 223 words differ */
    if (0) {
    }
    color[1] = r << 16 | g << 8 | b | 0xff000000;
}

/* fill _nj_ptr_rgb_lt_ with the queued lights, in queue order, and
 * _nj_rgb_lt_func_ with the calculator for each one's type. each index in the
 * queue must name a created light */
void njSetLightQueue(void)
{
    Int i;

    i = _nj_light_queue_.count;
    while (i != 0) {
        i--;
        _nj_ptr_rgb_lt_[i] =
            _nj_light_control_.light[_nj_light_queue_.light[i]];
        switch (_nj_ptr_rgb_lt_[i]->attr.lsrc) {
        case 0x00:
            _nj_rgb_lt_func_[i] = lt_rgb_00;
            break;
        case 0x01:
            _nj_rgb_lt_func_[i] = lt_rgb_01;
            break;
        case 0x02:
            _nj_rgb_lt_func_[i] = lt_rgb_02;
            break;
        case 0x82:
            _nj_rgb_lt_func_[i] = lt_rgb_82;
            break;
        case 0x04:
        case 0x84:
            _nj_rgb_lt_func_[i] = lt_rgb_04;
            break;
        case 0xc4:
            _nj_rgb_lt_func_[i] = lt_rgb_c4;
            break;
        case 0x08:
            _nj_rgb_lt_func_[i] = lt_rgb_08;
            break;
        case 0x88:
            _nj_rgb_lt_func_[i] = lt_rgb_88;
            break;
        case 0xc8:
            _nj_rgb_lt_func_[i] = lt_rgb_c8;
            break;
        case 0x40:
            _nj_rgb_lt_func_[i] = lt_rgb_40;
            break;
        case 0x10:
            _nj_rgb_lt_func_[i] = lt_rgb_10;
            break;
        case 0x90:
            _nj_rgb_lt_func_[i] = lt_rgb_90;
            break;
        case 0x20:
            _nj_rgb_lt_func_[i] = lt_rgb_20;
            break;
        case 0xa0:
            _nj_rgb_lt_func_[i] = lt_rgb_a0;
            break;
        case 0xe0:
            _nj_rgb_lt_func_[i] = lt_rgb_e0;
            break;
        case 0x03:
            _nj_rgb_lt_func_[i] = lt_rgb_03;
            break;
        case 0x83:
            _nj_rgb_lt_func_[i] = lt_rgb_83;
            break;
        case 0x05:
        case 0x85:
            _nj_rgb_lt_func_[i] = lt_rgb_05;
            break;
        case 0xc5:
            _nj_rgb_lt_func_[i] = lt_rgb_c5;
            break;
        case 0x13:
            _nj_rgb_lt_func_[i] = lt_rgb_13;
            break;
        case 0x93:
            _nj_rgb_lt_func_[i] = lt_rgb_93;
            break;
        case 0x25:
            _nj_rgb_lt_func_[i] = lt_rgb_25;
            break;
        case 0xa5:
            _nj_rgb_lt_func_[i] = lt_rgb_a5;
            break;
        case 0xe5:
            _nj_rgb_lt_func_[i] = lt_rgb_e5;
            break;
        default:
            _nj_rgb_lt_func_[i] = lt_rgb_82;
            break;
        }
    }
}

void njGetLightDistance(NJS_POINT3 *point)
{
    Int i;
    NJS_LIGHT *light;
    Float d2;
    Float inverse;
    Float ip;

    i = _nj_light_queue_.count;
    while (i != 0) {
        i--;
        light = _nj_light_control_.light[_nj_light_queue_.light[i]];
        switch (light->attr.lsrc) {
        case 0xc4:
        case 0xc5:
        case 0xe0:
        case 0xe5:
            d2 = ((light->ltcal.lpnt.x - point->x) *
                  (light->ltcal.lpnt.x - point->x)) +
                 ((light->ltcal.lpnt.y - point->y) *
                  (light->ltcal.lpnt.y - point->y)) +
                 ((light->ltcal.lpnt.z - point->z) *
                  (light->ltcal.lpnt.z - point->z));
            if (light->ltcal.frr > d2) {
                inverse = 1.0F / njSqrt(d2);
                light->ltcal.lmvctr.x =
                    (light->ltcal.lpnt.x - point->x) * inverse;
                light->ltcal.lmvctr.y =
                    (light->ltcal.lpnt.y - point->y) * inverse;
                light->ltcal.lmvctr.z =
                    (light->ltcal.lpnt.z - point->z) * inverse;
                if (d2 > light->ltcal.nrr) {
                    light->ltcal.ratten = light->ltcal.nrr / d2;
                } else {
                    light->ltcal.ratten = 1.0F;
                }
            } else {
                light->ltcal.ratten = 0.0F;
            }
            break;
        case 0xc8:
            d2 = ((light->ltcal.lpnt.x - point->x) *
                  (light->ltcal.lpnt.x - point->x)) +
                 ((light->ltcal.lpnt.y - point->y) *
                  (light->ltcal.lpnt.y - point->y)) +
                 ((light->ltcal.lpnt.z - point->z) *
                  (light->ltcal.lpnt.z - point->z));
            inverse = 1.0F / njSqrt(d2);
            light->ltcal.lmvctr.x = (light->ltcal.lpnt.x - point->x) * inverse;
            light->ltcal.lmvctr.y = (light->ltcal.lpnt.y - point->y) * inverse;
            light->ltcal.lmvctr.z = (light->ltcal.lpnt.z - point->z) * inverse;
            ip = (light->ltcal.lmvctr.x * light->ltcal.lvctr.x) +
                 (light->ltcal.lmvctr.y * light->ltcal.lvctr.y) +
                 (light->ltcal.lmvctr.z * light->ltcal.lvctr.z);
            light->ltcal.ipd = ip * ip;
            if (light->ltcal.frr > d2 && ip < 0.0F &&
                light->ltcal.ipd > light->ltcal.cose) {
                if (d2 > light->ltcal.nrr) {
                    light->ltcal.ratten = light->ltcal.nrr / d2;
                } else {
                    light->ltcal.ratten = 1.0F;
                }
                if (light->ltcal.ipd < light->ltcal.cosi) {
                    light->ltcal.ratten *=
                        light->ltcal.rate *
                        (light->ltcal.ipd - light->ltcal.cose);
                }
            } else {
                light->ltcal.ratten = 0.0F;
            }
            break;
        }
    }
}

void njCalcLightMaterials(void)
{
    Int i;
    NJS_LIGHT *light;
    Int exponent;
    Int type;

    i = _nj_light_queue_.count;
    while (i != 0) {
        i--;
        light = _nj_light_control_.light[_nj_light_queue_.light[i]];
        light->ltcal.mamb.r =
            _nj_light_material_.ambient.r * light->ltcal.amb.r;
        light->ltcal.mamb.g =
            _nj_light_material_.ambient.g * light->ltcal.amb.g;
        light->ltcal.mamb.b =
            _nj_light_material_.ambient.b * light->ltcal.amb.b;
        light->ltcal.mdif.r =
            _nj_light_material_.diffuse.r * light->ltcal.dif.r;
        light->ltcal.mdif.g =
            _nj_light_material_.diffuse.g * light->ltcal.dif.g;
        light->ltcal.mdif.b =
            _nj_light_material_.diffuse.b * light->ltcal.dif.b;
        light->ltcal.mspc.r =
            _nj_light_material_.specular.r * light->ltcal.spc.r;
        light->ltcal.mspc.g =
            _nj_light_material_.specular.g * light->ltcal.spc.g;
        light->ltcal.mspc.b =
            _nj_light_material_.specular.b * light->ltcal.spc.b;
        light->ltcal.mspc.a = light->ltcal.spc.a;
        /* @hack spelling: the exponent through a local, which SHC truncates
         * into R4 (STS FPUL,R4 at 0x8c6279a2); stored directly, it takes R3,
         * the entry loads R10 and R13 later (0x8c6278ea to 0x8c6278fe), and
         * 7 of 131 words differ */
        exponent = (Int)_nj_light_material_.exponent;
        light->ltcal.exp = exponent;
        type = light->attr.lsrc;
        /* @bug logic: type != 0x88 || type != 0xc8 holds for every type,
         * so types 0x88 and 0xc8 take this rate too, in place of the
         * 1 / |cosi - cose| that njCreateLight and njSetLightMaterials give
         * them. the CMP/EQ at 0x8c6279b8 runs only for type 0x88, so the BT
         * at 0x8c6279ba never branches */
        /* NOLINTNEXTLINE(*-redundant-expression,*-overlap-compare) */
        if ((type & 0x38) && (type != 0x88 || type != 0xc8)) {
            light->ltcal.rate =
                1.0F / lt_power(light->ltcal.cosi, light->ltcal.exp);
        }
    }
}

void njCheckLightMaterials(Int flags)
{
    NJS_LIGHT *light;
    Int mode;
    Int i;

    i = _nj_light_queue_.count;
    /* mode keeps bit 19 of flags, and bit 1 where bit 0 is set, so it is 0, 2,
     * 0x80000 or 0x80002. the switch's cases 3 and 1 never run, and 0x80000 and
     * 0x80002 match no case: only mode 2 sets anything */
    mode = flags & (((flags & 1) << 1) + 0x80000);
    switch (mode) {
    case 0:
        break;
    case 3:
        while (i) {
            i--;
            light = _nj_light_control_.light[_nj_light_queue_.light[i]];
            light->ltcal.mamb.r =
                _nj_light_material_.diffuse.r * light->ltcal.amb.r;
            light->ltcal.mamb.g =
                _nj_light_material_.diffuse.g * light->ltcal.amb.g;
            light->ltcal.mamb.b =
                _nj_light_material_.diffuse.b * light->ltcal.amb.b;
            light->ltcal.mspc.b = 0.0F;
            light->ltcal.mspc.g = 0.0F;
            light->ltcal.mspc.r = 0.0F;
            light->ltcal.mspc.a = _nj_light_material_.diffuse.a;
            /* @hack redundant: a store of light that never runs, which makes
             * SHC keep mode, then each light, in R5 and the count in R6, at
             * 0x8c627a12, 0x8c627a24 and 0x8c627a56; without it, 54 of 129
             * words differ */
            if (0) {
                _nj_light_queue_.count = (Int)light;
            }
        }
        /* @hack spelling: a return where the switch's end returns anyway, which
         * ends this case in its own RTS at 0x8c627a98; with break, 2 of 129
         * words differ */
        return;
    case 2:
        while (i) {
            i--;
            light = _nj_light_control_.light[_nj_light_queue_.light[i]];
            light->ltcal.mamb.r =
                _nj_light_material_.diffuse.r * light->ltcal.amb.r;
            light->ltcal.mamb.g =
                _nj_light_material_.diffuse.g * light->ltcal.amb.g;
            light->ltcal.mamb.b =
                _nj_light_material_.diffuse.b * light->ltcal.amb.b;
        }
        /* @hack spelling: a return where the switch's end returns anyway, which
         * ends this case in its own RTS at 0x8c627ade; with break, 2 of 129
         * words differ */
        return;
    case 1:
        while (i) {
            i--;
            light = _nj_light_control_.light[_nj_light_queue_.light[i]];
            light->ltcal.mspc.b = 0.0F;
            light->ltcal.mspc.g = 0.0F;
            light->ltcal.mspc.r = 0.0F;
            light->ltcal.mspc.a = _nj_light_material_.diffuse.a;
        }
        break;
    }
}

void njSetQueue(void)
{
    Int i;
    Int count = _nj_light_control_.count;

    if (count > 0) {
        for (i = 0; i < count; i++) {
            if (_nj_light_control_.light[i]->stat == 1) {
                _nj_light_queue_.light[_nj_light_queue_.count] = i;
                _nj_light_queue_.count++;
            }
        }
        njSetLightQueue();
    }
}
