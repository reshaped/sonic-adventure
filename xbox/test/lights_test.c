/* njLtSrcs.c's calculators, through its public entries: make gcc's split C
 * on the SH-4, its bodies through pairs.py and emulate.py, against make
 * pc's, whose bodies tools/sh2c.py translates. lights of every type the
 * queue serves, with random fields, each exponent within fpow_tbl's rows */
#include "harness.h"

typedef struct {
    float f[0x158 / 4];
} Light;
typedef struct {
    float a, r, g, b;
} Argb;
typedef struct {
    float x, y, z;
} Vector;

#define LSRC 0x124
#define EXP (0x60 + 40)

struct control {
    Light *light[32];
    int count;
};
struct queue {
    int light[32];
    int count;
};

struct control _nj_light_control_;
struct queue _nj_light_queue_;
float _nj_light_material_[13];
int _nj_depth_enable_;
float _nj_depth_near_ = 10.0F, _nj_depth_far_ = 400.0F, _nj_depth_range_ = 1.0F / 390.0F;

void njSetLightQueue(void);
void njCalculateLightsRGB(Argb *color, Vector *point, Vector *normal, int reverse);
void njCalculateLightsRGBVertex(Argb *color, Vector *point, Vector *normal, int reverse);
void njCalculateLightsRGBPacked(unsigned *color, Vector *point, Vector *normal, int reverse);
void njCalculateLightsRGBPackedVertex(unsigned *color, Vector *point, Vector *normal, int reverse);
float njLightPower(float value, int row);
float njLightSqrt(float value);
void njGetLightDistance(Vector *point);

/* a type 0x40 light's attr.func, which its calculator calls with color,
 * point, normal and the light */
#define FUNC 0x13c
static void user_light(Argb *color, Vector *point, Vector *normal, Light *light)
{
    color->r += point->x * 0.001F + light->f[0];
    color->g += normal->y * 0.5F;
}

static const unsigned types[] = {0x00, 0x01, 0x02, 0x82, 0x04, 0x84, 0xc4, 0x08, 0x88,
                                 0xc8, 0x40, 0x10, 0x90, 0x20, 0xa0, 0xe0, 0x03, 0x83,
                                 0x05, 0x85, 0xc5, 0x13, 0x93, 0x25, 0xa5, 0xe5};
static Light lights[32] __attribute__((aligned(8)));

static void vector(Vector *v, float range)
{
    v->x = harness_value(range);
    v->y = harness_value(range);
    v->z = harness_value(range);
}

int main(void)
{
    unsigned i;
    unsigned k;

    harness_fpu();
    for (k = 0; k < 13; k++) {
        _nj_light_material_[k] = harness_value(1.0F);
    }
    for (i = 0; i < 600; i++) {
        unsigned n = 1 + harness_random() % 4;
        Argb color[2];
        unsigned packed[2];
        Vector point;
        Vector normal;
        unsigned w;

        _nj_light_control_.count = 32;
        _nj_light_queue_.count = (int)n;
        for (k = 0; k < n; k++) {
            Light *l = &lights[k];

            for (w = 0; w < 0x158 / 4; w++) {
                l->f[w] = harness_value(w >= 0x90 / 4 && w < 0x9c / 4 ? 60.0F : 1.5F);
            }
            ((unsigned *)l->f)[LSRC / 4] = types[(i + k * 7) % 26];
            ((int *)l->f)[EXP / 4] = (int)(harness_random() % 17);
            ((void **)l->f)[FUNC / 4] = (void *)user_light;
            _nj_light_control_.light[k] = l;
            _nj_light_queue_.light[k] = (int)k;
        }
        njSetLightQueue();
        vector(&point, 50.0F);
        vector(&normal, 1.0F);
        _nj_depth_enable_ = (int)(i % 3 == 0);
        for (k = 0; k < 8; k++) {
            ((float *)color)[k] = 9.0F;
        }
        harness_begin("rgb", i);
        njCalculateLightsRGB(color, &point, &normal, (int)(i % 2));
        for (k = 0; k < 8; k++) {
            harness_float(((float *)color)[k]);
        }
        njCalculateLightsRGBVertex(color, &point, &normal, (int)(i % 2));
        for (k = 0; k < 8; k++) {
            harness_float(((float *)color)[k]);
        }
        njCalculateLightsRGBPacked(packed, &point, &normal, (int)(i % 2));
        harness_hex(packed[0]);
        harness_hex(packed[1]);
        njCalculateLightsRGBPackedVertex(packed, &point, &normal, (int)(i % 2));
        harness_hex(packed[0]);
        harness_hex(packed[1]);
        njGetLightDistance(&point);
        for (k = 0; k < n; k++) {
            for (w = 0x60 / 4; w < 0x124 / 4; w++) {
                harness_float(lights[k].f[w]);
            }
        }
        harness_end();
        /* within njLightPower's table: rows 0 to 16, values from 0 */
        harness_begin("power", i);
        harness_float(njLightPower(__builtin_fabsf(harness_value(1.2F)), (int)(harness_random() % 17)));
        harness_float(njLightSqrt(__builtin_fabsf(harness_value(4.0F))));
        harness_end();
    }
    return 0;
}
