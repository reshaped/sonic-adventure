/* njInitMatrix: make m the matrix stack, with room for n matrices, holding
 * one, the current matrix, in m[0] and in XMTRX: the unit matrix with x and
 * y negated. _nj_matrix_flag_ takes flag. make pc compiles this in place of
 * sdk/ninja/njInitMatrix.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njstack.h"

/* njGlobal.c's, which no header declares: the matrix stack's base */
extern NJS_MATRIX *_nj_base_matrix_ptr_;

void(njInitMatrix)(NJS_MATRIX *m, Sint32 n, Int flag)
{
    const Float *unit = (const Float *)&_unit_matrix_;
    Float *out = (Float *)m;
    int k;

    _stack_size_ = 1;
    _nj_matrix_flag_ = flag;
    _nj_current_matrix_ptr_ = m;
    _max_stack_size_ = n;
    _nj_base_matrix_ptr_ = m;
    for (k = 8; k < 16; k++) {
        out[k] = unit[k];
    }
    for (k = 0; k < 8; k++) {
        out[k] = unit[k];
    }
    out[0] = -unit[0];
    out[5] = -unit[5];
    sh4_xmtrx_set(out);
}
