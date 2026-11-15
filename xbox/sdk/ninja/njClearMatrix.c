/* njClearMatrix: empty the matrix stack to its base, set the base to the
 * unit matrix scaled by -1 in x and y and then times the current view's
 * matrix, and make it the current matrix, in XMTRX, and _nj_view_matrix_.
 * make pc compiles this in place of sdk/ninja/njClearMatrix.c, whose body
 * is SH-4 assembly. the body's first pair loads fill XMTRX with the base as
 * it was, which its last fill replaces, so the C leaves them out. each call
 * names its function in parentheses, so that it reaches the function, as
 * the body's JSRs do, and not a macro of its name */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njstack.h"
#include "xbox/sdk/ninja/njview.h"

/* njGlobal.c's, which no header declares: the matrix stack's base */
extern NJS_MATRIX *_nj_base_matrix_ptr_;

void(njClearMatrix)(void)
{
    NJS_MATRIX *base = _nj_base_matrix_ptr_;
    const Float *unit = (const Float *)&_unit_matrix_;
    Float *out = (Float *)base;
    Float *view = (Float *)&_nj_view_matrix_;
    int k;

    _stack_size_ = 1;
    for (k = 8; k < 16; k++) {
        out[k] = unit[k];
    }
    for (k = 0; k < 8; k++) {
        out[k] = unit[k];
    }
    _nj_current_matrix_ptr_ = base;
    (njScale)(base, -1.0F, -1.0F, 1.0F);
    (njMultiMatrix)(base, &_nj_view_->m);
    sh4_xmtrx_set(out);
    for (k = 0; k < 16; k++) {
        view[k] = out[k];
    }
}
