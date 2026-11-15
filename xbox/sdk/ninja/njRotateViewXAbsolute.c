/* njRotateViewXAbsolute: set v's direction to its absolute one rotated by
 * ang about x, or the current view's where v is null. make pc compiles this
 * in place of sdk/ninja/njRotateViewXAbsolute.c, whose body is SH-4
 * assembly: FSCA's sine s and cosine c, and an FMAC for each part */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njview.h"

void(njRotateViewXAbsolute)(NJS_VIEW *v, Angle ang)
{
    NJS_VIEW *view;
    Float s;
    Float c;
    Float y;
    Float z;

    _builtin_fsca(ang, &s, &c);
    view = nj_view_or_current(v);
    y = view->avy;
    z = view->avz;
    view->vx = view->avx;
    view->vz = sh4_fmac(s, y, z * c);
    view->vy = sh4_fmac(-s, z, y * c);
}
