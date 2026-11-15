/* njSetView: set v->m from v's position, direction and roll, make v the
 * current view, and call njClearMatrix. make pc compiles this in place of
 * sdk/ninja/njSetView.c, whose body is SH-4 assembly; its comment gives the
 * turns.
 *
 * h is the square root of vx * vx, FMAC vz * vz, and 1 / d FSRRA of h * h,
 * FMAC, plus vy * vy, as the body forms them. each angle is acosf's times
 * 10430.38 (H'4622F986), truncated by FTRC and cut to 16 bits. each call
 * names its function in parentheses, so that it reaches the function, as
 * the body's JSRs do, and not a macro of its name */
#include <mathf.h>
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njview.h"

/* the body's ?radian_to_angle, H'4622F986 */
static Float radian_to_angle(void)
{
    union {
        Uint32 bits;
        Float value;
    } k = {0x4622f986};

    return k.value;
}

void(njSetView)(NJS_VIEW *v)
{
    const Float *unit = (const Float *)&_unit_matrix_;
    Float *m = (Float *)&v->m;
    Float vx;
    Float vy;
    Float vz;
    Float h;
    Angle pitch;
    Angle yaw;
    int k;

    for (k = 0; k < 16; k++) {
        m[k] = unit[k];
    }
    (njRotateZ)(&v->m, -v->roll);
    vx = v->vx;
    vy = v->vy;
    vz = v->vz;
    h = __builtin_sqrtf(sh4_fmac(vz, vz, vx * vx));
    pitch = (Angle)(sh4_ftrc((acosf)(_builtin_fsrra(sh4_fmac(h, h, vy * vy)) * h) *
                             radian_to_angle()) &
                    0xffff);
    (njRotateX)(&v->m, 0.0F > vy ? -pitch : pitch);
    yaw = (Angle)(sh4_ftrc((acosf)(vz / h) * radian_to_angle()) & 0xffff);
    (njRotateY)(&v->m, 0.0F > vx ? yaw : -yaw);
    (njTranslate)(&v->m, -v->px, -v->py, -v->pz);
    _nj_view_ = v;
    (njClearMatrix)();
}
