/* njTranslateV: njTranslate by v. make pc compiles this in place of
 * sdk/ninja/njTranslateV.c, whose body is SH-4 assembly (njtranslate.h).
 * v is read whole first, as the body loads it */
#include <ninja.h>

#include "xbox/sdk/ninja/njtranslate.h"

void(njTranslateV)(NJS_MATRIX *m, NJS_VECTOR *v)
{
    nj_translate(m, v->x, v->y, v->z);
}
