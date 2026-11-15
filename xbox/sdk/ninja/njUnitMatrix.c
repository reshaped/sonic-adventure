/* njUnitMatrix: set m to the unit matrix, or the current matrix, XMTRX,
 * where m is null. make pc compiles this in place of
 * sdk/ninja/njUnitMatrix.c, whose body is SH-4 assembly. XMTRX is the
 * port's (xbox/include/sh4.h) */
#include <ninja.h>

void(njUnitMatrix)(NJS_MATRIX *m)
{
    const Float *unit = (const Float *)&_unit_matrix_;
    Float *out = (Float *)m;
    int k;

    if (m == NULL) {
        sh4_xmtrx_set(unit);
        return;
    }
    for (k = 0; k < 16; k++) {
        out[k] = unit[k];
    }
}
