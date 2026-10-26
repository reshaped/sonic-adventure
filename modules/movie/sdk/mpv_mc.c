/* MPEG video reference block copy setup */
#include <sg_xpt.h>

#include "movie/sdk/mpv.h"

/* the copy kernel for each source alignment: by longs, bytes, words, and bytes
 * again */
static void (*const mpvmc_ref1_kernels[4])(Uint8 *block, const Uint8 *src,
                                           Sint32 pitch) = {
    MPVAMC_OneRef1Long,
    MPVAMC_OneRef1Byte,
    MPVAMC_OneRef1Word,
    MPVAMC_OneRef1Byte,
};

void MPVMC_Init(struct mpv_mc *mc)
{
    mc->ref1[0] = mpvmc_ref1_kernels[0];
    mc->ref1[1] = mpvmc_ref1_kernels[1];
    mc->ref1[2] = mpvmc_ref1_kernels[2];
    mc->ref1[3] = mpvmc_ref1_kernels[3];
}
