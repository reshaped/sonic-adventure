/* the code Ninja's sprites and the game's share: a frame's corners, its
 * clip tests, and the strip that sends it. njSprite.c replaces
 * sdk/ninja/njSprite.src, and gcc/src/draw/sprite.c replaces
 * src/draw/sprite.src. each .src file holds its own copy of this code, and
 * the two differ only in the facing send's y scale and z. njSprite.c defines
 * the plane send, which sprite.c calls too.
 *
 * the C does the .src files' float arithmetic in their order. make gcc's
 * flags fuse no product and sum into an FMAC, and GCC takes each 1/z by
 * FDIV, so each result rounds as theirs does. the only arithmetic the C adds
 * scales x by the picture's aspect for 16:9, and changes nothing at 4:3,
 * where that aspect is 1 */
#ifndef GCC_SDK_NINJA_NJSPRITE_H
#define GCC_SDK_NINJA_NJSPRITE_H
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

/* NJS_TEXANIM's u1, v1, u2 and v2 count 256 to the texture's side */
#define UV_SCALE (1.0F / 256.0F)

/* njSetkmColorMode's mode for a sprite's strip in list, COLOR_MODE_OPAQUE or
 * COLOR_MODE_TRANS: textured, with each vertex's intensity scaling the
 * header's color. it leaves out the COLOR_MODE_GOURAUD bit that both lists'
 * modes hold */
#define SPRITE_MODE(list)                                                      \
    (((list) | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY) & ~COLOR_MODE_GOURAUD)

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

/* njDrawSprite2D's and njDrawSprite3D's way to port.h's present_record.
 * LOADER.BIN links njSprite.c's object but not present.c's, so the two call
 * through this, which present_end points at present.c's present_sprite, and
 * which the loader leaves null. present_sprite calls present_record, and
 * returns FALSE where njDrawSprite3D is to draw nothing: in the second
 * picture's pass, a sprite for a task whose step drew one after step 0 */
extern Int (*port_sprite_record)(enum present_kind kind, const void *what,
                                 Int count, Int a, Float pri, Uint32 attr);

/* the color a sprite takes without NJD_SPRITE_COLOR */
static const NJS_ARGB sprite_white = {1.0F, 1.0F, 1.0F, 1.0F};

/* make frame n's texture current, and set the color mode of the list
 * NJD_SPRITE_ALPHA picks. write a strip's polygon header at the end of that
 * list, in _nj_constant_material_'s color with NJD_SPRITE_COLOR and in
 * sprite_white without. return the store queue address for the strip's first
 * vertex */
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

/* whether all four corners lie past one edge of _nj_2d_clip_. the .src
 * files AND four outcodes, a bit for each edge a corner lies past. each
 * edge's test here stops at the first corner inside it, with the same
 * result. it is ALWAYS_INLINE because GCC lays out sprite_send_plane larger
 * otherwise */
ALWAYS_INLINE Bool sprite_off_screen(const struct sprite_corner corners[4])
{
    Float left = _nj_2d_clip_[0].x;
    Float top = _nj_2d_clip_[0].y;
    Float right = _nj_2d_clip_[1].x;
    Float bottom = _nj_2d_clip_[1].y;

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
 * sends the queue. it names no memory, since it writes only the store queue
 * area, which no C reads. it is ALWAYS_INLINE so that GCC's inline limits
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
 * view's x and y scale by the screen's distance over view.z, and by port.h's
 * port_draw_aspect and _nj_scr_aspect_y_. the corners scale about that point
 * by the distance over view.z, and their x by port_draw_aspect too. the .src
 * files take _nj_scr_aspect_x_ for x, and leave it out of the corners. that
 * changes nothing at 4:3, where both aspects are 1, but at 16:9's 3/4 would
 * draw the frame a third too wide.
 *
 * the frame isn't sent when z lies past _nj_far_clip_ or nearer than
 * _nj_near_clip_. it isn't sent either when all four corners lie past one
 * edge of _nj_2d_clip_ */
static inline struct sprite_vertex *
sprite_send_facing_at(const NJS_SPRITE *sp, Int n, Uint32 attr, Float y_scale,
                      shz_vec3_t view, Float z, struct sprite_vertex *sq)
{
    struct sprite_corner corners[4];
    Float scale;

    if (z > _nj_far_clip_ || _nj_near_clip_ > z) {
        return NULL;
    }
    scale = _nj_screen_.dist * (1.0F / view.z);
    sprite_corners(sp, n, attr, y_scale, scale, corners);
    sprite_scale_x(corners, port_draw_aspect);
    sprite_place(
        corners, (view.x * (port_draw_aspect * scale)) + _nj_screen_.cx,
        (view.y * (_nj_scr_aspect_y_ * scale)) + _nj_screen_.cy, 1.0F / z);
    if (sprite_off_screen(corners)) {
        return NULL;
    }
    return sprite_send_quad(sq, corners);
}

/* send frame n lying in the plane z = sp->p.z, about sp->p, from sq on. each
 * corner goes through the current matrix and takes its own 1/z. return the
 * vertex past the last, or NULL when the frame isn't sent.
 *
 * the frame isn't sent when all four corners lie past _nj_far_clip_, or all
 * four nearer than _nj_near_clip_. it isn't sent either when all four lie
 * past one edge of _nj_2d_clip_. when only some lie nearer than the near
 * clip, it goes as two triangles cut at the near clip */
__attribute__((noinline)) struct sprite_vertex *
sprite_send_plane(const NJS_SPRITE *sp, Int n, Uint32 attr,
                  struct sprite_vertex *sq);

#endif
