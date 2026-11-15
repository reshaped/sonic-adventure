/* njRotateViewYRelative: rotate v's direction by ang about y, or the
 * current view's where v is null. make pc compiles this in place of
 * sdk/ninja/njRotateViewYRelative.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njview.h"

void(njRotateViewYRelative)(NJS_VIEW *v, Angle ang)
{
    NJS_VIEW *view;
    Float s;
    Float c;
    Float x;
    Float z;

    _builtin_fsca(ang, &s, &c);
    view = nj_view_or_current(v);
    x = view->vx;
    z = view->vz;
    view->vx = sh4_fmac(s, z, x * c);
    view->vz = sh4_fmac(-s, x, z * c);
}
