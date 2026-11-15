/* Ninja's njCalcVector, njClipZ and njProjectScreen against
 * xbox/sdk/ninja/, of a matrix and of XMTRX */
#include "harness.h"

typedef struct {
    float x, y, z;
} Vector;
typedef struct {
    float x, y;
} Point2;
typedef struct {
    float dist, w, h, cx, cy;
} Screen;

int _nj_matrix_flag_;
float _nj_near_clip_;
float _nj_far_clip_;
Screen _nj_screen_ = {450.0F, 640.0F, 480.0F, 320.0F, 240.0F};
float _nj_scr_aspect_x_ = 1.0F;
float _nj_scr_aspect_y_ = 0.875F;

void njCalcVector(float *m, Vector *vs, Vector *vd);
void njClipZ(float n, float f);
void njProjectScreen(float *m, Vector *p3, Point2 *p2);

int main(void)
{
    unsigned i;

    harness_fpu();
    for (i = 0; i < 4000; i++) {
        float m[16];
        float x[16];
        Vector v;
        Vector out;
        Point2 p;
        int k;

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(3.0F);
            x[k] = harness_value(3.0F);
        }
        v.x = harness_value(400.0F);
        v.y = harness_value(400.0F);
        v.z = harness_value(400.0F);
        _nj_matrix_flag_ = (int)(i % 2);
        harness_load_xmtrx(x);
        harness_begin("calc", i);
        njCalcVector(m, &v, &out);
        harness_float(out.x);
        harness_float(out.y);
        harness_float(out.z);
        njCalcVector(0, &v, &out);
        harness_float(out.x);
        harness_float(out.y);
        harness_float(out.z);
        out = v;
        njCalcVector(m, &out, &out);
        harness_float(out.x);
        harness_float(out.y);
        harness_float(out.z);
        harness_end();

        harness_begin("project", i);
        njProjectScreen(m, &v, &p);
        harness_float(p.x);
        harness_float(p.y);
        njProjectScreen(0, &v, &p);
        harness_float(p.x);
        harness_float(p.y);
        harness_end();

        njClipZ(harness_value(100000.0F), harness_value(100000.0F) - 2.0F);
        harness_begin("clip", i);
        harness_float(_nj_near_clip_);
        harness_float(_nj_far_clip_);
        harness_end();
    }
    return 0;
}
