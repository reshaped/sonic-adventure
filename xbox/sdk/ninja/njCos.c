/* njCos: FSCA's cosine of n, of which only the low 16 bits count. make pc
 * compiles this in place of sdk/ninja/njCos.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njCos)(Angle n)
{
    Float sine;
    Float cosine;

    _builtin_fsca(n, &sine, &cosine);
    return cosine;
}
