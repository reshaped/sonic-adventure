/* Ninja's camera functions, whose bodies tools/sh2c.py translates:
 * njSetCamera, njSetCameraAngle, njPointCameraInterest and
 * njRollCameraInterest, against the SH-4's. they call the real rotations,
 * njMultiMatrix, njCalcVector, njRotate and njUnitMatrix, and standins.h's
 * njTranslate, njScale, njTan, njUnitVector and acosf */
#define REAL_NJCLEARMATRIX
#include "standins.h"

typedef struct {
    float px, py, pz;
    float vx, vy, vz;
    int roll;
    int ang;
    float n_clip;
    float f_clip;
    float lx[3], ly[3];
} Camera;

typedef struct {
    float dist, w, h, cx, cy;
} Screen;

__attribute__((aligned(32))) float _unit_matrix_[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                                                         0, 0, 1, 0, 0, 0, 0, 1};
__attribute__((aligned(32))) float _nj_view_matrix_[16];
__attribute__((aligned(32))) float _nj_camera_matrix_[16];
static __attribute__((aligned(32))) float stack[4][16];
float (*_nj_base_matrix_ptr_)[16] = &stack[0];
float (*_nj_current_matrix_ptr_)[16] = &stack[2];
int _stack_size_ = 3;
int _nj_matrix_flag_ = 1;
float _nj_near_clip_, _nj_far_clip_;
float _nj_scr_aspect_x_ = 1.0F;
Screen _nj_screen_ = {0, 640, 480, 320, 240};

void njSetCamera(Camera *c);
void njSetCameraAngle(Camera *c, int ang);
void njPointCameraInterest(Camera *c, float x, float y, float z);
void njRollCameraInterest(Camera *c, int ang);

static void put(const float *m, int n)
{
    int k;

    for (k = 0; k < n; k++) {
        harness_float(m[k]);
    }
}

int main(void)
{
    static Camera camera;
    unsigned i;

    harness_fpu();
    for (i = 0; i < 2000; i++) {
        float *at = (float *)&camera;
        float x[16];
        unsigned k;

        for (k = 0; k < sizeof camera / 4; k++) {
            at[k] = harness_value(5.0F);
        }
        camera.roll = (int)(harness_random() >> 12) - 0x80000;
        camera.ang = (int)(harness_random() >> 16);
        camera.n_clip = -1.0F - __builtin_fabsf(harness_value(10.0F));
        camera.f_clip = camera.n_clip - 100.0F;
        for (k = 0; k < 16; k++) {
            stack[0][k] = harness_value(3.0F);
            _nj_camera_matrix_[k] = harness_value(3.0F);
            x[k] = harness_value(3.0F);
        }
        harness_load_xmtrx(x);
        _stack_size_ = 3;
        _nj_current_matrix_ptr_ = &stack[2];
        njSetCameraAngle(&camera, camera.ang ^ 0x55);
        njSetCamera(&camera);
        harness_store_xmtrx(x);
        harness_begin("set", i);
        harness_hex((unsigned)_stack_size_);
        harness_hex((unsigned)(_nj_current_matrix_ptr_ - stack));
        put(stack[0], 16);
        put(_nj_view_matrix_, 16);
        put(_nj_camera_matrix_, 16);
        put(x, 16);
        harness_float(_nj_screen_.dist);
        harness_float(_nj_near_clip_);
        harness_float(_nj_far_clip_);
        put(at, sizeof camera / 4);
        harness_end();
        njPointCameraInterest(&camera, harness_value(50.0F), harness_value(50.0F), harness_value(50.0F));
        harness_begin("point", i);
        put(at, sizeof camera / 4);
        put(_nj_camera_matrix_, 16);
        harness_end();
        njRollCameraInterest(&camera, (int)(harness_random() >> 16));
        harness_begin("roll", i);
        put(at, sizeof camera / 4);
        put(_nj_camera_matrix_, 16);
        harness_end();
    }
    return 0;
}
