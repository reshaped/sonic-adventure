/* njDetMatrix and njClearMatrix against xbox/sdk/ninja/. njClearMatrix
 * calls the real njMultiMatrix and standins.h's njScale */
#define REAL_NJCLEARMATRIX
#include "standins.h"

typedef struct {
    float m[16];
    float rest[15];
} View;

__attribute__((aligned(32))) float _unit_matrix_[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                                                         0, 0, 1, 0, 0, 0, 0, 1};
__attribute__((aligned(32))) float _nj_view_matrix_[16];
static __attribute__((aligned(32))) float stack[4][16];
float (*_nj_base_matrix_ptr_)[16] = &stack[0];
float (*_nj_current_matrix_ptr_)[16] = &stack[2];
int _stack_size_ = 3;
View *_nj_view_;

float njDetMatrix(float *m);
void njClearMatrix(void);

static void put(const float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        harness_float(m[k]);
    }
}

int main(void)
{
    static View view __attribute__((aligned(8)));
    unsigned i;

    harness_fpu();
    _nj_view_ = &view;
    for (i = 0; i < 3000; i++) {
        float m[16];
        float x[16];
        int k;

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(10.0F);
            x[k] = harness_value(10.0F);
            view.m[k] = harness_value(3.0F);
            stack[0][k] = harness_value(9.0F);
        }
        harness_load_xmtrx(x);
        harness_begin("det", i);
        harness_float(njDetMatrix(m));
        harness_float(njDetMatrix(0));
        harness_end();
        _stack_size_ = 3;
        _nj_current_matrix_ptr_ = &stack[2];
        njClearMatrix();
        harness_store_xmtrx(x);
        harness_begin("clear", i);
        harness_hex((unsigned)_stack_size_);
        harness_hex((unsigned)(_nj_current_matrix_ptr_ - stack));
        put(stack[0]);
        put(_nj_view_matrix_);
        put(x);
        harness_end();
    }
    return 0;
}
