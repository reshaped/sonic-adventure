/* C runtime double math, as far as the sources use it */
#ifndef MATH_H
#define MATH_H

double frexp(double value, int *exp);
double atan2(double y, double x);
double cos(double x);
/* sqrt is _builtin_sqrtf, SHC's float square root intrinsic, which is right
 * only where double is float. its callers are compiled with -fpu=single or
 * -double=float */
#ifdef __clang__
#define _builtin_sqrtf __builtin_sqrtf
#endif
#ifdef GCC
#define _builtin_sqrtf __builtin_sqrtf
/* C99's INFINITY and isinf, by GCC's builtins: SH4ZAM's shz_complex.inl.h
 * reads both */
#define INFINITY __builtin_inff()
#define isinf(x) __builtin_isinf(x)
#endif
#define sqrt _builtin_sqrtf

#endif
