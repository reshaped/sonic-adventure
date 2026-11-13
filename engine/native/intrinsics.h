/* SHC's intrinsics, declared for a compiler that lacks them. SHC builds each
 * in, and make dc's headers define each for GCC. the native slice's prelude
 * includes this header, and clang-tidy and census.py read it ahead of each
 * source */
#ifndef NATIVE_INTRINSICS_H
#define NATIVE_INTRINSICS_H

#include "intrinsics_machine.h"

/* math.h's and ninja.h's: fabsf, sqrtf, FSRRA and FSCA. FSCA takes an Angle, a
 * Sint32. the prelude defines Sint32 where it takes sg_xpt.h's guard. else
 * this header comes ahead of sg_xpt.h, whose Sint32 is a long */
#define _builtin_fabsf __builtin_fabsf
#define _builtin_sqrtf __builtin_sqrtf
float _builtin_fsrra(float n);
#ifdef SG_XPT_H
void _builtin_fsca(Sint32 ang, float *sin, float *cos);
#else
void _builtin_fsca(long ang, float *sin, float *cos);
#endif

#endif
