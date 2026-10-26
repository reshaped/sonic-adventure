/* macroblock makers that make nothing. MPV_COND_DECODE 0 selects the two for
 * skipped macroblocks */
#include <sg_xpt.h>

#include "movie/sdk/mpv.h"

void MPVMMC_Intra(struct mpv_obj *mpv)
{
}

void MPVMMC_Forward(struct mpv_obj *mpv)
{
}

void MPVMMC_Backward(struct mpv_obj *mpv)
{
}

void MPVMMC_BiDirect(struct mpv_obj *mpv)
{
}

void MPVMMC_PpicSkipped(struct mpv_obj *mpv, Sint32 count)
{
}

void MPVMMC_BpicSkipped(struct mpv_obj *mpv, Sint32 count)
{
}
