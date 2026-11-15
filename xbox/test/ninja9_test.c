/* Ninja's lights against xbox/sdk/ninja/: njSetLightAngle,
 * njSetLightRange, njSetLightMaterials and njCalcLightMatrices, over a
 * queue of lights. njCalcLightMatrices calls the real njMultiMatrix and
 * njCalcVector, and stand-ins for njMemCopy4 and njCalcPoint */
#include "harness.h"

/* NJS_LIGHT, by its bytes: 0x158 of them */
typedef struct {
    float f[0x158 / 4];
} Light;

#define AT(l, offset) ((l)->f[(offset) / 4])
#define LSRC 0x124

struct control {
    Light *light[32];
    int count;
};
struct queue {
    int light[32];
    int count;
};

float _nj_light_material_[13];
struct control _nj_light_control_;
struct queue _nj_light_queue_;
__attribute__((aligned(32))) float _nj_view_matrix_[16];
int _nj_matrix_flag_ = 1;

void njSetLightAngle(Light *l, int iang, int oang);
void njSetLightRange(Light *l, float nrang, float frang);
void njSetLightMaterials(void);
void njCalcLightMatrices(void);

void njMemCopy4(void *dst, const void *src, int count)
{
    unsigned *d = dst;
    const unsigned *s = src;

    while (count-- > 0) {
        *d++ = *s++;
    }
}

void njCalcPoint(float *m, float *ps, float *pd)
{
    pd[0] = ps[0] * m[0] + ps[1] * m[4] + ps[2] * m[8] + m[12];
    pd[1] = ps[0] * m[1] + ps[1] * m[5] + ps[2] * m[9] + m[13];
    pd[2] = ps[0] * m[2] + ps[1] * m[6] + ps[2] * m[10] + m[14];
}

static Light lights[8] __attribute__((aligned(8)));

int main(void)
{
    static const unsigned kinds[] = {0x88, 0xc8, 0x10, 0x48, 0x88, 0x00};
    unsigned i;

    harness_fpu();
    for (i = 0; i < 1500; i++) {
        int k;
        unsigned w;

        for (k = 0; k < 8; k++) {
            for (w = 0; w < 0x158 / 4; w++) {
                lights[k].f[w] = harness_value(5.0F);
            }
            ((unsigned *)lights[k].f)[LSRC / 4] = kinds[(i + (unsigned)k) % 6];
            _nj_light_control_.light[k] = &lights[k];
        }
        for (k = 0; k < 16; k++) {
            _nj_view_matrix_[k] = harness_value(3.0F);
        }
        _nj_light_queue_.count = (int)(harness_random() % 9);
        for (k = 0; k < _nj_light_queue_.count; k++) {
            _nj_light_queue_.light[k] = (int)(harness_random() % 8);
        }
        njSetLightAngle(&lights[i % 8], (int)harness_random(), (int)harness_random());
        njSetLightRange(&lights[(i + 3) % 8], harness_value(50.0F), harness_value(500.0F));
        njSetLightMaterials();
        njCalcLightMatrices();
        harness_begin("lights", i);
        for (k = 0; k < 13; k++) {
            harness_float(_nj_light_material_[k]);
        }
        for (k = 0; k < 8; k++) {
            for (w = 0; w < 0x158 / 4; w++) {
                harness_float(lights[k].f[w]);
            }
        }
        harness_end();
    }
    return 0;
}
