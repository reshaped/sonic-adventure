/* njGetTranslation: p = m's translation, m[12] to m[14], or the current
 * matrix's, XF12 to XF14, where m is null. make pc compiles this in place
 * of sdk/ninja/njGetTranslation.c, whose body is SH-4 assembly. the three
 * are read first and stored z first, as the body stores them */
#include <ninja.h>

#include "xbox/include/sh4.h"

void(njGetTranslation)(NJS_MATRIX *m, NJS_POINT3 *p)
{
    const Float *t = m != NULL ? &(*m)[12] : &sh4_xmtrx[12];
    Float x = t[0];
    Float y = t[1];
    Float z = t[2];

    p->z = z;
    p->y = y;
    p->x = x;
}
