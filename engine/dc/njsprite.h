/* what SA1's two sprite layers share: njSprite.c, Ninja's entries, and
 * engine/dc/sprite.c, the game's. each layer makes a frame's texture current
 * and writes its strip's header through Ninja. sa_present_live runs
 * sa1/sprite.h's sends, which write the strip's vertices.
 * sdk/ninja/njSprite.src's and src/draw/sprite.src's sends differ only in a
 * facing frame's y scale and depth. each layer passes its own in a struct
 * sa1_live_sprite.
 *
 * each extern below gives one of sa1/screen.h's names Ninja's own symbol. so
 * the sends read Ninja's own screen, clips and y aspect, as the .src files
 * do */
#ifndef ENGINE_DC_NJSPRITE_H
#define ENGINE_DC_NJSPRITE_H
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

extern NJS_SCREEN sa1_live_ninja_screen __asm__("__nj_screen_");
extern Float sa1_live_far_clip __asm__("__nj_far_clip_");
extern NJS_POINT2 sa1_live_clip[2] __asm__("__nj_2d_clip_");
extern Float sa1_live_aspect_y __asm__("__nj_scr_aspect_y_");
extern Float sa1_live_near_clip __asm__("__nj_near_clip_");

#include "sa1/sprite.h"

/* njSetkmColorMode's mode for a sprite's strip in list, COLOR_MODE_OPAQUE or
 * COLOR_MODE_TRANS. the strip is textured, and each vertex's intensity scales
 * the header's color. the mode leaves out the COLOR_MODE_GOURAUD bit that
 * both lists' modes hold */
#define SPRITE_MODE(list)                                                      \
    (((list) | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY) & ~COLOR_MODE_GOURAUD)

/* the color a sprite takes without NJD_SPRITE_COLOR */
static const NJS_ARGB sprite_white = {1.0F, 1.0F, 1.0F, 1.0F};

/* make frame n's texture current, and set the color mode of the list
 * NJD_SPRITE_ALPHA picks. write a strip's polygon header at the end of that
 * list. return the store queue address for the strip's first vertex */
static inline struct sprite_vertex *sprite_begin(const NJS_SPRITE *sp, Int n,
                                                 Uint32 attr)
{
    Uint32 *color = (Uint32 *)(attr & NJD_SPRITE_COLOR ? &_nj_constant_material_
                                                       : &sprite_white);

    njSetTexture(sp->tlist);
    njSetkmTextureNum(sp->tanim[n].texid);
    if (attr & NJD_SPRITE_ALPHA) {
        njSetkmColorMode(SPRITE_MODE(COLOR_MODE_TRANS));
        return njStartVertexTexTransIntens(color);
    }
    njSetkmColorMode(SPRITE_MODE(COLOR_MODE_OPAQUE));
    return njStartVertexTexOpaqueIntens(color);
}

/* draw frame n of sp from sq on, as how, an sa1_live_sprite_how, says. a
 * facing frame reads y_scale, and a screen or biased facing frame reads
 * bias. a frame in the plane reads neither. the draw is an sa_draw_live of
 * SA_SA1_SPRITE, kept in registers through the call. return the vertex past
 * the last, or NULL where the frame isn't sent */
ALWAYS_INLINE struct sprite_vertex *sprite_live(const NJS_SPRITE *sp, Int n,
                                                Uint32 attr, Uint32 how,
                                                Float y_scale, Float bias,
                                                struct sprite_vertex *sq)
{
    struct sa1_live_sprite state = {how, n, attr, y_scale, bias};
    sa_draw_live draw = {sp, &state, SA_SA1_SPRITE, 0};

    sa1_live_strip = sq;
    sa_present_live(SA_DC_ENGINE, &draw);
    return sa1_live_strip;
}

#endif
