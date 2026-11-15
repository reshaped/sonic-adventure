/* the light system's queue: njSetLightMaterials, njCalcLightMatrices,
 * njResetQueue and njSetLightSystem. make pc compiles this in place of
 * sdk/ninja/njLight.c, whose first two bodies are SH-4 assembly. each walks
 * the queue from its last light to its first, as the bodies do. each call
 * names its function in parentheses, so that it reaches the function, as
 * the bodies' JSRs do, and not a macro of its name */
#include <stddef.h>

#include <ninja.h>

/* the light types whose cone gives ltcal.rate */
#define LIGHT_SPOT_A 0x88
#define LIGHT_SPOT_B 0xc8

_Static_assert(offsetof(NJS_LIGHT, ltcal.rate) == 0x80, "rate");
_Static_assert(offsetof(NJS_LIGHT, attr.lsrc) == 0x124, "lsrc");
_Static_assert(sizeof(NJS_LIGHT_MATERIAL) >= 52, "the material's 13 floats");

void njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd);
void njMemCopy4(void *dst, const void *src, Sint32 count);

/* each queued light's mtrx times _nj_view_matrix_, in turn, as the body
 * reserves it in bss */
static __attribute__((aligned(32))) NJS_MATRIX light_to_view_matrix;

/* the light queue's lights, last first */
static NJS_LIGHT *queued(Int k)
{
    return _nj_light_control_.light[_nj_light_queue_.light[k]];
}

void(njSetLightMaterials)(void)
{
    static const Float material[13] = {1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 0.5F,
                                       0.5F, 1.0F, 0.5F, 0.5F, 0.5F, 6.0F};
    Float *out = (Float *)&_nj_light_material_;
    Int k;

    for (k = 12; k >= 0; k--) {
        out[k] = material[k];
    }
    for (k = _nj_light_queue_.count - 1; k >= 0; k--) {
        NJS_LIGHT *light = queued(k);

        if (light->attr.lsrc == LIGHT_SPOT_A || light->attr.lsrc == LIGHT_SPOT_B) {
            Float spread = light->ltcal.cosi - light->ltcal.cose;

            light->ltcal.rate = _builtin_fsrra(spread * spread);
        }
    }
}

void(njCalcLightMatrices)(void)
{
    Int k;

    for (k = _nj_light_queue_.count - 1; k >= 0; k--) {
        NJS_LIGHT *light = queued(k);

        (njMemCopy4)(&light_to_view_matrix, &_nj_view_matrix_, 16);
        (njMultiMatrix)(&light_to_view_matrix, &light->mtrx);
        (njCalcPoint)(&light_to_view_matrix, &light->pnt, &light->ltcal.lpnt);
        (njCalcVector)(&light_to_view_matrix, &light->vctr, &light->ltcal.lvctr);
    }
}

void njResetQueue(void)
{
    _nj_light_queue_.count = 0;
}

void njSetLightSystem(void)
{
}
