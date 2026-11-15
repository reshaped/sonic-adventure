/* njRotateZ: set m to R * m, where R rotates by ang about z, or the
 * current matrix, XMTRX, where m is null. make pc compiles this in place
 * of sdk/ninja/njRotateZ.c, whose body is SH-4 assembly: FSCA's sine s
 * and cosine c, and njrotate.h's two FTRVs, which turn rows or columns 0
 * and 1 */
#include <ninja.h>

#include "xbox/sdk/ninja/njrotate.h"

void(njRotateZ)(NJS_MATRIX *m, Angle ang)
{
    Float s;
    Float c;

    _builtin_fsca(ang, &s, &c);
    {
        const Float a[4] = {c, s, 0.0F, 0.0F};
        const Float b[4] = {-s, c, 0.0F, 0.0F};

        nj_rotate_axes(m, 0, 1, a, b);
    }
}
