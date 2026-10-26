#include <ninja.h>
#include <sg_xpt.h>

void njSetLightDirection(NJS_LIGHT *l, Float x, Float y, Float z)
{
    l->vctr.x = x;
    l->vctr.y = y;
    l->vctr.z = z;
    njUnitVector(&l->vctr);
    l->ltcal.lvctr = l->vctr;
}
