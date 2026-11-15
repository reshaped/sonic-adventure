/* njCalcVector: vd = (vs, 0) times m, or the current matrix, XMTRX, where
 * m is null, made unit length by FSRRA while _nj_matrix_flag_ is set. make
 * pc compiles this in place of sdk/ninja/njCalcVector.c, whose body is SH-4
 * assembly: FTRV, or a FIPR of (vs, 0) with each of m's first three
 * columns. vs is read whole first, so vd may be vs */
#include <ninja.h>

#include "xbox/include/sh4.h"

void(njCalcVector)(NJS_MATRIX *m, NJS_VECTOR *vs, NJS_VECTOR *vd)
{
    const Float v[4] = {vs->x, vs->y, vs->z, 0.0F};
    Float out[4];

    if (m == NULL) {
        Float xf[16];

        sh4_xmtrx_get(xf);
        sh4_ftrv(xf, v, out);
    } else {
        const Float *c = (const Float *)m;

        out[0] = sh4_fipr(v[0], v[1], v[2], v[3], c[0], c[4], c[8], 0.0F);
        out[1] = sh4_fipr(v[0], v[1], v[2], v[3], c[1], c[5], c[9], 0.0F);
        out[2] = sh4_fipr(v[0], v[1], v[2], v[3], c[2], c[6], c[10], 0.0F);
    }
    if (_nj_matrix_flag_ != 0) {
        Float scale = _builtin_fsrra(
            sh4_fipr(out[0], out[1], out[2], 0.0F, out[0], out[1], out[2], 0.0F));

        out[2] *= scale;
        out[1] *= scale;
        out[0] *= scale;
    }
    vd->z = out[2];
    vd->y = out[1];
    vd->x = out[0];
}
