/* njGetMatrix on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njGetMatrix.c */
#include <ninja.h>

/* copy the current matrix, in XMTRX, to m. the store tests m's alignment as
 * it runs, and moves pairs only where m is 8-byte aligned */
void njGetMatrix(NJS_MATRIX *m)
{
    shz_xmtrx_store_unaligned_4x4(*m);
}
