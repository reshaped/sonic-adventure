/* SA1's sprites on the Dreamcast: the sends sa_present_live runs for a live
 * draw of SA_SA1_SPRITE, a frame of the game's own NJS_SPRITE, which they
 * read during the call alone. Ninja's sprite entries, engine/dc/njSprite.c,
 * and the game's, engine/dc/sprite.c, are SA1's layer: each keeps Ninja's
 * state, opens the sprite's strip through Ninja, whose header holds its
 * texture, color and list, and hands the sprite to sa_present_live, which
 * projects and clips the frame and sends its vertices into that strip before
 * it returns. a sprite draws in the transform live at the call, as a model
 * does. Ninja's screen, clips and y aspect come lent by screen.h's names, and
 * 16:9's picture and squeeze are the Dreamcast engine's own, as dreamcast.h
 * gives them.
 *
 * the C does the .src files' float arithmetic in their order. make dc's
 * flags fuse no product and sum into an FMAC, and GCC takes each 1/z by
 * FDIV, so each result rounds as theirs does. the only arithmetic the C adds
 * scales x by the picture's aspect for 16:9, and changes nothing at 4:3,
 * where that aspect is 1 */
#ifndef SA1_SPRITE_H
#define SA1_SPRITE_H

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <sh4zam/shz_xmtrx.h>

#include "engine/dc/dreamcast.h"
#include "sa1/screen.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* NJS_TEXANIM's u1, v1, u2 and v2 count 256 to the texture's side */
#define UV_SCALE (1.0F / 256.0F)

/* a vertex as the tile accelerator takes it. a sprite writes every word but
 * unwritten, which goes out as the store queue held it */
struct sprite_vertex {
    Uint32 control;
    Float x, y, rhw, u, v;
    Float intensity;
    Uint32 unwritten;
};
_Static_assert(sizeof(struct sprite_vertex) == 32, "a vertex isn't 32 bytes");

/* a frame's corner: its x and y, its 1/z, and its u and v. x and y lie
 * about the frame's center until the corner is placed on the screen */
struct sprite_corner {
    Float x, y, rhw, u, v;
};

/* fill corners with frame n's corners, about its center, in the order
 * (x1, y1), (x1, y2), (x2, y1), (x2, y2). with frame = sp->tanim[n], x1 is
 * -frame->cx and x2 is frame->sx - frame->cx, each times sp->sx and then
 * scale. y1 is -frame->cy and y2 is frame->sy - frame->cy, each times y_scale
 * and then scale. NJD_SPRITE_ANGLE then turns the corners about the center by
 * sp->ang. their u and v are (u1, v1), (u1, v2), (u2, v1) and (u2, v2).
 * NJD_SPRITE_HFLIP swaps u1 and u2, and NJD_SPRITE_VFLIP swaps v1 and v2 */
static inline void sprite_corners(const NJS_SPRITE *sp, Int n, Uint32 attr,
                                  Float y_scale, Float scale,
                                  struct sprite_corner corners[4])
{
    const NJS_TEXANIM *frame = &sp->tanim[n];
    Float u1 = frame->u1 * UV_SCALE;
    Float v1 = frame->v1 * UV_SCALE;
    Float u2 = frame->u2 * UV_SCALE;
    Float v2 = frame->v2 * UV_SCALE;
    Float x1 = (Float)-frame->cx * sp->sx * scale;
    Float y1 = (Float)-frame->cy * y_scale * scale;
    Float x2 = (Float)(frame->sx - frame->cx) * sp->sx * scale;
    Float y2 = (Float)(frame->sy - frame->cy) * y_scale * scale;
    Float swap;

    if (attr & NJD_SPRITE_HFLIP) {
        swap = u1;
        u1 = u2;
        u2 = swap;
    }
    if (attr & NJD_SPRITE_VFLIP) {
        swap = v1;
        v1 = v2;
        v2 = swap;
    }
    corners[0].u = u1;
    corners[0].v = v1;
    corners[1].u = u1;
    corners[1].v = v2;
    corners[2].u = u2;
    corners[2].v = v1;
    corners[3].u = u2;
    corners[3].v = v2;
    if (attr & NJD_SPRITE_ANGLE) {
        Float sine;
        Float cosine;

        _builtin_fsca(sp->ang, &sine, &cosine);
        corners[0].x = (x1 * cosine) - (y1 * sine);
        corners[0].y = (x1 * sine) + (y1 * cosine);
        corners[1].x = (x1 * cosine) - (y2 * sine);
        corners[1].y = (x1 * sine) + (y2 * cosine);
        corners[2].x = (x2 * cosine) - (y1 * sine);
        corners[2].y = (x2 * sine) + (y1 * cosine);
        corners[3].x = (x2 * cosine) - (y2 * sine);
        corners[3].y = (x2 * sine) + (y2 * cosine);
    } else {
        corners[0].x = x1;
        corners[0].y = y1;
        corners[1].x = x1;
        corners[1].y = y2;
        corners[2].x = x2;
        corners[2].y = y1;
        corners[3].x = x2;
        corners[3].y = y2;
    }
}

static inline void sprite_scale_x(struct sprite_corner corners[4], Float scale)
{
    corners[0].x *= scale;
    corners[1].x *= scale;
    corners[2].x *= scale;
    corners[3].x *= scale;
}

static inline void sprite_place(struct sprite_corner corners[4], Float x,
                                Float y, Float rhw)
{
    corners[0].x += x;
    corners[0].y += y;
    corners[0].rhw = rhw;
    corners[1].x += x;
    corners[1].y += y;
    corners[1].rhw = rhw;
    corners[2].x += x;
    corners[2].y += y;
    corners[2].rhw = rhw;
    corners[3].x += x;
    corners[3].y += y;
    corners[3].rhw = rhw;
}

/* whether all four corners lie past one edge of Ninja's 2D clip. the .src
 * files AND four outcodes, a bit for each edge a corner lies past. each
 * edge's test here stops at the first corner inside it, with the same
 * result. it is ALWAYS_INLINE because GCC lays out sprite_send_plane larger
 * otherwise */
ALWAYS_INLINE Bool sprite_off_screen(const struct sprite_corner corners[4])
{
    Float left = sa1_live_clip[0].x;
    Float top = sa1_live_clip[0].y;
    Float right = sa1_live_clip[1].x;
    Float bottom = sa1_live_clip[1].y;

    return (corners[0].y > bottom && corners[1].y > bottom &&
            corners[2].y > bottom && corners[3].y > bottom) ||
           (top > corners[0].y && top > corners[1].y && top > corners[2].y &&
            top > corners[3].y) ||
           (corners[0].x > right && corners[1].x > right &&
            corners[2].x > right && corners[3].x > right) ||
           (left > corners[0].x && left > corners[1].x && left > corners[2].x &&
            left > corners[3].x);
}

/* send corner as the vertex at sq, with control as its parameter control
 * word and an intensity of 1. return the next vertex's address. the asm
 * writes the vertex's words 6 down to 0, as the .src files do, and its PREF
 * sends the queue: a store to the store queues from registers, which SH4ZAM
 * has no entry for. it names no memory, since it writes only the store
 * queue area, which no C reads. it is ALWAYS_INLINE so that GCC's inline limits
 * never leave a call in a send */
ALWAYS_INLINE struct sprite_vertex *
sprite_send_corner(struct sprite_vertex *sq, const struct sprite_corner *corner,
                   Uint32 control)
{
    Uint32 *at = &sq->unwritten;

    __asm__ volatile("fmov.s %1,@-%0\n\t"
                     "fmov.s %2,@-%0\n\t"
                     "fmov.s %3,@-%0\n\t"
                     "fmov.s %4,@-%0\n\t"
                     "fmov.s %5,@-%0\n\t"
                     "fmov.s %6,@-%0\n\t"
                     "mov.l %7,@-%0\n\t"
                     "pref @%0"
                     : "+r"(at)
                     : "f"(1.0F), "f"(corner->v), "f"(corner->u),
                       "f"(corner->rhw), "f"(corner->y), "f"(corner->x),
                       "r"(control));
    /* at now addresses the vertex's first word, its control word */
    return (struct sprite_vertex *)at + 1;
}

static inline struct sprite_vertex *
sprite_send_quad(struct sprite_vertex *sq,
                 const struct sprite_corner corners[4])
{
    sq = sprite_send_corner(sq, &corners[0], KM_VERTEXPARAM_NORMAL);
    sq = sprite_send_corner(sq, &corners[1], KM_VERTEXPARAM_NORMAL);
    sq = sprite_send_corner(sq, &corners[2], KM_VERTEXPARAM_NORMAL);
    return sprite_send_corner(sq, &corners[3], KM_VERTEXPARAM_ENDOFSTRIP);
}

/* send frame n facing the screen from sq on, centered where view, a point in
 * the view, lands on the screen. return the vertex past the last, or NULL
 * when the frame isn't sent. the corners' y scales by y_scale. z is the depth
 * the clip tests and the corners' 1/z take.
 *
 * view's x and y scale by the screen's distance over view.z, and by
 * dreamcast.h's port_draw_aspect and Ninja's y aspect. the corners scale about
 * that point by the distance over view.z, and their x by port_draw_aspect too.
 * the .src files take Ninja's x aspect for x, and leave it out of the corners.
 * that changes nothing at 4:3, where both aspects are 1, but at 16:9's 3/4
 * would draw the frame a third too wide.
 *
 * the frame isn't sent when z lies past the far clip or nearer than the
 * near clip. it isn't sent either when all four corners lie past one edge
 * of the 2D clip */
static inline struct sprite_vertex *
sprite_send_facing_at(const NJS_SPRITE *sp, Int n, Uint32 attr, Float y_scale,
                      shz_vec3_t view, Float z, struct sprite_vertex *sq)
{
    struct sprite_corner corners[4];
    Float scale;

    if (z > sa1_live_far_clip || sa1_live_near_clip > z) {
        return NULL;
    }
    scale = sa1_live_ninja_screen.dist * (1.0F / view.z);
    sprite_corners(sp, n, attr, y_scale, scale, corners);
    sprite_scale_x(corners, port_draw_aspect);
    sprite_place(
        corners,
        (view.x * (port_draw_aspect * scale)) + sa1_live_ninja_screen.cx,
        (view.y * (sa1_live_aspect_y * scale)) + sa1_live_ninja_screen.cy,
        1.0F / z);
    if (sprite_off_screen(corners)) {
        return NULL;
    }
    return sprite_send_quad(sq, corners);
}

/* send frame n lying in the plane z = sp->p.z, about sp->p, from sq on. each
 * corner goes through the current matrix and takes its own 1/z. return the
 * vertex past the last, or NULL when the frame isn't sent.
 *
 * the frame isn't sent when all four corners lie past the far clip, or all
 * four nearer than the near clip. it isn't sent either when all four lie
 * past one edge of the 2D clip. when only some lie nearer than the near
 * clip, it goes as two triangles cut at the near clip. sa1/sprite.c, which
 * Ninja's entries' object includes, defines it */
__attribute__((noinline)) struct sprite_vertex *
sprite_send_plane(const NJS_SPRITE *sp, Int n, Uint32 attr,
                  struct sprite_vertex *sq);

/* send frame n on the screen, centered at sp->p.x and sp->p.y, from sq on, and
 * return the vertex past the last. each corner's 1/z is 1 where pri is over
 * -1, and -1/pri otherwise. njDrawPolygon2D gives the same 1/z at any pri but
 * NaN. where the 2D draws squeeze, port_wide_x squeezes the center, and the
 * corners' x scales about it by the aspect */
static inline struct sprite_vertex *sprite_send_2d(const NJS_SPRITE *sp, Int n,
                                                   Uint32 attr, Float pri,
                                                   struct sprite_vertex *sq)
{
    struct sprite_corner corners[4];
    Float depth = -pri;
    Float x = sp->p.x;
    Int squeeze = port_wide_squeezes();

    if (squeeze) {
        x = port_wide_x(x);
    }
    sprite_corners(sp, n, attr, sp->sy, 1.0F, corners);
    if (squeeze) {
        sprite_scale_x(corners, port_draw_aspect);
    }
    sprite_place(corners, x, sp->p.y, 1.0F > depth ? 1.0F : 1.0F / depth);
    return sprite_send_quad(sq, corners);
}

/* how a sprite goes, as an sa_draw_live of SA_SA1_SPRITE's state says */
enum sa1_live_sprite_how {
    /* on the screen, at pri bias */
    SA1_SPRITE_SCREEN,
    /* facing the screen at sp->p, through the current matrix, at the
     * point's depth */
    SA1_SPRITE_FACING,
    /* facing the screen, at the point's depth plus bias */
    SA1_SPRITE_FACING_BIASED,
    /* lying in the plane z = sp->p.z, through the current matrix */
    SA1_SPRITE_PLANE
};

/* an sa_draw_live of SA_SA1_SPRITE's state: how the sprite goes, its frame,
 * its NJD_SPRITE_ flags, and the y scale and bias a facing or screen
 * frame takes */
struct sa1_live_sprite {
    Uint32 how;
    Int n;
    Uint32 attr;
    Float y_scale;
    Float bias;
};

/* the strip a sprite's draw sends into: the store queue address of its next
 * vertex, which the layer sets once it opens the strip, and which the draw
 * leaves past the last vertex it sent, or NULL where it sent none. each
 * object that includes this has its own, which only the layer's draw and
 * the call inside it read, so GCC keeps it in registers */
static struct sprite_vertex *sa1_live_strip __attribute__((unused));

/* draw frame state->n of sp as state says, into sa1_live_strip's strip */
ALWAYS_INLINE void sa1_live_sprite(const NJS_SPRITE *sp,
                                   const struct sa1_live_sprite *state)
{
    struct sprite_vertex *sq = sa1_live_strip;
    shz_vec3_t view;

    if (state->how == SA1_SPRITE_SCREEN) {
        sq = sprite_send_2d(sp, state->n, state->attr, state->bias, sq);
    } else if (state->how == SA1_SPRITE_PLANE) {
        sq = sprite_send_plane(sp, state->n, state->attr, sq);
    } else {
        view = shz_xmtrx_transform_point3(zam_load_vec3(&sp->p));
        sq = sprite_send_facing_at(
            sp, state->n, state->attr, state->y_scale, view,
            state->how == SA1_SPRITE_FACING_BIASED ? view.z + state->bias
                                                   : view.z,
            sq);
    }
    sa1_live_strip = sq;
}

/* draw a sprite as its state says, consumed in the call, as live.h's
 * sa_present_live draws a basic model: each call compiles into the layer's
 * draw, with the descriptor and its state in registers. the call compiles
 * into a draw only where its format's drawer does, and the sprites' layers
 * compile no model walk, so this header gives the Dreamcast's call for
 * SA_SA1_SPRITE, and live.h for SA_SA1_BASIC; boundary.py fails an object
 * including both, which would define the call twice. the engine goes unread */
ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    (void)engine;
    if (draw->format != SA_SA1_SPRITE) {
        return SA_ERR_UNSUPPORTED;
    }
    sa1_live_sprite(draw->geometry, draw->state);
    return SA_OK;
}

#endif
