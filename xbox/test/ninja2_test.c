/* Ninja's matrix basics against xbox/sdk/ninja/: njUnitMatrix,
 * njSetMatrix and njInitMatrix, to memory and to XMTRX */
#include "harness.h"

typedef float Matrix[16];

/* njGlobal.c's, which the harness defines on both runs */
__attribute__((aligned(32))) Matrix _unit_matrix_ = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
int _stack_size_;
int _max_stack_size_;
int _nj_matrix_flag_;
Matrix *_nj_current_matrix_ptr_;
Matrix *_nj_base_matrix_ptr_;

void njUnitMatrix(Matrix *m);
void njSetMatrix(Matrix *md, Matrix *ms);
void njInitMatrix(Matrix *m, int n, int flag);

static void put(const float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        harness_float(m[k]);
    }
}

int main(void)
{
    static Matrix stack[8];
    unsigned i;

    harness_fpu();
    for (i = 0; i < 500; i++) {
        Matrix a;
        Matrix b;
        Matrix x;
        int k;

        for (k = 0; k < 16; k++) {
            a[k] = harness_value(100.0F);
            b[k] = harness_value(100.0F);
            x[k] = harness_value(100.0F);
        }
        harness_load_xmtrx(x);
        harness_begin("unit", i);
        njUnitMatrix(&a);
        put(a);
        njUnitMatrix(0);
        harness_store_xmtrx(x);
        put(x);
        harness_end();

        harness_begin("set", i);
        njSetMatrix(&a, &b);
        put(a);
        for (k = 0; k < 16; k++) {
            b[k] = harness_value(100.0F);
        }
        njSetMatrix(0, &b);
        harness_store_xmtrx(x);
        put(x);
        njSetMatrix(&a, &a);
        put(a);
        harness_end();

        for (k = 0; k < 8 * 16; k++) {
            stack[k / 16][k % 16] = harness_value(9.0F);
        }
        njInitMatrix(&stack[0], 8, (int)i);
        harness_store_xmtrx(x);
        harness_begin("init", i);
        harness_hex((unsigned)_stack_size_);
        harness_hex((unsigned)_max_stack_size_);
        harness_hex((unsigned)_nj_matrix_flag_);
        harness_hex((unsigned)(_nj_current_matrix_ptr_ - stack));
        harness_hex((unsigned)(_nj_base_matrix_ptr_ - stack));
        put(stack[0]);
        put(stack[1]);
        put(x);
        harness_end();
    }
    return 0;
}
