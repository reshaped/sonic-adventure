/* njProjectScreen: project p3, transformed by m or the current matrix,
 * XMTRX, where m is null, onto the screen into p2. make pc compiles this in
 * place of sdk/ninja/njProjectScreen.c, whose body is SH-4 assembly: FTRV
 * of (p3, 1), or a FIPR of it with each of m's first three columns, then
 * each of x and y times the screen's distance, the reciprocal of z, and
 * its aspect, plus its center, in that order */
#include <ninja.h>

#include "xbox/include/sh4.h"

void(njProjectScreen)(NJS_MATRIX *m, NJS_POINT3 *p3, NJS_POINT2 *p2)
{
    const Float v[4] = {p3->x, p3->y, p3->z, 1.0F};
    Float at[4];
    Float inverse;

    if (m == NULL) {
        Float xf[16];

        sh4_xmtrx_get(xf);
        sh4_ftrv(xf, v, at);
    } else {
        const Float *c = (const Float *)m;

        at[0] = sh4_fipr(v[0], v[1], v[2], v[3], c[0], c[4], c[8], c[12]);
        at[1] = sh4_fipr(v[0], v[1], v[2], v[3], c[1], c[5], c[9], c[13]);
        at[2] = sh4_fipr(v[0], v[1], v[2], v[3], c[2], c[6], c[10], c[14]);
    }
    inverse = 1.0F / at[2];
    p2->x = _nj_screen_.dist * at[0] * inverse * _nj_scr_aspect_x_ + _nj_screen_.cx;
    p2->y = _nj_screen_.dist * at[1] * inverse * _nj_scr_aspect_y_ + _nj_screen_.cy;
}
