/* the single-axis rotations' shared body, as njRotateX, njRotateY and
 * njRotateZ's SH-4 assembly runs it: two FTRVs, of a and b, that turn
 * columns p and q of XMTRX. where m is a matrix, the body puts m's rows p
 * and q in those columns first, writes the FTRVs to those rows, and leaves
 * XMTRX as it was. the FTRVs' other columns are XMTRX's, which a and b's
 * zeros multiply, as the console's do */
#ifndef XBOX_NJROTATE_H
#define XBOX_NJROTATE_H
#include <ninja.h>

#include "xbox/include/sh4.h"

static inline void nj_rotate_axes(NJS_MATRIX *m, int p, int q, const Float a[4],
                                  const Float b[4])
{
    Float xf[16];
    Float *rows = (Float *)m;
    Float *at = xf;
    Float turned[4];
    int k;

    sh4_xmtrx_get(xf);
    if (m != NULL) {
        for (k = 0; k < 4; k++) {
            xf[4 * p + k] = rows[4 * p + k];
            xf[4 * q + k] = rows[4 * q + k];
        }
        at = rows;
    }
    sh4_ftrv(xf, a, turned);
    sh4_ftrv(xf, b, at + 4 * q);
    for (k = 0; k < 4; k++) {
        at[4 * p + k] = turned[k];
    }
    if (m == NULL) {
        sh4_xmtrx_set(xf);
    }
}

#endif
