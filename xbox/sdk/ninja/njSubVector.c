/* njSubVector: subtract vs from vd. make pc compiles this in place of
 * sdk/ninja/njSubVector.c, whose body is SH-4 assembly. vs is read whole
 * before vd is written, so the two may be one */
#include <ninja.h>

void(njSubVector)(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
    Float x = vd->x - vs->x;
    Float y = vd->y - vs->y;
    Float z = vd->z - vs->z;

    vd->z = z;
    vd->y = y;
    vd->x = x;
}
