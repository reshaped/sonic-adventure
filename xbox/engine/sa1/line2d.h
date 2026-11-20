/* the Xbox port's SA1 2D lines, njDrawLine2D: engine/dc/sa1/line2d.h's sends,
 * unchanged, into a run of the renderer's where one takes them (sa1/strip.h's
 * sa1_emit_present), and else into Kamui's list as the Dreamcast's */
#ifndef XBOX_SA1_LINE2D_H
#define XBOX_SA1_LINE2D_H

#define sa_present_live sa1_dc_line2d_live
#include "../../../engine/dc/sa1/line2d.h"
#undef sa_present_live

ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    return sa1_emit_present(engine, draw, sa1_dc_line2d_live);
}

#endif
