/* Ninja's view functions against xbox/sdk/ninja/: the view rotations and
 * translations, through a view and through the current one, and
 * njSetPerspective. njTan is a stand-in, the same on both runs */
#include "harness.h"

typedef struct {
    float m[16];
    float px, py, pz;
    float vx, vy, vz;
    int roll;
    float apx, apy, apz;
    float avx, avy, avz;
    int aroll;
} View;

typedef struct {
    float dist, w, h, cx, cy;
} Screen;

View *_nj_view_;
Screen _nj_screen_ = {0, 640, 480, 320, 240};

float njTan(int n)
{
    return (float)n * 0.0001F + 0.25F;
}

void njRotateViewXAbsolute(View *v, int ang);
void njRotateViewXRelative(View *v, int ang);
void njRotateViewYRelative(View *v, int ang);
void njRotateViewZAbsolute(View *v, int ang);
void njRotateViewZRelative(View *v, int ang);
void njTranslateViewAbsolute(View *v, float x, float y, float z);
void njTranslateViewRelative(View *v, float x, float y, float z);
void njSetPerspective(int ang);

static void fill(View *v)
{
    float *at = (float *)v;
    unsigned k;

    for (k = 0; k < sizeof(View) / 4; k++) {
        at[k] = harness_value(100.0F);
    }
    v->roll = (int)harness_random();
    v->aroll = (int)harness_random();
}

static void put(const View *v)
{
    const unsigned *at = (const unsigned *)v;
    unsigned k;

    for (k = 16; k < sizeof(View) / 4; k++) {
        harness_hex(at[k]);
    }
}

int main(void)
{
    static View views[2];
    unsigned i;

    harness_fpu();
    _nj_view_ = &views[1];
    for (i = 0; i < 3000; i++) {
        View *v = (i % 2) ? 0 : &views[0];
        int ang = (int)(harness_random() >> 12) - 0x80000;

        fill(&views[0]);
        fill(&views[1]);
        harness_begin("view", i);
        njRotateViewXAbsolute(v, ang);
        put(&views[0]);
        put(&views[1]);
        njRotateViewXRelative(v, ang + 77);
        njRotateViewYRelative(v, ang - 1234);
        put(&views[0]);
        put(&views[1]);
        njRotateViewZAbsolute(v, ang);
        njRotateViewZRelative(v, ang);
        njTranslateViewAbsolute(v, harness_value(10.0F), harness_value(10.0F), harness_value(10.0F));
        put(&views[0]);
        put(&views[1]);
        njTranslateViewRelative(v, harness_value(10.0F), harness_value(10.0F), harness_value(10.0F));
        put(&views[0]);
        put(&views[1]);
        njSetPerspective(ang);
        harness_float(_nj_screen_.dist);
        harness_end();
    }
    return 0;
}
