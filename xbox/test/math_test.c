/* math.src against xbox/src/system/math.c: each call's result, as hex, for
 * the same inputs on both. point_calc_xy runs with a matrix in memory alone,
 * since XMTRX holds no value the two runs share */
#include "harness.h"

typedef int Angle;
typedef struct {
    float x, y, z;
} Vector;
typedef struct {
    float a, r, g, b;
} Argb;
typedef struct {
    float dist, w, h, cx, cy;
} Screen;

/* the names math.src takes from the game, which the harness defines */
Screen _nj_screen_ = {500.0F, 640.0F, 480.0F, 320.0F, 240.0F};
float _nj_scr_aspect_x_ = 1.0F;
float _nj_scr_aspect_y_ = 1.125F;
#ifdef __sh__
void njExecuteFade(void) {}
void njFadeEnable(void) {}
void njFadeDisable(void) {}
void njSetFadeColor(void) {}
void syCfgInit(void) {}
void syCfgExit(void) {}
void syCfgSetSoundMode(void) {}
void syCfgGetSoundMode(void) {}
#endif

float angle_sin_over_abs_cos(Angle ang);
float vector_inner_product(Vector *a, Vector *b);
float vector_inner_product_across(Vector *a, Vector *b);
void vector_outer_product(Vector *a, Vector *b, Vector *out);
float vector_magnitude(Vector *v);
float vector_magnitude_across(Vector *v);
float vector_magnitude_squared(Vector *v);
float vector_magnitude_squared_across(Vector *v);
float vector_unit(Vector *v, Vector *out);
float vector_unit_across(Vector *v, Vector *out);
float vector_cosine(Vector *a, Vector *b);
float point_distance(Vector *from, Vector *to);
float point_distance_across(Vector *from, Vector *to);
float point_distance_squared(Vector *from, Vector *to);
float point_distance_squared_across(Vector *from, Vector *to);
Angle angle_arc_tan2(float y, float x);
void point_project_centered(Vector *pos, Vector *out);
float float_mod(float x, float y);
Angle angle_arc_sin(float sine);
Angle angle_arc_cos(float cosine);
void point_calc_xy(float *matrix, Vector *point, float *out);
void color_to_argb(unsigned color, Argb *argb);
unsigned color_from_argb(Argb *argb);
unsigned color_multiply(unsigned color, unsigned by);
void color_to_argb_unit(unsigned color, Argb *argb);
unsigned color_from_argb_unit(Argb *argb);
void fill_bytes(void *mem, int value, int count);
void fill_blocks(void *mem, unsigned value, int size);

#define CASES 2000

static void vector(Vector *v, float range)
{
    v->x = harness_value(range);
    v->y = harness_value(range);
    v->z = harness_value(range);
}

static void put_vector(const Vector *v)
{
    harness_float(v->x);
    harness_float(v->y);
    harness_float(v->z);
}

static void put_argb(const Argb *c)
{
    harness_float(c->a);
    harness_float(c->r);
    harness_float(c->g);
    harness_float(c->b);
}

int main(void)
{
    unsigned i;

    harness_fpu();
    for (i = 0; i < CASES; i++) {
        Vector a;
        Vector b;
        Vector out;
        float range = (i % 3 == 0) ? 1.0F : (i % 3 == 1) ? 100.0F : 5000.0F;

        vector(&a, range);
        vector(&b, range);
        /* a nonzero vector, for the units */
        if (a.x == 0.0F && a.y == 0.0F && a.z == 0.0F) {
            a.x = 0.5F;
        }
        if (b.x == 0.0F && b.y == 0.0F && b.z == 0.0F) {
            b.z = -2.0F;
        }

        harness_begin("inner", i);
        harness_float(vector_inner_product(&a, &b));
        harness_float(vector_inner_product_across(&a, &b));
        harness_end();

        out.x = out.y = out.z = 7.0F;
        vector_outer_product(&a, &b, &out);
        harness_begin("outer", i);
        put_vector(&out);
        harness_end();

        harness_begin("magnitude", i);
        harness_float(vector_magnitude(&a));
        harness_float(vector_magnitude_across(&a));
        harness_float(vector_magnitude_squared(&a));
        harness_float(vector_magnitude_squared_across(&a));
        harness_end();

        out.y = 3.0F;
        harness_begin("unit", i);
        harness_float(vector_unit(&a, &out));
        put_vector(&out);
        out.y = 3.0F;
        harness_float(vector_unit_across(&b, &out));
        put_vector(&out);
        harness_float(vector_cosine(&a, &b));
        harness_end();

        harness_begin("distance", i);
        harness_float(point_distance(&a, &b));
        harness_float(point_distance_across(&a, &b));
        harness_float(point_distance_squared(&a, &b));
        harness_float(point_distance_squared_across(&a, &b));
        harness_end();

        harness_begin("atan2", i);
        harness_hex((unsigned)angle_arc_tan2(a.y, a.x));
        harness_hex((unsigned)angle_arc_tan2(b.z, b.x));
        harness_hex((unsigned)angle_arc_tan2(a.y * 0.0F, a.x * 0.0F));
        harness_end();

        {
            float sine = harness_value(1.0F);

            harness_begin("arcsin", i);
            harness_hex((unsigned)angle_arc_sin(sine));
            harness_hex((unsigned)angle_arc_cos(sine));
            harness_end();
        }

        harness_begin("sincos", i);
        harness_float(angle_sin_over_abs_cos((Angle)(harness_random() >> 8)));
        harness_end();

        if (a.z == 0.0F) {
            a.z = 10.0F;
        }
        harness_begin("project", i);
        point_project_centered(&a, &out);
        put_vector(&out);
        harness_end();

        harness_begin("mod", i);
        harness_float(float_mod(a.x, b.y));
        harness_float(float_mod(b.x * 37.0F, a.y));
        harness_end();

        {
            float matrix[16];
            float xy[2];
            int k;

            for (k = 0; k < 16; k++) {
                matrix[k] = harness_value(range);
            }
            point_calc_xy(matrix, &a, xy);
            harness_begin("calc_xy", i);
            harness_float(xy[0]);
            harness_float(xy[1]);
            harness_end();
        }

        {
            unsigned color = harness_random();
            unsigned by = harness_random();
            Argb argb;

            harness_begin("color", i);
            color_to_argb(color, &argb);
            put_argb(&argb);
            harness_hex(color_from_argb(&argb));
            harness_hex(color_multiply(color, by));
            color_to_argb_unit(color, &argb);
            put_argb(&argb);
            harness_hex(color_from_argb_unit(&argb));
            argb.a = harness_value(300.0F);
            argb.r = harness_value(1.5F);
            argb.g = harness_value(1.0F);
            argb.b = harness_value(0.5F);
            harness_hex(color_from_argb(&argb));
            harness_hex(color_from_argb_unit(&argb));
            harness_end();
        }
    }

    {
        static unsigned char bytes[96];
        static unsigned blocks[24];
        unsigned k;

        for (k = 0; k < sizeof bytes; k++) {
            bytes[k] = 0xee;
        }
        fill_bytes(bytes + 5, 0x1234, 77);
        fill_blocks(blocks, 0xdeadbeef, 64);
        harness_begin("fill", 0);
        for (k = 0; k < sizeof bytes; k += 4) {
            harness_hex((unsigned)bytes[k] | (unsigned)bytes[k + 1] << 8 |
                        (unsigned)bytes[k + 2] << 16 |
                        (unsigned)bytes[k + 3] << 24);
        }
        harness_end();
        harness_begin("blocks", 0);
        for (k = 0; k < 24; k++) {
            harness_hex(blocks[k]);
        }
        harness_end();
    }
    return 0;
}
