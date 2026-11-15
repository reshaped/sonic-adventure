/* SH-4 register, cache and matrix helpers. make pc compiles this in place
 * of src/system/crt0.src.
 *
 * x86 has no SR, CCR or stack to hand over: _SetSR sets the port's
 * interrupt mask from SR's bits 4 to 7, the only part of SR the game sets
 * that the port models, _SetSP leaves the stack to the port's own startup,
 * _GetSP returns this frame's address, and _CashOn does nothing.
 *
 * point_calc and matrix_load load XMTRX by rows, the transpose of Ninja's
 * order, as crt0.src's loads after its FRCHG do. so FTRV computes the
 * matrix times (x, y, z, 1) with w taking floats 3, 7 and 11. no C names
 * these, so their declarations are here */
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "xbox/include/sh4.h"

void _SetSR(Uint32 sr);
void _SetSP(void *sp);
void *_GetSP(void);
void _CashOn(void);
void point_calc(NJS_MATRIX *matrix, NJS_POINT3 *point, NJS_POINT3 *out);
void matrix_load(NJS_MATRIX *matrix);
void point_calc_loaded(NJS_POINT3 *point, NJS_POINT3 *out);

void _SetSR(Uint32 sr)
{
    set_imask((int)(sr >> 4 & 15));
}

void _SetSP(void *sp)
{
    (void)sp;
}

void *_GetSP(void)
{
    return __builtin_frame_address(0);
}

void _CashOn(void)
{
}

void matrix_load(NJS_MATRIX *matrix)
{
    const Float *m = (const Float *)(matrix != NULL ? matrix : _nj_current_matrix_ptr_);
    Float rows[16];
    int k;

    for (k = 0; k < 16; k++) {
        rows[4 * (k % 4) + k / 4] = m[k];
    }
    sh4_xmtrx_set(rows);
}

/* point is read whole before out is written, so out may be point */
void point_calc_loaded(NJS_POINT3 *point, NJS_POINT3 *out)
{
    sh4_xmtrx_apply3(point, 1.0F, out);
}

void point_calc(NJS_MATRIX *matrix, NJS_POINT3 *point, NJS_POINT3 *out)
{
    NJS_POINT3 at = *point;

    matrix_load(matrix);
    point_calc_loaded(&at, out);
}
