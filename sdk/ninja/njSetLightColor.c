#include <ninja.h>
#include <sg_xpt.h>

static void scale_color(NJS_ARGB *dst, NJS_ARGB *src, Float scale)
{
    dst->a = src->a * scale;
    dst->r = src->r * scale;
    dst->g = src->g * scale;
    dst->b = src->b * scale;
}

void njSetLightColor(NJS_LIGHT *l, Float r, Float g, Float b)
{
    l->attr.argb.r = r;
    l->attr.argb.g = g;
    l->attr.argb.b = b;
    scale_color(&l->ltcal.atten, &l->attr.argb, l->ltcal.intns);
    scale_color(&l->ltcal.spc, &l->ltcal.atten, l->attr.ispc);
    scale_color(&l->ltcal.dif, &l->ltcal.atten, l->attr.idif);
    scale_color(&l->ltcal.amb, &l->ltcal.atten, l->attr.iamb);
}
