/* njTranslateViewAbsolute: set v's position to its absolute one moved by x,
 * y and z, or the current view's where v is null. make pc compiles this in
 * place of sdk/ninja/njTranslateViewAbsolute.c, whose body is SH-4
 * assembly */
#include <ninja.h>

#include "xbox/sdk/ninja/njview.h"

void(njTranslateViewAbsolute)(NJS_VIEW *v, Float x, Float y, Float z)
{
    NJS_VIEW *view = nj_view_or_current(v);
    Float px = x + view->apx;
    Float py = y + view->apy;
    Float pz = z + view->apz;

    view->pz = pz;
    view->py = py;
    view->px = px;
}
