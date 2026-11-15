/* the XMTRX routines of src/system/crt0.src and cache.src against
 * xbox/src/system/crt0.c and cache.c: point_calc, matrix_load with
 * point_calc_loaded, and matrix_load_parent with point_calc_loaded, each
 * with a matrix given and with Ninja's current one */
#include "harness.h"

typedef float Matrix[16];
typedef struct {
    float x, y, z;
} Point;

#ifndef __sh__
/* the port layer's, which stage 2 writes: the test runs no routine that
 * calls it */
void _builtin_set_imask(int mask)
{
    (void)mask;
}
#endif

static Matrix stack[4];
Matrix *_nj_current_matrix_ptr_ = &stack[2];

void point_calc(Matrix *matrix, Point *point, Point *out);
void matrix_load(Matrix *matrix);
void point_calc_loaded(Point *point, Point *out);
int matrix_load_parent(void);

static void point(Point *p)
{
    p->x = harness_value(500.0F);
    p->y = harness_value(500.0F);
    p->z = harness_value(500.0F);
}

static void put(const Point *p)
{
    harness_float(p->x);
    harness_float(p->y);
    harness_float(p->z);
}

int main(void)
{
    unsigned i;

    harness_fpu();
    for (i = 0; i < 2000; i++) {
        Matrix given;
        Point a;
        Point out;
        int k;

        for (k = 0; k < 16; k++) {
            given[k] = harness_value(3.0F);
            stack[1][k] = harness_value(3.0F);
            stack[2][k] = harness_value(3.0F);
        }
        point(&a);
        harness_begin("calc", i);
        point_calc(&given, &a, &out);
        put(&out);
        point_calc(0, &a, &out);
        put(&out);
        out = a;
        point_calc(&given, &out, &out); /* out the same as point */
        put(&out);
        harness_end();

        harness_begin("loaded", i);
        matrix_load((i % 2) ? &given : 0);
        point_calc_loaded(&a, &out);
        put(&out);
        point(&a);
        point_calc_loaded(&a, &out);
        put(&out);
        harness_end();

        harness_begin("parent", i);
        harness_hex((unsigned)matrix_load_parent());
        point_calc_loaded(&a, &out);
        put(&out);
        harness_end();
    }
    return 0;
}
