#include <ninja.h>

void njRotateLightY(NJS_LIGHT *l, Angle ang)
{
    njRotateY(&l->mtrx, ang);
}
