/* njRotateXYZ on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njRotateXYZ.c */
#include <ninja.h>

/* turn m in memory. it is out of line so that the null m's path keeps no
 * frame */
static void __attribute__((noinline)) turn_rows(NJS_MATRIX *m, Angle angx,
                                                Angle angy, Angle angz)
{
    if (angz != 0) {
        njRotateZ(m, angz);
    }
    if (angy != 0) {
        njRotateY(m, angy);
    }
    if (angx != 0) {
        njRotateX(m, angx);
    }
}

/* set m to Rx * Ry * Rz * m, where Rx rotates by angx about x, Ry by angy
 * about y and Rz by angz about z. a null m selects the current matrix, in
 * XMTRX. a turn by an angle of 0 is left out, as in the shipped body */
void njRotateXYZ(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
    if (m != NULL) {
        turn_rows(m, angx, angy, angz);
        return;
    }
    if (angz != 0) {
        zam_turn(ZAM_Z, angz);
    }
    if (angy != 0) {
        zam_turn(ZAM_Y, angy);
    }
    if (angx != 0) {
        zam_turn(ZAM_X, angx);
    }
}
