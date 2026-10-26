#include <ninja.h>

void njRotateLightX(NJS_LIGHT *l, Angle ang)
{
    njRotateX(&l->mtrx, ang);
}
