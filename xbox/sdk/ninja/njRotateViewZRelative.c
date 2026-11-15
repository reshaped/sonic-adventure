/* njRotateViewZRelative: add ang to v's roll, or the current view's where v
 * is null. make pc compiles this in place of
 * sdk/ninja/njRotateViewZRelative.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/sdk/ninja/njview.h"

void(njRotateViewZRelative)(NJS_VIEW *v, Angle ang)
{
    NJS_VIEW *view = nj_view_or_current(v);

    view->roll += ang;
}
