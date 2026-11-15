/* njSetMatrix: md = ms, or the current matrix, XMTRX, = ms where md is
 * null. make pc compiles this in place of sdk/ninja/njSetMatrix.c, whose
 * body is SH-4 assembly. it copies to memory a half at a time, ms's first 8
 * floats then its last 8, as the body does */
#include <ninja.h>

void(njSetMatrix)(NJS_MATRIX *md, NJS_MATRIX *ms)
{
    const Float *in = (const Float *)ms;
    Float *out = (Float *)md;
    Float half[8];
    int k;

    if (md == NULL) {
        sh4_xmtrx_set(in);
        return;
    }
    for (k = 0; k < 8; k++) {
        half[k] = in[k];
    }
    for (k = 0; k < 8; k++) {
        out[k] = half[k];
    }
    for (k = 0; k < 8; k++) {
        half[k] = in[8 + k];
    }
    for (k = 0; k < 8; k++) {
        out[8 + k] = half[k];
    }
}
