/* the sprites, as njSprite.src's and sprite.src's entries. each entry makes
 * its record, then sets Ninja's texture and color mode as the Dreamcast's
 * does. so Ninja's context holds what it would there, and the next draw
 * inherits it. sprite_draw_3d sets them only for a frame the Dreamcast
 * sends. so it runs the sends' tests, in the .src files' float order. the
 * corners those tests place go nowhere else. each entry first asks
 * FRAME_DRAWS or present_sprite whether it draws, as the Dreamcast's does */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>

#include "ninja/njkamui.h"
#include "native.h"
#include "play_math.h"
#include "sa1.h"

/* njSetkmColorMode's mode for a sprite's strip in list, COLOR_MODE_OPAQUE or
 * COLOR_MODE_TRANS, as engine/dc/njsprite.h gives it */
#define SPRITE_MODE(list)                                                      \
    (((list) | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY) & ~COLOR_MODE_GOURAUD)

extern int frame_step;
extern float _nj_far_clip_;

static const NJS_ARGB sprite_white = {1.0F, 1.0F, 1.0F, 1.0F};
static float sprite_depth;

void sprite_set_depth(float depth)
{
    sprite_depth = depth;
}

float sprite_get_depth(void)
{
    return sprite_depth;
}

void njInitSprite(void)
{
}

struct corner {
    float x, y;
};

/* fill out with frame n's corners about its center, in the order
 * engine/dc/sa1/sprite.h's sprite_corners gives them. x scales by sp->sx and
 * y by y_scale, then both by scale. NJD_SPRITE_ANGLE turns them by sp->ang */
static void sprite_corners(const NJS_SPRITE *sp, int n, uint32_t attr,
                           float y_scale, float scale, struct corner out[4])
{
    const NJS_TEXANIM *frame = &sp->tanim[n];
    float x1 = (float)-frame->cx * sp->sx * scale;
    float y1 = (float)-frame->cy * y_scale * scale;
    float x2 = (float)(frame->sx - frame->cx) * sp->sx * scale;
    float y2 = (float)(frame->sy - frame->cy) * y_scale * scale;

    if (attr & NJD_SPRITE_ANGLE) {
        float turn[2];

        sa_fsca((uint32_t)sp->ang, turn);
        out[0].x = (x1 * turn[1]) - (y1 * turn[0]);
        out[0].y = (x1 * turn[0]) + (y1 * turn[1]);
        out[1].x = (x1 * turn[1]) - (y2 * turn[0]);
        out[1].y = (x1 * turn[0]) + (y2 * turn[1]);
        out[2].x = (x2 * turn[1]) - (y1 * turn[0]);
        out[2].y = (x2 * turn[0]) + (y1 * turn[1]);
        out[3].x = (x2 * turn[1]) - (y2 * turn[0]);
        out[3].y = (x2 * turn[0]) + (y2 * turn[1]);
    } else {
        out[0].x = out[1].x = x1;
        out[2].x = out[3].x = x2;
        out[0].y = out[2].y = y1;
        out[1].y = out[3].y = y2;
    }
}

/* whether all four lie past one edge of _nj_2d_clip_ */
static Bool off_screen(const struct corner c[4])
{
    float left = _nj_2d_clip_[0].x;
    float top = _nj_2d_clip_[0].y;
    float right = _nj_2d_clip_[1].x;
    float bottom = _nj_2d_clip_[1].y;

    return (c[0].y > bottom && c[1].y > bottom && c[2].y > bottom &&
            c[3].y > bottom) ||
           (top > c[0].y && top > c[1].y && top > c[2].y && top > c[3].y) ||
           (c[0].x > right && c[1].x > right && c[2].x > right &&
            c[3].x > right) ||
           (left > c[0].x && left > c[1].x && left > c[2].x && left > c[3].x);
}

/* whether engine/dc/sprite.c's sprite_send_facing sends frame n. the
 * corners' y scales by sp->sx, as its @bug says. their x scales by
 * port_draw_aspect, as sprite_send_facing_at scales them for 16:9 */
static Bool facing_sent(const NJS_SPRITE *sp, int n, uint32_t attr)
{
    float in[4] = {sp->p.x, sp->p.y, sp->p.z, 1.0F};
    float at[4];
    float z;
    float scale;
    float x;
    float y;
    struct corner c[4];

    sa_ftrv(native_xmtrx(), in, at);
    z = at[2] + sprite_depth;
    if (z > _nj_far_clip_ || _nj_near_clip_ > z) {
        return FALSE;
    }
    scale = _nj_screen_.dist * (1.0F / at[2]);
    sprite_corners(sp, n, attr, sp->sx, scale, c);
    x = (at[0] * (port_draw_aspect * scale)) + _nj_screen_.cx;
    y = (at[1] * (_nj_scr_aspect_y_ * scale)) + _nj_screen_.cy;
    for (int i = 0; i < 4; i++) {
        c[i].x = (c[i].x * port_draw_aspect) + x;
        c[i].y += y;
    }
    return !off_screen(c);
}

/* whether engine/dc/sa1/sprite.c's sprite_send_plane sends frame n, its x
 * scaled by port_draw_aspect. the off-screen test projects all four corners,
 * even of a frame the near clip cuts. a corner behind the eye keeps its 1/z's
 * sign */
static Bool plane_sent(const NJS_SPRITE *sp, int n, uint32_t attr)
{
    float far_clip = _nj_far_clip_;
    float near_clip = _nj_near_clip_;
    float scale_x = _nj_screen_.dist * port_draw_aspect;
    float scale_y = _nj_screen_.dist * _nj_scr_aspect_y_;
    float views[4][4];
    struct corner c[4];
    int past = 0;
    int nearer = 0;

    sprite_corners(sp, n, attr, sp->sy, 1.0F, c);
    for (int i = 0; i < 4; i++) {
        float in[4] = {c[i].x + sp->p.x, c[i].y + sp->p.y, sp->p.z, 1.0F};

        sa_ftrv(native_xmtrx(), in, views[i]);
        past += views[i][2] > far_clip;
        nearer += near_clip > views[i][2];
    }
    if (past == 4 || nearer == 4) {
        return FALSE;
    }
    for (int i = 0; i < 4; i++) {
        float inverse = 1.0F / views[i][2];

        c[i].x = (views[i][0] * inverse * scale_x) + _nj_screen_.cx;
        c[i].y = (views[i][1] * inverse * scale_y) + _nj_screen_.cy;
    }
    return !off_screen(c);
}

/* open frame n's strip through Ninja, as engine/dc/njsprite.h's sprite_begin
 * does */
static void *sprite_begin(const NJS_SPRITE *sp, int n, uint32_t attr)
{
    Uint32 *color = (Uint32 *)(attr & NJD_SPRITE_COLOR ? &_nj_constant_material_
                                                       : &sprite_white);

    njSetTexture(sp->tlist);
    njSetkmTextureNum((Uint32)(int32_t)sp->tanim[n].texid);
    if (attr & NJD_SPRITE_ALPHA) {
        njSetkmColorMode(SPRITE_MODE(COLOR_MODE_TRANS));
        return njStartVertexTexTransIntens(color);
    }
    njSetkmColorMode(SPRITE_MODE(COLOR_MODE_OPAQUE));
    return njStartVertexTexOpaqueIntens(color);
}

void sprite_draw_3d(NJS_SPRITE *sprite, Int entry, Uint32 attr)
{
    if (!FRAME_DRAWS()) {
        return;
    }
    present_record(PRESENT_SPRITE, sprite, 0, entry, 0.0F, attr);
    /* a frame facing the view scales its y by sprite->sx, as
     * sprite_send_facing's @bug says. one in the plane scales it by
     * sprite->sy */
    sa1_record_sprite(sprite, entry, attr,
                      attr & NJD_SPRITE_SCALE ? sprite->sx : sprite->sy,
                      sprite_depth, FALSE);
    njGetkmPointer();
    if (attr & NJD_SPRITE_SCALE ? facing_sent(sprite, entry, attr)
                                : plane_sent(sprite, entry, attr)) {
        njEndVertex(sprite_begin(sprite, entry, attr));
    }
    njSetkmPointer();
}

void njDrawSprite2D(NJS_SPRITE *sp, Int n, Float pri, Uint32 attr)
{
    if (!present_sprite(PRESENT_SPRITE_2D, sp, 0, n, pri, attr)) {
        return;
    }
    sa1_record_sprite(sp, n, attr, sp->sy, pri, TRUE);
    njGetkmPointer();
    njEndVertex(sprite_begin(sp, n, attr));
    njSetkmPointer();
}

void njDrawSprite3D(NJS_SPRITE *sp, Int n, Uint32 attr)
{
    if (!present_sprite(PRESENT_NJ_SPRITE, sp, 0, n, 0.0F, attr)) {
        return;
    }
    /* a host counts each frame its adapter takes, as port.h says of its
     * counts. the Dreamcast's entry counts the bytes of each frame it sends */
    port_sprites_sent++;
    /* a frame facing the view or in the plane scales its y by sp->sy, as
     * njSprite.src's sends do */
    sa1_record_sprite(sp, n, attr, sp->sy, 0.0F, FALSE);
    njGetkmPointer();
    /* the Dreamcast's entry sets the texture and color mode for every frame,
     * and ends the strip only for a frame it sends. the strip's end moves
     * the list's write pointer alone, which reaches no hardware here. so
     * every frame ends its strip, and this entry runs no send's tests */
    njEndVertex(sprite_begin(sp, n, attr));
    njSetkmPointer();
}
