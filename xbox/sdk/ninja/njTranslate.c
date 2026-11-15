/* njTranslate: m = T * m, T the translation by (x, y, z), or the current
 * matrix, XMTRX, where m is null. make pc compiles this in place of
 * sdk/ninja/njTranslate.c, whose body is SH-4 assembly (njtranslate.h) */
#include <ninja.h>

#include "xbox/sdk/ninja/njtranslate.h"

void(njTranslate)(NJS_MATRIX *m, Float x, Float y, Float z)
{
    nj_translate(m, x, y, z);
}
