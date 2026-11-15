/* Ninja's single-axis rotations against xbox/sdk/ninja/: njRotateX,
 * njRotateY and njRotateZ, of a matrix and of XMTRX, with XMTRX and the
 * matrix checked after each */
#include "harness.h"

typedef float Matrix[16];

void njRotateX(Matrix *m, int ang);
void njRotateY(Matrix *m, int ang);
void njRotateZ(Matrix *m, int ang);

static void put(const float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        harness_float(m[k]);
    }
}

int main(void)
{
    static void (*const rotations[3])(Matrix *, int) = {njRotateX, njRotateY, njRotateZ};
    unsigned i;

    harness_fpu();
    for (i = 0; i < 3000; i++) {
        Matrix m;
        Matrix x;
        int k;
        int ang = (int)(harness_random() >> 8) - 0x800000;

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(50.0F);
            x[k] = harness_value(50.0F);
        }
        harness_load_xmtrx(x);
        rotations[i % 3](&m, ang);
        harness_store_xmtrx(x);
        harness_begin("memory", i);
        put(m);
        put(x);
        harness_end();
        rotations[i % 3](0, ang);
        harness_store_xmtrx(x);
        harness_begin("xmtrx", i);
        put(x);
        harness_end();
    }
    return 0;
}
