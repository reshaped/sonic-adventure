/* njTranslate's and njTranslateV's shared body: m = T * m, T the
 * translation by (x, y, z), or the current matrix, XMTRX, where m is null.
 * the bodies run an FTRV of (x, y, z, 1) by XMTRX into its last column, or
 * with m in memory a FIPR of (x, y, z, 1) with each of m's columns into
 * m[12] to m[15], in order */
#ifndef XBOX_NJTRANSLATE_H
#define XBOX_NJTRANSLATE_H
#include <ninja.h>

#include "xbox/include/sh4.h"

static inline void nj_translate(NJS_MATRIX *m, Float x, Float y, Float z)
{
    Float *c = (Float *)m;
    int k;

    if (m == NULL) {
        const Float by[4] = {x, y, z, 1.0F};

        sh4_xmtrx_ftrv(by, &sh4_xmtrx[12]);
        return;
    }
    for (k = 0; k < 4; k++) {
        c[12 + k] = sh4_fipr(x, y, z, 1.0F, c[k], c[4 + k], c[8 + k], c[12 + k]);
    }
}

#endif
