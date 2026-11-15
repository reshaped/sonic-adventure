/* njRotateViewZAbsolute: set v's roll to its absolute roll plus ang, or the
 * current view's where v is null. make pc compiles this in place of
 * sdk/ninja/njRotateViewZAbsolute.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/sdk/ninja/njview.h"

void(njRotateViewZAbsolute)(NJS_VIEW *v, Angle ang)
{
    NJS_VIEW *view = nj_view_or_current(v);

    view->roll = view->aroll + ang;
}
