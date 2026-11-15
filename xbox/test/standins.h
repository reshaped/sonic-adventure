/* stand-ins for the Ninja and runtime calls a test's functions make, the
 * same C on both runs, so that a test checks its functions' use of them
 * rather than their own results. each touches the matrix it's given, or
 * XMTRX where it's null, by a deterministic sum. a test that links the
 * real njClearMatrix defines REAL_NJCLEARMATRIX first */
#ifndef XBOX_TEST_STANDINS_H
#define XBOX_TEST_STANDINS_H
#include "harness.h"

static void standin_touch(float *m, float a, float b, float c)
{
    float x[16];
    float *at = m != 0 ? m : x;
    int k;

    if (m == 0) {
        harness_store_xmtrx(x);
    }
    for (k = 0; k < 16; k++) {
        at[k] = at[k] * 0.5F + a * (float)(k + 1) - b + c * 0.25F;
    }
    if (m == 0) {
        harness_load_xmtrx(x);
    }
}

int standin_cleared;

void njTranslate(float *m, float x, float y, float z)
{
    standin_touch(m, x, y, z);
}

void njScale(float *m, float x, float y, float z)
{
    standin_touch(m, x * 3.0F, y, -z);
}

#ifndef REAL_NJCLEARMATRIX
void njClearMatrix(void)
{
    standin_cleared++;
}
#endif

float asinf(float x)
{
    return x * 0.875F + 0.0625F;
}

float acosf(float x)
{
    return 1.5F - x * 0.75F;
}

float njTan(int n)
{
    return (float)n * 0.0001F + 0.25F;
}

/* the vector made a unit vector, roughly, and its length */
float njUnitVector(float *v)
{
    float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + 0.5F;

    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
    return length;
}

#endif
