/* njPushMatrix: push the current matrix, XMTRX, then make it m, or keep it
 * where m is null. FALSE where the stack is full. make pc compiles this in
 * place of sdk/ninja/njPushMatrix.c, whose body is SH-4 assembly: XMTRX
 * stored at the stack's top by pairs, the top moved on a matrix, then m
 * loaded into XMTRX, or XMTRX stored again at the new top */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njstack.h"

Bool(njPushMatrix)(NJS_MATRIX *m)
{
    NJS_MATRIX *top = _nj_current_matrix_ptr_;

    if (_stack_size_ >= _max_stack_size_) {
        return FALSE;
    }
    _stack_size_++;
    sh4_xmtrx_get((Float *)top);
    _nj_current_matrix_ptr_ = top + 1;
    if (m != NULL) {
        sh4_xmtrx_set((const Float *)m);
    } else {
        sh4_xmtrx_get((Float *)(top + 1));
    }
    return TRUE;
}
