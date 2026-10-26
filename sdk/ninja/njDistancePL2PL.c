#include <ninja.h>
#include <sg_xpt.h>

/* the distance between pl1 and pl2 if they are parallel, else 0. l, if not
 * null, receives the line where they meet, or all zeros if they are parallel */
Float njDistancePL2PL(NJS_PLANE *pl1, NJS_PLANE *pl2, NJS_LINE *l)
{
    NJS_POINT3 p;
    Float d1;
    Float d2;

    if (njIsParalellPL2PL(pl1, pl2)) {
        p.x = pl1->px;
        p.y = pl1->py;
        p.z = pl1->pz;
        if (l) {
            l->px = 0.0F;
            l->py = 0.0F;
            l->pz = 0.0F;
            l->vx = 0.0F;
            l->vy = 0.0F;
            l->vz = 0.0F;
        }
        return njDistanceP2PL(&p, pl2, NULL);
    }
    if (l) {
        /* the line runs along the cross product of the normals */
        l->vx = (pl1->vy * pl2->vz) - (pl2->vy * pl1->vz);
        l->vy = (pl1->vz * pl2->vx) - (pl2->vz * pl1->vx);
        l->vz = (pl1->vx * pl2->vy) - (pl2->vx * pl1->vy);
        d1 = (-pl1->vx * pl1->px) - (pl1->vy * pl1->py) - (pl1->vz * pl1->pz);
        d2 = (-pl2->vx * pl2->px) - (pl2->vy * pl2->py) - (pl2->vz * pl2->pz);
        /* the line's point is where it crosses z = 0 for a line along the z
         * axis, y = 0 for one along the y axis, and x = 0 for any other */
        if (l->vx == 0.0F && l->vy == 0.0F) {
            l->px = ((pl1->vy * d2) - (pl2->vy * d1)) /
                    ((pl1->vx * pl2->vy) - (pl2->vx * pl1->vy));
            l->py = ((pl1->vx * d2) - (pl2->vx * d1)) /
                    ((pl2->vx * pl1->vy) - (pl1->vx * pl2->vy));
            l->pz = 0.0F;
        } else if (l->vx == 0.0F && l->vz == 0.0F) {
            l->px = ((pl1->vz * d2) - (pl2->vz * d1)) /
                    ((pl1->vx * pl2->vz) - (pl2->vx * pl1->vz));
            l->py = 0.0F;
            l->pz = ((pl1->vx * d2) - (pl2->vx * d1)) /
                    ((pl2->vx * pl1->vz) - (pl1->vx * pl2->vz));
        } else {
            l->px = 0.0F;
            /* @bug arithmetic: a line parallel to the plane x = 0 but to
             * neither the y nor the z axis, as one along (0, 1, 1), has vx 0
             * and takes this arm, whose divisors for py and pz, -vx and vx,
             * are then 0 (FDIV at 0x8c6227e8 and 0x8c62280e) */
            l->py = ((pl2->vz * d1) - (pl1->vz * d2)) /
                    ((pl2->vy * pl1->vz) - (pl1->vy * pl2->vz));
            l->pz = ((pl2->vy * d1) - (pl1->vy * d2)) /
                    ((pl1->vy * pl2->vz) - (pl2->vy * pl1->vz));
        }
    }
    return 0.0F;
}
