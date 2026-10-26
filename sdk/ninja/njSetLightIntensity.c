#include <ninja.h>
#include <sg_xpt.h>

static void scale_color(NJS_ARGB *dst, NJS_ARGB *src, Float scale)
{
    dst->a = src->a * scale;
    dst->r = src->r * scale;
    dst->g = src->g * scale;
    dst->b = src->b * scale;
}

void njSetLightIntensity(NJS_LIGHT *l, Float spc, Float dif, Float amb)
{
    l->attr.ispc = spc;
    l->attr.idif = dif;
    l->attr.iamb = amb;
    scale_color(&l->ltcal.atten, &l->attr.argb, l->ltcal.intns);
    scale_color(&l->ltcal.spc, &l->ltcal.atten, l->attr.ispc);
    scale_color(&l->ltcal.dif, &l->ltcal.atten, l->attr.idif);
    scale_color(&l->ltcal.amb, &l->ltcal.atten, l->attr.iamb);
}
