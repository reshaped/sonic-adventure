#include <ninja.h>
#include <sg_xpt.h>

Float njDistanceP2P(NJS_POINT3 *p1, NJS_POINT3 *p2)
{
    return njSqrt(((p1->x - p2->x) * (p1->x - p2->x)) +
                  ((p1->y - p2->y) * (p1->y - p2->y)) +
                  ((p1->z - p2->z) * (p1->z - p2->z)));
}
