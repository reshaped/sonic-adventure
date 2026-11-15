/* njRotateXYZ: set m to Rx * Ry * Rz * m, or the current matrix, XMTRX,
 * where m is null. make pc compiles this in place of
 * sdk/ninja/njRotateXYZ.c, whose body is SH-4 assembly: it runs
 * njRotateZ's, njRotateY's and njRotateX's bodies in that order, each only
 * where its angle isn't 0 */
#include <ninja.h>

void(njRotateXYZ)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
    if (angz != 0) {
        (njRotateZ)(m, angz);
    }
    if (angy != 0) {
        (njRotateY)(m, angy);
    }
    if (angx != 0) {
        (njRotateX)(m, angx);
    }
}
