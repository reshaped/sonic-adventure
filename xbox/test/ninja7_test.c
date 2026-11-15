/* njMultiMatrix and njMirror against xbox/sdk/ninja/, of a matrix and of
 * XMTRX. njMirror calls the real njRotateX and njRotateY, and standins.h's
 * njTranslate, njScale and asinf */
#include "standins.h"

typedef struct {
    float px, py, pz, vx, vy, vz;
} Plane;

void njMultiMatrix(float *md, float *ms);
void njMirror(float *m, Plane *pl);

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
    for (i = 0; i < 3000; i++) {
        float a[16];
        float b[16];
        float x[16];
        Plane pl;
        float length;
        int k;

        for (k = 0; k < 16; k++) {
            a[k] = harness_value(20.0F);
            b[k] = harness_value(20.0F);
            x[k] = harness_value(20.0F);
        }
        harness_load_xmtrx(x);
        harness_begin("multi", i);
        njMultiMatrix(a, b);
        put(a);
        njMultiMatrix(a, a);
        put(a);
        njMultiMatrix(0, b);
        harness_store_xmtrx(x);
        put(x);
        harness_end();

        pl.px = harness_value(100.0F);
        pl.py = harness_value(100.0F);
        pl.pz = harness_value(100.0F);
        pl.vx = harness_value(1.0F);
        pl.vy = harness_value(1.0F);
        pl.vz = harness_value(1.0F);
        if (pl.vz < 0.0F) {
            pl.vz = -pl.vz;
        }
        length = __builtin_sqrtf(pl.vx * pl.vx + pl.vy * pl.vy + pl.vz * pl.vz);
        if (length > 0.0F) {
            pl.vx /= length;
            pl.vy /= length;
            pl.vz /= length;
        }
        harness_begin("mirror", i);
        njMirror(a, &pl);
        harness_store_xmtrx(x);
        put(a);
        put(x);
        njMirror(0, &pl);
        harness_store_xmtrx(x);
        put(x);
        harness_end();
    }
    return 0;
}
