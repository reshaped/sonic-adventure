/* njScale's and njScaleV's shared body: m = S * m, S the scale by (sx, sy,
 * sz), or the current matrix, XMTRX, where m is null: the first three
 * columns of XMTRX, or the first three rows of m, times sx, sy and sz. the
 * fourth is left as it is */
#ifndef XBOX_NJSCALE_H
#define XBOX_NJSCALE_H
#include <ninja.h>

#include "xbox/include/sh4.h"

static inline void nj_scale(NJS_MATRIX *m, Float sx, Float sy, Float sz)
{
    /* each four products one MULPS, whose lanes round as the body's FMULs
     * (sh4_sse.h) */
    sh4_v4u *c = (sh4_v4u *)(m != NULL ? (Float *)m : sh4_xmtrx);

    c[0] = c[0] * sx;
    c[1] = c[1] * sy;
    c[2] = c[2] * sz;
}

#endif
