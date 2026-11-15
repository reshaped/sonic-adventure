/* njGetMatrix: copy the current matrix, XMTRX, to m, XF0 to XF15 in order.
 * make pc compiles this in place of sdk/ninja/njGetMatrix.c, whose body is
 * SH-4 assembly. XMTRX is the port's (xbox/include/sh4.h) */
#include <ninja.h>

void(njGetMatrix)(NJS_MATRIX *m)
{
    sh4_xmtrx_get((Float *)m);
}
