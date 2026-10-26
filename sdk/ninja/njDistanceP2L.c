#include <ninja.h>
#include <sg_xpt.h>

/* cp, if not null, receives the point of the line nearest to p */
Float njDistanceP2L(NJS_POINT3 *p, NJS_LINE *l, NJS_POINT3 *cp)
{
    /* @hack redundant: register on t, which puts p in R14 and l in R13
     * (0x8c622426, 0x8c62242a); without it, the two trade and 41 of 133 words
     * differ */
    register Float t;

    t = ((l->vx * (p->x - l->px)) + (l->vy * (p->y - l->py)) +
         (l->vz * (p->z - l->pz))) /
        ((l->vx * l->vx) + (l->vy * l->vy) + (l->vz * l->vz));
    if (cp) {
        cp->x = (l->vx * t) + l->px;
        cp->y = (l->vy * t) + l->py;
        cp->z = (l->vz * t) + l->pz;
        return njSqrt(((p->x - cp->x) * (p->x - cp->x)) +
                      ((p->y - cp->y) * (p->y - cp->y)) +
                      ((p->z - cp->z) * (p->z - cp->z)));
    }
    return njSqrt(
        ((p->x - (l->vx * t) - l->px) * (p->x - (l->vx * t) - l->px)) +
        ((p->y - (l->vy * t) - l->py) * (p->y - (l->vy * t) - l->py)) +
        ((p->z - (l->vz * t) - l->pz) * (p->z - (l->vz * t) - l->pz)));
}
