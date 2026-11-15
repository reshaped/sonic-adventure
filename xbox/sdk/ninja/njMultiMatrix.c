/* njMultiMatrix: md = ms * md, or the current matrix, XMTRX, = ms * XMTRX
 * where md is null. make pc compiles this in place of
 * sdk/ninja/njMultiMatrix.c, whose body is SH-4 assembly: with XMTRX, an
 * FTRV of each of ms's rows by XMTRX, which becomes that column of XMTRX;
 * with md, a FIPR of each of ms's rows with each of md's columns. ms is
 * read whole before md is written, so the two may overlap */
#include <ninja.h>

#include "xbox/include/sh4.h"

void(njMultiMatrix)(NJS_MATRIX *md, NJS_MATRIX *ms)
{
    const Float *s = (const Float *)ms;
    Float out[16];
    int r;
    int c;

    if (md == NULL) {
        Float xf[16];

        sh4_xmtrx_get(xf);
        for (r = 0; r < 4; r++) {
            sh4_ftrv(xf, s + 4 * r, out + 4 * r);
        }
        sh4_xmtrx_set(out);
        return;
    }
    {
        Float *d = (Float *)md;

        for (c = 0; c < 4; c++) {
            for (r = 0; r < 4; r++) {
                out[4 * r + c] = sh4_fipr(d[c], d[4 + c], d[8 + c], d[12 + c], s[4 * r],
                                          s[4 * r + 1], s[4 * r + 2], s[4 * r + 3]);
            }
        }
        for (r = 0; r < 16; r++) {
            d[r] = out[r];
        }
    }
}

/* the original's empty njMultiMatrix_old */
void njMultiMatrix_old(void)
{
}
