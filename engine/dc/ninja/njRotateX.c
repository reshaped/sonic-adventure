/* njRotateX on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njRotateX.c */
#include <ninja.h>

static void __attribute__((noinline, cold)) turn_rows(NJS_MATRIX *m, Angle ang)
{
    zam_turn_rows(m, ZAM_X, ang);
}

/* set m to R * m, where R rotates by ang about x. a null m selects the
 * current matrix, in XMTRX */
void njRotateX(NJS_MATRIX *m, Angle ang)
{
    if (m == NULL) {
        zam_turn(ZAM_X, ang);
    } else {
        turn_rows(m, ang);
    }
}
