#include <ninja.h>
#include <sg_xpt.h>

void njSetLightLocation(NJS_LIGHT *l, Float x, Float y, Float z)
{
    l->pnt.x = x;
    l->pnt.y = y;
    l->pnt.z = z;
    l->ltcal.lpnt = l->pnt;
}
