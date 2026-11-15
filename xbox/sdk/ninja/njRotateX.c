/* njRotateX: set m to R * m, where R rotates by ang about x, or the
 * current matrix, XMTRX, where m is null. make pc compiles this in place
 * of sdk/ninja/njRotateX.c, whose body is SH-4 assembly: FSCA's sine s
 * and cosine c, and njrotate.h's two FTRVs, which turn rows or columns 1
 * and 2 */
#include <ninja.h>

#include "xbox/sdk/ninja/njrotate.h"

void(njRotateX)(NJS_MATRIX *m, Angle ang)
{
    Float s;
    Float c;

    _builtin_fsca(ang, &s, &c);
    {
        const Float a[4] = {0.0F, c, s, 0.0F};
        const Float b[4] = {0.0F, -s, c, 0.0F};

        nj_rotate_axes(m, 1, 2, a, b);
    }
}
