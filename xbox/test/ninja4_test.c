/* Ninja's combined rotations against xbox/sdk/ninja/: njRotateXYZ,
 * njRotateZXY and njRotate, of a matrix and of XMTRX, with angles of 0
 * among them and njRotate's axes with zero parts */
#include "harness.h"

typedef float Matrix[16];
typedef struct {
    float x, y, z;
} Vector;

void njRotateXYZ(Matrix *m, int angx, int angy, int angz);
void njRotateZXY(Matrix *m, int angx, int angy, int angz);
void njRotate(Matrix *m, Vector *v, int ang);

/* the axes the body treats apart: units along each axis, both ways, and
 * vectors with one or two parts 0 */
static const Vector specials[] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
    {0, 0, 0}, {0.6F, 0.8F, 0}, {0.6F, 0, -0.8F}, {0, -0.6F, 0.8F},
    {-0.6F, 0.8F, 0}, {0.6F, 0, 0.8F},
};

static void put(const float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        harness_float(m[k]);
    }
}

static int angle(void)
{
    unsigned pick = harness_random();

    return (pick % 5 == 0) ? 0 : (int)(pick >> 12) - 0x80000;
}

int main(void)
{
    unsigned i;

    harness_fpu();
    for (i = 0; i < 3000; i++) {
        Matrix m;
        Matrix x;
        Vector v;
        int k;
        int ax = angle();
        int ay = angle();
        int az = angle();

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(50.0F);
            x[k] = harness_value(50.0F);
        }
        if (i % 4 == 0) {
            v = specials[(i / 4) % (sizeof specials / sizeof specials[0])];
        } else {
            float length;

            v.x = harness_value(1.0F);
            v.y = harness_value(1.0F);
            v.z = harness_value(1.0F);
            length = __builtin_sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
            if (length > 0.0F) {
                v.x /= length;
                v.y /= length;
                v.z /= length;
            }
        }
        harness_load_xmtrx(x);
        harness_begin("xyz", i);
        njRotateXYZ(&m, ax, ay, az);
        put(m);
        njRotateXYZ(0, ax, ay, az);
        harness_store_xmtrx(x);
        put(x);
        harness_end();
        harness_begin("zxy", i);
        njRotateZXY(&m, ax, ay, az);
        put(m);
        njRotateZXY(0, ax, ay, az);
        harness_store_xmtrx(x);
        put(x);
        harness_end();
        harness_begin("axis", i);
        njRotate(&m, &v, ax);
        harness_store_xmtrx(x);
        put(m);
        put(x);
        njRotate(0, &v, ay);
        harness_store_xmtrx(x);
        put(x);
        harness_end();
    }
    return 0;
}
