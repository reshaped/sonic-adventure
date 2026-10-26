#include <ninja.h>
#include <sg_xpt.h>

void njTranslateLight(NJS_LIGHT *l, Float x, Float y, Float z)
{
    njTranslate(&l->mtrx, x, y, z);
}
