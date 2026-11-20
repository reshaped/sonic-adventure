/* Ninja's sprite entries in C, SA1's layer for them on the Dreamcast. make
 * dc compiles this in place of sdk/ninja/njSprite.src. sa1/sprite.h's
 * sprite_send_facing_at, sprite_send_plane and sprite_send_2d do the work of
 * njSprite.src's sprite_emit_billboard, sprite_emit_transformed and
 * sprite_emit_2d.
 *
 * a draw opens a strip with sprite_begin, which writes its polygon header.
 * njsprite.h's sprite_live then sends the frame's vertices through the store
 * queues, and njEndVertex ends the strip. this file includes sa1/sprite.c,
 * so sprite_send_plane links in this object, where sprite_emit_transformed
 * lay */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "engine/dc/dreamcast.h"
#include "engine/dc/njsprite.h"
#include "sa1/sprite.c"
#include "sdk/ninja/njkamui.h"

/* do nothing, since the sprites keep no state */
void njInitSprite(void)
{
}

Uint32 port_sprites_sent;

/* dreamcast.h's port_draw_aspect, port_pillarbox, port_wide_x and
 * port_wide_squeezes are defined here, not in wide.c. LOADER.BIN links this
 * file's object but not wide.c's */
Float port_draw_aspect = 1.0F;
Int port_pillarbox;

Float port_wide_x(Float x)
{
    /* the x the squeeze leaves in place: the pivot, 320 pixels from the
     * screen's left edge at WIDE_MIDDLE */
    Float at = port_pillarbox ? 320.0F : 320.0F + port_wide_anchor;

    return at + ((x - at) * port_draw_aspect);
}

Int port_wide_squeezes(void)
{
    return port_draw_aspect != 1.0F && (port_pillarbox || !port_wide_cover);
}

/* draw frame n of sp's texture animation on the screen. the frame's center
 * lies at sp->p.x and sp->p.y, in pixels. its 1/z is 1 where pri is over -1,
 * and -1/pri otherwise. NJD_SPRITE_ALPHA sends it to the translucent list.
 * NJD_SPRITE_COLOR colors the strip with _nj_constant_material_ in place of
 * white. where present_sprite returns FALSE, it does nothing */
void njDrawSprite2D(NJS_SPRITE *sp, Int n, Float pri, Uint32 attr)
{
    struct sprite_vertex *sq;

    if (!present_sprite(PRESENT_SPRITE_2D, sp, 0, n, pri, attr)) {
        return;
    }
    njGetkmPointer();
    sq = sprite_live(sp, n, attr, SA1_SPRITE_SCREEN, 0.0F, pri,
                     sprite_begin(sp, n, attr));
    njEndVertex((Uint32 *)sq);
    njSetkmPointer();
}

/* draw frame n of sp's texture animation at sp->p, through the current
 * matrix. with NJD_SPRITE_SCALE the frame faces the screen, as
 * sprite_send_facing_at sends it. without it the frame lies in the plane
 * z = sp->p.z, as sprite_send_plane sends it. each takes the y scale
 * sp->sy and no bias. NJD_SPRITE_ALPHA and NJD_SPRITE_COLOR act as in
 * njDrawSprite2D. when the frame isn't sent, njEndVertex isn't called, and
 * the list's write pointer stays at the header's start. where
 * present_sprite returns FALSE, it does nothing */
void njDrawSprite3D(NJS_SPRITE *sp, Int n, Uint32 attr)
{
    struct sprite_vertex *sq;
    Uint32 *list;

    if (!present_sprite(PRESENT_NJ_SPRITE, sp, 0, n, 0.0F, attr)) {
        return;
    }
    njGetkmPointer();
    sq = sprite_begin(sp, n, attr);
    list = *_nj_listtype_;
    if (attr & NJD_SPRITE_SCALE) {
        sq = sprite_live(sp, n, attr, SA1_SPRITE_FACING, sp->sy, 0.0F, sq);
    } else {
        sq = sprite_live(sp, n, attr, SA1_SPRITE_PLANE, 0.0F, 0.0F, sq);
    }
    if (sq != NULL) {
        njEndVertex((Uint32 *)sq);
        port_sprites_sent += (Uint32)*_nj_listtype_ - (Uint32)list;
    }
    njSetkmPointer();
}
