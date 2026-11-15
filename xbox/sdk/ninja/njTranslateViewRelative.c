/* njTranslateViewRelative: move v's position by x, y and z, or the current
 * view's where v is null. make pc compiles this in place of
 * sdk/ninja/njTranslateViewRelative.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/sdk/ninja/njview.h"

void(njTranslateViewRelative)(NJS_VIEW *v, Float x, Float y, Float z)
{
    NJS_VIEW *view = nj_view_or_current(v);
    Float px = x + view->px;
    Float py = y + view->py;
    Float pz = z + view->pz;

    view->pz = pz;
    view->py = py;
    view->px = px;
}
