/* njCreateLight: add l to the lights njCreateLight has created, and set it
 * up as ninja.h says, as a light of type. make pc compiles this in place of
 * sdk/ninja/njCreateLight.c, whose create_light and light_power bodies are
 * SH-4 assembly. the C runs their stores and float operations in their
 * order, with the constants their pools hold, among them SHC's truncated
 * 0.3 and 0.2, H'3E999999 and H'3E4CCCCC. each call names its function in
 * parentheses, so that it reaches the function, as the bodies' JSRs do, and
 * not a macro of its name */
#include <stddef.h>

#include <ninja.h>
#include <sg_xpt.h>

#include "xbox/include/sh4.h"

/* the offsets the body writes through &l->ltcal and &l->attr */
_Static_assert(offsetof(NJS_LIGHT, stat) == 88, "stat");
_Static_assert(offsetof(NJS_LIGHT, ltcal) == 0x60, "ltcal");
_Static_assert(offsetof(NJS_LIGHT, attr) == 0x124, "attr");
_Static_assert(offsetof(NJS_LIGHT_CAL, intns) == 36, "intns");
_Static_assert(offsetof(NJS_LIGHT_CAL, exp) == 40, "exp");
_Static_assert(offsetof(NJS_LIGHT_CAL, reserve) == 44, "reserve");
_Static_assert(offsetof(NJS_LIGHT_CAL, atten) == 84, "atten");
_Static_assert(offsetof(NJS_LIGHT_ATTR, argb) == 36, "argb");

/* the spotlights, whose cone gives ltcal.rate */
#define LIGHT_SPOT_A 0x88
#define LIGHT_SPOT_B 0xc8
/* the cone's angles a new light takes */
#define INNER_ANGLE 0x071c
#define OUTER_ANGLE 0x1555

static Float bits_float(Uint32 bits)
{
    union {
        Uint32 bits;
        Float value;
    } value = {bits};

    return value.value;
}

/* value to the specular power step stands for, as NJS_LIGHT_CAL.exp lists
 * them, by light_power's chains: an even step squares value and returns
 * it, an odd one squares it and then cubes it, and a step past 16, read
 * unsigned, is 16's */
static Float light_power(Float value, Int step)
{
    Uint32 at = (Uint32)step;
    int squares;

    if (at >= 17) {
        at = 16;
    }
    if (at == 0) {
        return 1.0F;
    }
    if (at == 1) {
        return __builtin_sqrtf(value);
    }
    if (at == 3) {
        return value * __builtin_sqrtf(value);
    }
    if (at % 2 == 0) {
        for (squares = (int)at / 2 - 1; squares > 0; squares--) {
            value *= value;
        }
        return value;
    }
    for (squares = (int)(at - 5) / 2; squares > 0; squares--) {
        value *= value;
    }
    return value * value * value;
}

/* output gets input's a, r, g and b times scale, all read before any is
 * written */
void light_scale_color(NJS_ARGB *output, const NJS_ARGB *input, Float scale)
{
    Float a = input->a * scale;
    Float r = input->r * scale;
    Float g = input->g * scale;
    Float b = input->b * scale;

    output->a = a;
    output->r = r;
    output->g = g;
    output->b = b;
}

void(njCreateLight)(NJS_LIGHT *l, Int type)
{
    NJS_LIGHT_CAL *cal = &l->ltcal;
    NJS_LIGHT_ATTR *attr = &l->attr;
    Int count = _nj_light_control_.count;
    Float s;
    Float c;
    Float inverse;

    if (count >= 32) {
        return;
    }
    _nj_light_control_.light[count] = l;
    _nj_light_control_.count = count + 1;
    l->stat = 1;
    l->vctr.z = -1.0F;
    l->vctr.y = 0.0F;
    l->vctr.x = 0.0F;
    l->pnt.z = 0.0F;
    l->pnt.y = 0.0F;
    l->pnt.x = 0.0F;
    attr->argb.b = 1.0F;
    attr->argb.g = 1.0F;
    attr->argb.r = 1.0F;
    attr->argb.a = 0.0F;
    attr->oang = OUTER_ANGLE;
    attr->iang = INNER_ANGLE;
    attr->frang = bits_float(0x477fff00);
    attr->nrang = 1.0F;
    attr->ispc = bits_float(0x3e999999);
    attr->idif = 1.0F;
    attr->iamb = bits_float(0x3e4ccccc);
    attr->lsrc = type;
    cal->lvctr.z = -1.0F;
    cal->lvctr.y = 0.0F;
    cal->lvctr.x = 0.0F;
    cal->lpnt.z = 0.0F;
    cal->lpnt.y = 0.0F;
    cal->lpnt.x = 0.0F;
    cal->nrr = attr->nrang * attr->nrang;
    cal->frr = attr->frang * attr->frang;
    _builtin_fsca(INNER_ANGLE, &s, &c);
    cal->cosi = c * c;
    _builtin_fsca(OUTER_ANGLE, &s, &c);
    cal->cose = c * c;
    inverse = 1.0F / -(attr->nrang - attr->frang);
    cal->idev = inverse;
    cal->odev = attr->frang * inverse;
    /* the body's two halfword stores, the outer angle low and the inner
     * angle high */
    cal->reserve = (Int)((Uint32)INNER_ANGLE << 16 | OUTER_ANGLE);
    cal->exp = sh4_ftrc(((const Float *)&_nj_light_material_)[12]);
    cal->intns = 1.0F;
    if (type == LIGHT_SPOT_A || type == LIGHT_SPOT_B) {
        Float spread = cal->cosi - cal->cose;

        cal->rate = _builtin_fsrra(spread * spread);
    } else {
        Float power = light_power(cal->cosi, cal->exp);

        cal->rate = _builtin_fsrra(power * power);
    }
    light_scale_color(&cal->atten, &attr->argb, cal->intns);
    light_scale_color(&cal->amb, &cal->atten, attr->iamb);
    light_scale_color(&cal->dif, &cal->atten, attr->idif);
    light_scale_color(&cal->spc, &cal->atten, attr->ispc);
    (njUnitMatrix)(&l->mtrx);
}
