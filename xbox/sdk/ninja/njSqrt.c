/* njSqrt: sqrt(n), or -sqrt(-n) for a negative n; njInvertSqrt: FSRRA's
 * 1 / sqrt(n), or -1 / sqrt(-n) for a negative n. make pc compiles this in
 * place of sdk/ninja/njSqrt.c, whose two bodies are SH-4 assembly: FCMP/GT
 * of n with 0, which a NaN and -0 fail, so both take the positive path */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njSqrt)(Float n)
{
    if (0.0F > n) {
        return -__builtin_sqrtf(__builtin_fabsf(n));
    }
    return __builtin_sqrtf(n);
}

Float(njInvertSqrt)(Float n)
{
    if (0.0F > n) {
        return -_builtin_fsrra(__builtin_fabsf(n));
    }
    return _builtin_fsrra(n);
}
