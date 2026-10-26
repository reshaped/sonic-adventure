#include <ninja.h>
#include <sg_xpt.h>

Int njCollisionCheckSS(NJS_SPHERE *sphere1, NJS_SPHERE *sphere2)
{
    Float distance;

    distance =
        njSqrt(((sphere1->c.x - sphere2->c.x) * (sphere1->c.x - sphere2->c.x)) +
               ((sphere1->c.y - sphere2->c.y) * (sphere1->c.y - sphere2->c.y)) +
               ((sphere1->c.z - sphere2->c.z) * (sphere1->c.z - sphere2->c.z)));
    return sphere1->r + sphere2->r > distance;
}
