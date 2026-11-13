/* njRotateZ on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njRotateZ.c */
#include <ninja.h>

static void __attribute__((noinline, cold)) turn_rows(NJS_MATRIX *m, Angle ang)
{
    zam_turn_rows(m, ZAM_Z, ang);
}

/* set m to R * m, where R rotates by ang about z. a null m selects the
 * current matrix, in XMTRX */
void njRotateZ(NJS_MATRIX *m, Angle ang)
{
    if (m == NULL) {
        zam_turn(ZAM_Z, ang);
    } else {
        turn_rows(m, ang);
    }
}
