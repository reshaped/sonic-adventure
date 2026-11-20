/* the Xbox port's SA1 screen polygons, njDrawPolygon and njDrawTexture: engine/dc/sa1/polygon.h's sends,
 * unchanged, into a run of the renderer's where one takes them (sa1/strip.h's
 * sa1_emit_present), and else into Kamui's list as the Dreamcast's */
#ifndef XBOX_SA1_POLYGON_H
#define XBOX_SA1_POLYGON_H

#define sa_present_live sa1_dc_polygon_live
#include "../../../engine/dc/sa1/polygon.h"
#undef sa_present_live

ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    return sa1_emit_present(engine, draw, sa1_dc_polygon_live);
}

#endif
