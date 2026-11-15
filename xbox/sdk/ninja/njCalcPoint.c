/* njCalcPoint: pd = (ps, 1) times m, or the current matrix, XMTRX, where m
 * is null. make pc compiles this in place of sdk/ninja/njCalcPoint.c, whose
 * body is SH-4 assembly: FTRV, or a FIPR of (ps, 1) with each of m's first
 * three columns, each result stored after the next column's loads, as the
 * body orders them. ps is read whole first, so pd may be ps */
#include <ninja.h>

#include "xbox/include/sh4.h"

void(njCalcPoint)(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd)
{
    const Float v[4] = {ps->x, ps->y, ps->z, 1.0F};

    if (m == NULL) {
        Float out[4];

        sh4_xmtrx_ftrv(v, out);
        pd->z = out[2];
        pd->y = out[1];
        pd->x = out[0];
        return;
    }
    {
        const Float *c = (const Float *)m;
        Float x = sh4_fipr(v[0], v[1], v[2], v[3], c[0], c[4], c[8], c[12]);
        Float c1[4] = {c[1], c[5], c[9], c[13]};
        Float y;
        Float c2[4];
        Float z;

        pd->x = x;
        y = sh4_fipr(v[0], v[1], v[2], v[3], c1[0], c1[1], c1[2], c1[3]);
        c2[0] = c[2];
        c2[1] = c[6];
        c2[2] = c[10];
        c2[3] = c[14];
        pd->y = y;
        z = sh4_fipr(v[0], v[1], v[2], v[3], c2[0], c2[1], c2[2], c2[3]);
        pd->z = z;
    }
}
