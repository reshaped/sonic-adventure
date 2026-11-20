/* the port's XMTRX (xbox/include/sh4.h), which 1ST_READ.BIN holds for every
 * image, and machine.h's st_ext, the store of it into m, row by row as the
 * console's pair stores leave it. SH4ZAM's software back end, which the
 * Dreamcast engine's drawers take on x86, keeps its XMTRX where
 * shz_xmtrx_state_ says: the port's, so that both read one matrix. its
 * columns are shz_vec4_t's, 16-byte vectors, as sh4_xmtrx's columns lie */
#include <machine.h>

/* sh4.h includes SH4ZAM's XMTRX header, after sg_xpt.h */
#include "xbox/include/sh4.h"

Float sh4_xmtrx[16] __attribute__((aligned(64)));

_Static_assert(sizeof(shz_xmtrx__t) == sizeof sh4_xmtrx, "SH4ZAM's XMTRX is 16 floats");

shz_xmtrx__t *shz_xmtrx_state_(void)
{
    return (shz_xmtrx__t *)sh4_xmtrx;
}

void _builtin_st_ext(float (*m)[4])
{
    sh4_xmtrx_get((Float *)m);
}
