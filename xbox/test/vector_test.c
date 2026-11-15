/* vector.src against xbox/src/system/vector.c, with out the same as an
 * input in a third of the cases, as vector.h allows */
#include "harness.h"

typedef struct {
    float x, y, z;
} Vector;

void vector_sum(Vector *a, Vector *b, Vector *out);
void vector_diff(Vector *a, Vector *b, Vector *out);
float vector_dot_from(Vector *from, Vector *a, Vector *b);

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
    for (i = 0; i < 3000; i++) {
        float range = (i % 2) ? 1.0F : 4000.0F;
        Vector a, b, c, out;
        Vector *target = (i % 3 == 0) ? &a : (i % 3 == 1) ? &b : &out;

        vector(&a, range);
        vector(&b, range);
        vector(&c, range);
        harness_begin("dot", i);
        harness_float(vector_dot_from(&c, &a, &b));
        harness_end();
        vector_sum(&a, &b, target);
        harness_begin("sum", i);
        put(target);
        harness_end();
        vector_diff(&a, &c, target);
        harness_begin("diff", i);
        put(target);
        harness_end();
    }
    return 0;
}
