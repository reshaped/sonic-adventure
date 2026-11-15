/* njScale: m = S * m, S the scale by (sx, sy, sz), or the current matrix,
 * XMTRX, where m is null. make pc compiles this in place of
 * sdk/ninja/njScale.c, whose body is SH-4 assembly (njscale.h) */
#include <ninja.h>

#include "xbox/sdk/ninja/njscale.h"

void(njScale)(NJS_MATRIX *m, Float sx, Float sy, Float sz)
{
    nj_scale(m, sx, sy, sz);
}
