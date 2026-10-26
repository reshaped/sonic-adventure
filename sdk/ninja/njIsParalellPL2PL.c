#include <ninja.h>
#include <sg_xpt.h>

#define SQUARE(n) ((n) * (n))

/* true when pl1 is parallel to pl2: when the cross product of their normals
 * has a squared length of exactly 0 */
Bool njIsParalellPL2PL(NJS_PLANE *pl1, NJS_PLANE *pl2)
{
    if (SQUARE((pl1->vx * pl2->vy) - (pl1->vy * pl2->vx)) +
            SQUARE((pl1->vy * pl2->vz) - (pl1->vz * pl2->vy)) +
            SQUARE((pl1->vz * pl2->vx) - (pl1->vx * pl2->vz)) ==
        0.0F) {
        return true;
    }
    return false;
}
