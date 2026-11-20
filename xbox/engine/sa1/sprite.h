/* the Xbox port's SA1 sprites: engine/dc/sa1/sprite.h's sends, unchanged,
 * into a run of the renderer's where one takes them (sa1/emit.h), and else
 * into Kamui's list as the Dreamcast's. a sprite's corners hold an intensity
 * of 1, so each takes the colors of the intensity header its draw follows
 * (emit_face). sprite_send_corner's XBOX side writes a corner into the open
 * run */
#ifndef XBOX_SA1_SPRITE_H
#define XBOX_SA1_SPRITE_H

#include "sa1/emit.h"

#define sa_present_live sa1_dc_sprite_live
#include "../../../engine/dc/sa1/sprite.h"
#undef sa_present_live

ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    struct sprite_vertex *start = sa1_live_strip;
    Uint32 *after;
    sa_result result;

    if (!emit_face((const Uint32 *)start) || !emit_begin(TRUE, EMIT_SCREEN)) {
        return sa1_dc_sprite_live(engine, draw);
    }
    result = sa1_dc_sprite_live(engine, draw);
    after = emit_end((Uint32 *)start);
    if (after != NULL) {
        sa1_live_strip = (struct sprite_vertex *)after;
        return result;
    }
    sa1_live_strip = start;
    return sa1_dc_sprite_live(engine, draw);
}

#endif
