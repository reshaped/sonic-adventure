/* njCreateLight against xbox/sdk/ninja/njCreateLight.c: lights of each
 * type, past the 32 the table holds, with the material's exponent as
 * each step. it calls the real njUnitMatrix */
#include "harness.h"

typedef struct {
    float f[0x158 / 4];
} Light;

struct control {
    Light *light[32];
    int count;
};

struct control _nj_light_control_;
float _nj_light_material_[13];
__attribute__((aligned(32))) float _unit_matrix_[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                                                         0, 0, 1, 0, 0, 0, 0, 1};

void njCreateLight(Light *l, int type);

static Light lights[40] __attribute__((aligned(8)));

int main(void)
{
    static const int kinds[] = {0x88, 0xc8, 0x10, 0x48, 0x08, 0x00, 0xc0};
    unsigned i;

    harness_fpu();
    for (i = 0; i < 40; i++) {
        unsigned w;

        for (w = 0; w < 0x158 / 4; w++) {
            lights[i].f[w] = harness_value(5.0F);
        }
        /* each power step, and some past them */
        _nj_light_material_[12] = (float)(i % 20) - 1.0F;
        njCreateLight(&lights[i], kinds[i % 7]);
        harness_begin("light", i);
        harness_hex((unsigned)_nj_light_control_.count);
        for (w = 0; w < 0x158 / 4; w++) {
            harness_float(lights[i].f[w]);
        }
        harness_end();
    }
    harness_begin("table", 0);
    for (i = 0; i < 32; i++) {
        harness_hex((unsigned)(_nj_light_control_.light[i] - lights));
    }
    harness_end();
    return 0;
}
