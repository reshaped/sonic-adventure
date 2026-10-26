#include <ninja.h>
#include <sg_xpt.h>

Float njDistanceL2PL(NJS_LINE *l, NJS_PLANE *pl, NJS_POINT3 *cp)
{
    NJS_POINT3 p;
    /* @hack redundant: register on all four, which puts pl in R14 and l in
     * R13 (0x8c622342, 0x8c62234a), and d and side in FR4 (0x8c6223a6,
     * 0x8c6223ca); without it, 46 of 107 words differ */
    register Float d;
    register Float side;
    register Float slope;
    register Float t;

    /* a parallel line is as far from the plane as any point on it */
    if (njIsParalellL2PL(l, pl)) {
        p.x = l->px;
        p.y = l->py;
        p.z = l->pz;
        return njDistanceP2PL(&p, pl, cp);
    }
    /* any other line crosses the plane, so the distance is 0. cp, if not null,
     * receives the crossing point */
    if (cp != NULL) {
        d = (-pl->vx * pl->px) - (pl->vy * pl->py) - (pl->vz * pl->pz);
        side = (pl->vx * l->px) + (pl->vy * l->py) + (pl->vz * l->pz) + d;
        slope = (pl->vx * l->vx) + (pl->vy * l->vy) + (pl->vz * l->vz);
        t = -side / slope;
        cp->x = (l->vx * t) + l->px;
        cp->y = (l->vy * t) + l->py;
        cp->z = (l->vz * t) + l->pz;
    }
    return 0.0F;
}
