/* the view the view functions take where theirs is null: Ninja's current
 * one, _nj_view_ in njGlobal.c, which no header declares */
#ifndef XBOX_NJVIEW_H
#define XBOX_NJVIEW_H
#include <ninja.h>

extern NJS_VIEW *_nj_view_;

static inline NJS_VIEW *nj_view_or_current(NJS_VIEW *v)
{
    return v != NULL ? v : _nj_view_;
}

#endif
