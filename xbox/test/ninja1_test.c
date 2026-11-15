/* Ninja's first batch against xbox/sdk/ninja/: njAddVector, njSubVector,
 * njOuterProduct, njFraction, njRoundOff and njGetMatrix. modff and
 * floorf are stand-ins defined here, the same on both runs */
#include "harness.h"

typedef struct {
    float x, y, z;
} Vector;

void njAddVector(Vector *vd, Vector *vs);
void njSubVector(Vector *vd, Vector *vs);
float njOuterProduct(Vector *v1, Vector *v2, Vector *ov);
float njFraction(float n);
float njRoundOff(float n);
void njGetMatrix(float *m);
extern float FracInt;

#ifdef __sh__
float FracInt;
#endif

/* the stand-ins: whole parts by FTRC, as the runtime's would give them for
 * the values the test passes */
float modff(float value, float *iptr)
{
    float whole = (float)(int)value;

    *iptr = whole;
    return value - whole;
}

float floorf(float x)
{
    float whole = (float)(int)x;

    return whole > x ? whole - 1.0F : whole;
}

static void vector(Vector *v, float range)
{
    v->x = harness_value(range);
    v->y = harness_value(range);
    v->z = harness_value(range);
}

static void put(const Vector *v)
{
    harness_float(v->x);
    harness_float(v->y);
    harness_float(v->z);
}

int main(void)
{
    unsigned i;

    harness_fpu();
    for (i = 0; i < 4000; i++) {
        float range = (i % 3 == 0) ? 1.0F : (i % 3 == 1) ? 300.0F : 100000.0F;
        Vector a;
        Vector b;
        Vector out;
        float m[16];
        float got[16];
        int k;

        vector(&a, range);
        vector(&b, range);
        harness_begin("add", i);
        out = a;
        njAddVector(&out, &b);
        put(&out);
        njAddVector(&out, &out);
        put(&out);
        njSubVector(&out, &a);
        put(&out);
        njSubVector(&out, &out);
        put(&out);
        harness_end();

        harness_begin("outer", i);
        harness_float(njOuterProduct(&a, &b, &out));
        put(&out);
        out = a;
        harness_float(njOuterProduct(&out, &b, &out));
        put(&out);
        harness_end();

        harness_begin("round", i);
        harness_float(njFraction(a.x));
        harness_float(FracInt);
        harness_float(njRoundOff(a.y));
        harness_float(njRoundOff(-a.y));
        harness_end();

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(range);
            got[k] = 0.0F;
        }
        harness_load_xmtrx(m);
        njGetMatrix(got);
        harness_begin("get", i);
        for (k = 0; k < 16; k++) {
            harness_float(got[k]);
        }
        harness_end();
    }
    return 0;
}
