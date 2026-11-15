/* njInvertMatrix against xbox/sdk/ninja/njInvertMatrix.c, of a matrix and
 * of XMTRX, with a determinant of 0 in some cases. njDetMatrix is a
 * stand-in, the same on both runs */
#include "harness.h"

int njInvertMatrix(float *m);

static int zero_next;

float njDetMatrix(float *m)
{
    float x[16];
    const float *at = m;

    if (zero_next) {
        return 0.0F;
    }
    if (m == 0) {
        harness_store_xmtrx(x);
        at = x;
    }
    return at[0] * at[5] - at[1] * at[4] + at[10] * 0.5F + 0.75F;
}

static void put(const float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        harness_float(m[k]);
    }
}

int main(void)
{
    unsigned i;

    harness_fpu();
    for (i = 0; i < 4000; i++) {
        float m[16];
        float x[16];
        int k;

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(10.0F);
            x[k] = harness_value(10.0F);
        }
        zero_next = i % 9 == 0;
        harness_load_xmtrx(x);
        harness_begin("invert", i);
        harness_hex((unsigned)njInvertMatrix(m));
        put(m);
        harness_hex((unsigned)njInvertMatrix(0));
        harness_store_xmtrx(x);
        put(x);
        harness_end();
    }
    return 0;
}
