#include <ninja.h>

void njTranslateLightV(NJS_LIGHT *l, NJS_VECTOR *v)
{
    njTranslateV(&l->mtrx, v);
}
