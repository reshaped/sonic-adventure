#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* light_power_body and create_light have bodies only SHC reads, so clang sees
 * them declared and never defined */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static Float light_power_body(Float value, Int step);
static void create_light(NJS_LIGHT *l, Int type);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

/* clang-format off */
#ifndef __clang__
/* light_power's body, which SHC expands into light_power. its label
 * _light_power names light_power's entry for create_light's BSR, since SHC
 * gives the static function an internal label */
/* @asm open: no inspect rule reads its table switch on R4, which ends in BRAF
 * R0 */
#pragma inline_asm(light_power_body)
static Float light_power_body(Float value, Int step)
{
_light_power:
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

#ifdef GCC
/* light_power_body's text defines _light_power, the label GCC would give
 * light_power. light_power takes another, so create_light's BSR
 * _light_power reaches the body */
static Float light_power(Float value, Int step) __asm__("_light_power_c");
#endif

/* value to the specular power that step stands for, as NJS_LIGHT_CAL.exp lists
 * them */
static Float light_power(Float value, Int step)
{
    return light_power_body(value, step);
}

#ifdef GCC
/* create_light's body keeps &l->attr in R7 and &l->ltcal in R6 across each
 * BSR _light_scale_color, and GCC's code for light_scale_color may change
 * both. so light_scale_color takes the label _light_scale_color_c, and
 * _light_scale_color is a veneer: it saves R6 and R7, calls
 * light_scale_color and restores them. the veneer starts a section of its
 * own, at offset 0, so the BSR lands on it: GNU as relocates a branch to a
 * symbol of another section past the symbol by its offset there */
static void light_scale_color(NJS_ARGB *output, const NJS_ARGB *input,
                              Float scale) __asm__("_light_scale_color_c");
__asm__("\t.pushsection\t.text._light_scale_color,\"ax\",@progbits\n"
        "\t.global\t_light_scale_color\n"
        "\t.type\t_light_scale_color, @function\n"
        "_light_scale_color:\n"
        "\tmov.l\tr6,@-r15\n"
        "\tmov.l\tr7,@-r15\n"
        "\tsts.l\tpr,@-r15\n"
        "\tmov.l\t1f,r0\n"
        "\tjsr\t@r0\n"
        "\tnop\n"
        "\tlds.l\t@r15+,pr\n"
        "\tmov.l\t@r15+,r7\n"
        "\trts\n"
        "\tmov.l\t@r15+,r6\n"
        "\t.align\t2\n"
        "1:\t.long\t%a0\n"
        "\t.size\t_light_scale_color, . - _light_scale_color\n"
        "\t.popsection\n"
        :
        : "i"(light_scale_color));
#endif

/* output gets input's a, r, g and b times scale */
void light_scale_color(NJS_ARGB *output, const NJS_ARGB *input, Float scale)
{
    /* @hack spelling: Float pointers walk input's and output's a, r, g and b,
     * which SHC loads with FMOV.S @R5+ and stores in ADD #4 steps (0x8c62599c
     * to 0x8c6259ba); read and stored by member, 16 of 16 words differ and the
     * section is 8 bytes longer */
    const Float *in = &input->a;
    Float *out = &output->a;
    Float a;
    Float r;
    Float g;
    Float b;

    a = *in++ * scale;
    r = *in++ * scale;
    g = *in++ * scale;
    b = *in++ * scale;
    /* @hack redundant: a never-run test of in, which keeps the last load's
     * increment, FMOV.S @R5+,FR0 at 0x8c6259a8; without it, @R5, and 1 of 16
     * words differ */
    if (0) {
        if (in != NULL) {
        }
    }
    *out++ = a;
    *out++ = r;
    *out++ = g;
    *out = b;
    /* @hack redundant: a never-run trap, and a test of the four products
     * after it, which keeps them in FR3 to FR0 (0x8c62599e to 0x8c6259aa):
     * across the trap SHC colors them FR14, FR13, FR12 and FR15, which its
     * late mapping moves to FR3 to FR0; without it, they take FR5, FR6, FR7
     * and FR4, the function is 34 bytes, and 16 of 16 words differ */
    if (0) {
        trapa(0);
        if (a == r || g == b) {
        }
    }
}

/* clang-format off */
#ifndef __clang__
/* njCreateLight's body, which sets l up as ninja.h says. it keeps &l->attr in
 * R7 and &l->ltcal in R6 across its calls. the calling convention lets a
 * callee change R6 and R7, but light_power and light_scale_color write no
 * register but R0, R1, R4, R5 and FR0 to FR4 */
/* @asm operand: fsca-registers, FSCA FPUL,DR2 at 0x8c625a62 keeps the inner
 * cosine in FR1 from FSCA FPUL,DR0 at 0x8c625a5e */
#pragma inline_asm(create_light)
static void create_light(NJS_LIGHT *l, Int type)
{
        .IMPORT     __nj_light_control_
        .IMPORT     __nj_light_material_
        .IMPORT     _njUnitMatrix
        MOV.W       #H'0080,R0
        MOV.L       #__nj_light_control_,R3
        MOV         #32,R1
        MOV.L       @(R0,R3),R2
        CMP/GE      R1,R2
        BF/S        ?room
        MOV         R2,R1
        RTS
        NOP
?room:
        MOV.L       R14,@-R15
        STS.L       PR,@-R15
        MOV         R4,R14
        SHLL2       R1
        ADD         R3,R1
        MOV.L       R4,@R1
        ADD         #1,R2
        MOV.L       R2,@(R0,R3)
        MOV.W       #H'0124,R7
        MOV.W       #H'0060,R6
        ADD         R4,R7
        ADD         R4,R6
        FLDI1       FR11
        MOV         #1,R1
        FMOV.S      FR11,FR10
        MOV         #88,R3
        FLDI0       FR9
        ADD         R4,R3
        FNEG        FR10
        MOV.L       R1,@R3
        ADD         #-4,R3
        FMOV.S      FR10,@R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        MOV.W       #H'1555,R2
        MOV.W       #H'071C,R1
        MOVA        #H'477FFF00,R0
        FMOV.S      @R0,FR3
        MOVA        #H'3E999999,R0
        FMOV.S      @R0,FR2
        MOV         #48,R3
        ADD         R7,R3
        MOVA        #H'3E4CCCCC,R0
        FMOV.S      @R0,FR1
        FMOV.S      FR11,@R3
        FMOV.S      FR11,@-R3
        FMOV.S      FR11,@-R3
        FMOV.S      FR9,@-R3
        MOV.L       R2,@-R3
        MOV.L       R1,@-R3
        ADD         #-4,R3
        FMOV.S      FR3,@-R3
        FMOV.S      FR11,@-R3
        FMOV.S      FR2,@-R3
        FMOV.S      FR11,@-R3
        FMOV.S      FR1,@-R3
        MOV.L       R5,@-R3
        MOV         #68,R3
        ADD         R6,R3
        FMOV.S      FR10,@R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        FMOV.S      FR9,@-R3
        MOV         #16,R0
        FMOV.S      @(R0,R7),FR4
        MOV         #20,R0
        FMOV.S      @(R0,R7),FR3
        FMUL        FR4,FR4
        FMUL        FR3,FR3
        MOV         #8,R0
        FMOV.S      FR4,@(R0,R6)
        MOV         #12,R0
        FMOV.S      FR3,@(R0,R6)
        LDS         R1,FPUL
        FSCA        FPUL,DR0
        LDS         R2,FPUL
        FSCA        FPUL,DR2
        FMUL        FR1,FR1
        FMUL        FR3,FR3
        MOV         #16,R0
        FMOV.S      FR1,@(R0,R6)
        MOV         #20,R0
        FMOV.S      FR3,@(R0,R6)
        MOV         #16,R0
        FMOV.S      @(R0,R7),FR4
        MOV         #20,R0
        FMOV.S      @(R0,R7),FR3
        FSUB        FR3,FR4
        FNEG        FR4
        FLDI1       FR2
        FDIV        FR4,FR2
        FMUL        FR2,FR3
        MOV         #24,R0
        FMOV.S      FR2,@(R0,R6)
        MOV         #28,R0
        FMOV.S      FR3,@(R0,R6)
        MOV         #44,R0
        MOV.W       R2,@(R0,R6)
        ADD         #2,R0
        MOV.W       R1,@(R0,R6)
        MOV.L       #__nj_light_material_+48,R3
        FMOV.S      @R3,FR4
        FTRC        FR4,FPUL
        MOV         #40,R0
        STS         FPUL,R1
        MOV.L       R1,@(R0,R6)
        MOV         #36,R0
        FMOV.S      FR11,@(R0,R6)
        MOV.W       #H'0088,R2
        CMP/EQ      R2,R5
        BT          ?spot_rate
        MOV.W       #H'00C8,R1
        CMP/EQ      R1,R5
        BF          ?power_rate
?spot_rate:
        MOV         #20,R0
        FMOV.S      @(R0,R6),FR3
        ADD         #-4,R0
        FMOV.S      @(R0,R6),FR2
        FSUB        FR3,FR2
        FMUL        FR2,FR2
        FSRRA       FR2
        BRA         ?rate
        ADD         #16,R0
?power_rate:
        MOV         #40,R0
        MOV.L       @(R0,R6),R4
        ADD         #-24,R0
        BSR         _light_power
        FMOV.S      @(R0,R6),FR4
        FMOV.S      FR0,FR2
        FMUL        FR2,FR2
        FSRRA       FR2
        MOV         #32,R0
?rate:
        FMOV.S      FR2,@(R0,R6)
        MOV         #36,R0
        MOV         #84,R4
        MOV         #36,R5
        FMOV.S      @(R0,R6),FR4
        ADD         R7,R5
        BSR         _light_scale_color
        ADD         R6,R4
        MOV         #4,R0
        MOV         #84,R5
        MOV         #16,R4
        FMOV.S      @(R0,R7),FR4
        ADD         R6,R5
        BSR         _light_scale_color
        ADD         R5,R4
        MOV         #8,R0
        MOV         #84,R5
        MOV         #32,R4
        FMOV.S      @(R0,R7),FR4
        ADD         R6,R5
        BSR         _light_scale_color
        ADD         R5,R4
        MOV         #12,R0
        MOV         #84,R5
        MOV         #48,R4
        FMOV.S      @(R0,R7),FR4
        ADD         R6,R5
        BSR         _light_scale_color
        ADD         R5,R4
        MOV.L       #_njUnitMatrix,R2
        MOV         R14,R4
        LDS.L       @R15+,PR
        MOV.L       @R15+,R14
        JMP         @R2
        ADD         #0,R4
}
#endif
/* clang-format on */

void njCreateLight(NJS_LIGHT *l, Int type)
{
    create_light(l, type);
    /* @hack redundant: a never-run call of light_power, which keeps it in the
     * object: SHC leaves out a static function no C calls, and only
     * create_light's body calls it. without the call, asmsh stops at the
     * body's BSR _light_power, 200 (E) undefined symbol reference, so without
     * it, count unmeasured */
    if (0) {
        light_power(0.0F, 0);
    }
}
