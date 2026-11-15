/* njTan: FSCA's sine of n over its cosine, by FDIV. make pc compiles this
 * in place of sdk/ninja/njTan.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njTan)(Angle n)
{
    Float sine;
    Float cosine;

    _builtin_fsca(n, &sine, &cosine);
    return sine / cosine;
}
