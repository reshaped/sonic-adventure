/* njPopMatrix: drop n matrices from the stack and load the one then at its
 * top into XMTRX. FALSE where that would leave the stack empty. make pc
 * compiles this in place of sdk/ninja/njPopMatrix.c, whose body is SH-4
 * assembly: its CMP/HS compares the depth left, unsigned, with 1, so only
 * a depth of 0 fails, and an n past the depth wraps as the body's does */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njstack.h"

Bool(njPopMatrix)(Uint32 n)
{
    Uint32 depth = (Uint32)_stack_size_ - n;

    if (depth < 1) {
        return FALSE;
    }
    _stack_size_ = (Sint32)depth;
    _nj_current_matrix_ptr_ -= n;
    sh4_xmtrx_set((const Float *)_nj_current_matrix_ptr_);
    return TRUE;
}
