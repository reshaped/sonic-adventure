/* C runtime float math, as far as the sources use it */
#ifndef MATHF_H
#define MATHF_H

float acosf(float x);
float asinf(float x);
float atan2f(float y, float x);
float ceilf(float x);
float floorf(float x);
float modff(float value, float *iptr);
float powf(float x, float y);
#ifdef __clang__
float sqrtf(float x);
#else
#define sqrtf _builtin_sqrtf
#endif

#endif
