/* dust raised along a line between two points */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "stg05/dust_line.h"

void dust_create_line(NJS_POINT3 *from, NJS_POINT3 *to, Float scale,
                      Uint8 count)
{
    NJS_POINT3 pos;
    NJS_VECTOR line;
    NJS_VECTOR step;
    NJS_VECTOR speed;
    Uint8 i;

    line.x = to->x - from->x;
    line.y = 0.0F;
    line.z = to->z - from->z;
    step.x = line.x / count;
    step.y = 0.0F;
    step.z = line.z / count;
    /* the puffs blow out perpendicular to the line, all to the same side */
    speed.x = line.z;
    speed.y = 0.0F;
    speed.z = -line.x;
    njUnitVector(&speed);
    speed.x *= scale + 1.8F;
    speed.y = scale * 0.25F;
    speed.z *= scale + 1.8F;
    pos = *from;
    for (i = 0; i < count; i++) {
        dust_create(&pos, &speed, scale);
        pos.x += step.x;
        pos.z += step.z;
    }
}
