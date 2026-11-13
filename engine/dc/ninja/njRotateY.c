/* njRotateY on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njRotateY.c */
#include <ninja.h>

static void __attribute__((noinline, cold)) turn_rows(NJS_MATRIX *m, Angle ang)
{
    zam_turn_rows(m, ZAM_Y, ang);
}

/* set m to R * m, where R rotates by ang about y. a null m selects the
 * current matrix, in XMTRX */
void njRotateY(NJS_MATRIX *m, Angle ang)
{
    if (m == NULL) {
        zam_turn(ZAM_Y, ang);
    } else {
        turn_rows(m, ang);
    }
}
