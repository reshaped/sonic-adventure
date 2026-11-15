/* njRotateZXY: set m to Rz * Rx * Ry * m, or the current matrix, XMTRX,
 * where m is null. make pc compiles this in place of
 * sdk/ninja/njRotateZXY.c, whose body is SH-4 assembly: it runs
 * njRotateY's, njRotateX's and njRotateZ's bodies in that order, each only
 * where its angle isn't 0 */
#include <ninja.h>

void(njRotateZXY)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
    if (angy != 0) {
        (njRotateY)(m, angy);
    }
    if (angx != 0) {
        (njRotateX)(m, angx);
    }
    if (angz != 0) {
        (njRotateZ)(m, angz);
    }
}
