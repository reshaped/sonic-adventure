/* njRotateViewYAbsolute: set v's direction (vx, vy, vz) to its absolute
 * direction (avx, avy, avz) turned by ang about y, from z toward x, or the
 * current view's where v is null. make pc compiles this in place of
 * modules/movie/sdk/njRotateViewYAbsolute.c, whose body is SH-4 assembly,
 * which MOVIE.BIN links (the cast's view) and 1ST_READ.BIN doesn't. its
 * steps are the assembly's: FSCA's sine and cosine, an FMUL, then an FMAC
 * for each of x and z */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njview.h"

void(njRotateViewYAbsolute)(NJS_VIEW *v, Angle ang)
{
    NJS_VIEW *view = nj_view_or_current(v);
    Float s, c;
    Float ax = view->avx;
    Float ay = view->avy;
    Float az = view->avz;

    _builtin_fsca(ang, &s, &c);
    view->vy = ay;
    view->vx = sh4_fmac(s, az, ax * c);
    view->vz = sh4_fmac(-s, ax, az * c);
}
