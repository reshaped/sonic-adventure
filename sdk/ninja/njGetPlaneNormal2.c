#include <ninja.h>

/* set *v to (p1 - p0) x (p2 - p0), not normalized */
void njGetPlaneNormal2(NJS_POINT3 *p0, NJS_POINT3 *p1, NJS_POINT3 *p2,
                       NJS_VECTOR *v)
{
    v->x = ((p1->y - p0->y) * (p2->z - p0->z)) -
           ((p1->z - p0->z) * (p2->y - p0->y));
    v->y = ((p1->z - p0->z) * (p2->x - p0->x)) -
           ((p1->x - p0->x) * (p2->z - p0->z));
    v->z = ((p1->x - p0->x) * (p2->y - p0->y)) -
           ((p1->y - p0->y) * (p2->x - p0->x));
}
