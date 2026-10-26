/* an event's request that Zero throw its hand at a point */
#include <ninja.h>
#include <sg_xpt.h>

#include "enemy/zero.h"
#include "shared/zero_grab.h"

Bool event_zero_grab(Float x, Float y, Float z)
{
    NJS_POINT3 pos;

    pos.x = x;
    pos.y = y;
    pos.z = z;
    return zero_event_grab(&pos);
}
