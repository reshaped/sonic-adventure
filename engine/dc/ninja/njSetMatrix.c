/* njSetMatrix on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njSetMatrix.c */
#include <ninja.h>

/* md = ms. a null md selects the current matrix, in XMTRX. its load tests
 * ms's alignment as it runs, as njPushMatrix's does (ninja_zam.h). the copy
 * to md moves words */
void njSetMatrix(NJS_MATRIX *md, NJS_MATRIX *ms)
{
    if (md == NULL) {
        shz_xmtrx_load_unaligned_4x4(*ms);
    } else {
        zam_copy_matrix(*md, *ms);
    }
}
