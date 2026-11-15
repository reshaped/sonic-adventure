/* the port's XMTRX (xbox/include/sh4.h), which 1ST_READ.BIN holds for every
 * image, and machine.h's st_ext, the store of it into m, row by row as the
 * console's pair stores leave it */
#include <machine.h>

#include "xbox/include/sh4.h"

Float sh4_xmtrx[16];

void _builtin_st_ext(float (*m)[4])
{
    sh4_xmtrx_get((Float *)m);
}
