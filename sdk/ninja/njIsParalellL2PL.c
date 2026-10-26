#include <ninja.h>
#include <sg_xpt.h>

/* true when l is parallel to pl: when the dot product of l's direction and pl's
 * normal is exactly 0 */
Bool njIsParalellL2PL(NJS_LINE *l, NJS_PLANE *pl)
{
    if ((l->vx * pl->vx) + (l->vy * pl->vy) + (l->vz * pl->vz) == 0.0F) {
        return true;
    }
    return false;
}
