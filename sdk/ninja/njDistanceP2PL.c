#include <ninja.h>
#include <sg_xpt.h>

/* cp, if not null, receives the point of the plane nearest to p */
Float njDistanceP2PL(NJS_POINT3 *p, NJS_PLANE *pl, NJS_POINT3 *cp)
{
    /* @hack redundant: register on all four, which puts pl->vx and pl->vz in
     * FR6 and FR7 and d in FR4 (0x8c622538, 0x8c622550, 0x8c62255a); without
     * it, 13 of 85 words differ */
    register Float d;
    register Float side;
    register Float len2;
    register Float t;

    d = (-pl->vx * pl->px) - (pl->vy * pl->py) - (pl->vz * pl->pz);
    side = (pl->vx * p->x) + (pl->vy * p->y) + (pl->vz * p->z) + d;
    len2 = (pl->vx * pl->vx) + (pl->vy * pl->vy) + (pl->vz * pl->vz);
    if (cp) {
        t = -side / len2;
        cp->x = (pl->vx * t) + p->x;
        cp->y = (pl->vy * t) + p->y;
        cp->z = (pl->vz * t) + p->z;
    }
    return njAbs(side) * njInvertSqrt(len2);
}
