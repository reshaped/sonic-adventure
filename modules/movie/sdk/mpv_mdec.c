/* block decoders that decode nothing and return 0. MPV_COND_DECODE 0 selects
 * the first two */
#include <sg_xpt.h>

#include "movie/sdk/mpv.h"

Sint32 MPVMDEC_IntraBlocks(struct mpv_obj *mpv)
{
    return 0;
}

Sint32 MPVMDEC_NintraBlocks(struct mpv_obj *mpv)
{
    return 0;
}

Sint32 mpvmdec_AwayIntraYblock(void)
{
    return 0;
}

Sint32 mpvmdec_AwayIntraCblock(void)
{
    return 0;
}

Sint32 mpvmdec_AwayNintraBlock(void)
{
    return 0;
}
