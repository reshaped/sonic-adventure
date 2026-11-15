/* njSetView and njInitView against xbox/sdk/ninja/. njSetView calls the
 * real njRotateX, njRotateY and njRotateZ, and standins.h's njTranslate,
 * acosf and njClearMatrix */
#include "standins.h"

typedef struct {
    float m[16];
    float px, py, pz;
    float vx, vy, vz;
    int roll;
    float apx, apy, apz;
    float avx, avy, avz;
    int aroll;
} View;

__attribute__((aligned(32))) float _unit_matrix_[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                                                         0, 0, 1, 0, 0, 0, 0, 1};
View *_nj_view_;

void njSetView(View *v);
void njInitView(View *v);

static void put(const View *v)
{
    const unsigned *at = (const unsigned *)v;
    unsigned k;

    for (k = 0; k < sizeof(View) / 4; k++) {
        harness_hex(at[k]);
    }
}

int main(void)
{
    static View view __attribute__((aligned(8)));
    unsigned i;

    harness_fpu();
    for (i = 0; i < 3000; i++) {
        float *at = (float *)&view;
        unsigned k;

        for (k = 0; k < sizeof(View) / 4; k++) {
            at[k] = harness_value(100.0F);
        }
        view.roll = (int)(harness_random() >> 8);
        if (i % 7 == 0) {
            view.vx = 0.0F;
        }
        if (i % 11 == 0) {
            view.vy = -view.vy;
        }
        _nj_view_ = 0;
        standin_cleared = 0;
        njSetView(&view);
        harness_begin("set", i);
        put(&view);
        harness_hex(_nj_view_ == &view);
        harness_hex((unsigned)standin_cleared);
        harness_end();
        njInitView(&view);
        harness_begin("init", i);
        put(&view);
        harness_end();
    }
    return 0;
}
