/* Ninja's sprites: a frame of an NJS_SPRITE's texture animation, drawn as
 * textured vertices sent straight to the tile accelerator's lists. make gcc
 * compiles this in place of sdk/ninja/njSprite.src, whose
 * sprite_emit_billboard, sprite_emit_transformed and sprite_emit_2d are
 * sprite_send_facing, sprite_send_plane and sprite_send_2d here.
 *
 * a draw starts a strip with sprite_begin, which writes the strip's polygon
 * header. a send then writes the strip's vertices through the store queues,
 * and njEndVertex ends the strip */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "njsprite.h"

/* the points sprite_clip_triangle sends of a triangle, indexed by its corners
 * nearer than the near clip: 4 for its first corner, 2 for its second and 1
 * for its third. side 0 runs from the third corner to the first. side 1 runs
 * from the first to the second, and side 2 from the second to the third.
 * CLIP_CROSSING(side) names the crossing, the point where the side meets the
 * near clip, and CLIP_CORNER(side) the corner the side ends at. an entry
 * names three points, or four with CLIP_FOUR, or none */
#define CLIP_CROSSING(side) (1U << (2 * (side)))
#define CLIP_CORNER(side) (2U << (2 * (side)))
#define CLIP_FOUR 0x80
static const Uint8 sprite_clip_orders[8] = {
    CLIP_CORNER(0) | CLIP_CORNER(1) | CLIP_CORNER(2),
    CLIP_CROSSING(0) | CLIP_CORNER(0) | CLIP_CORNER(1) | CLIP_CROSSING(2) |
        CLIP_FOUR,
    CLIP_CORNER(0) | CLIP_CROSSING(1) | CLIP_CROSSING(2) | CLIP_CORNER(2) |
        CLIP_FOUR,
    CLIP_CROSSING(0) | CLIP_CORNER(0) | CLIP_CROSSING(1),
    CLIP_CROSSING(0) | CLIP_CROSSING(1) | CLIP_CORNER(1) | CLIP_CORNER(2) |
        CLIP_FOUR,
    CLIP_CROSSING(1) | CLIP_CORNER(1) | CLIP_CROSSING(2),
    CLIP_CROSSING(0) | CLIP_CROSSING(2) | CLIP_CORNER(2),
    0,
};

/* what sprite_clip_triangle places a crossing by */
struct sprite_clip {
    Float near; /* _nj_near_clip_ */
    Float rhw; /* the crossings' 1/z: 1/near, until
                * sprite_clip_triangle's @bug overwrites it */
    Float scale_x, scale_y; /* from a view x and y at z = near to the screen */
    Float cx, cy; /* the screen's center */
};

/* send the triangle of corners as the points order names, from sq on, and
 * return the vertex past the last. views holds the corners' places in the
 * view. of four points p1 to p4, p3 waits to end the strip, so the strip goes
 * p1, p2, p4, p3 */
static struct sprite_vertex *
sprite_clip_triangle(struct sprite_vertex *sq, Uint32 order,
                     const struct sprite_corner corners[3],
                     const shz_vec3_t views[3], struct sprite_clip *clip)
{
    struct sprite_corner points[4];
    int count = 0;

    for (int side = 0; side < 3; side++) {
        int from = side == 0 ? 2 : side - 1;
        const shz_vec3_t *start = &views[from];
        const shz_vec3_t *end = &views[side];
        const struct sprite_corner *first = &corners[from];
        const struct sprite_corner *last = &corners[side];

        if (order & CLIP_CROSSING(side)) {
            struct sprite_corner *crossing = &points[count++];
            Float t = (clip->near - start->z) / (end->z - start->z);

            crossing->x =
                ((((end->x - start->x) * t) + start->x) * clip->scale_x) +
                clip->cx;
            crossing->y =
                ((((end->y - start->y) * t) + start->y) * clip->scale_y) +
                clip->cy;
            crossing->rhw = clip->rhw;
            crossing->u = ((last->u - first->u) * t) + first->u;
            crossing->v = ((last->v - first->v) * t) + first->v;
        }
        if (order & CLIP_CORNER(side)) {
            points[count++] = *last;
        }
    }
    if (count == 0) {
        return sq;
    }
    sq = sprite_send_corner(sq, &points[0], KM_VERTEXPARAM_NORMAL);
    sq = sprite_send_corner(sq, &points[1], KM_VERTEXPARAM_NORMAL);
    if (!(order & CLIP_FOUR)) {
        return sprite_send_corner(sq, &points[2], KM_VERTEXPARAM_ENDOFSTRIP);
    }
    /* @bug logic: the waiting point's 1/z overwrites the crossings' 1/near.
     * when a corner of the first triangle waits, the second triangle's
     * crossings take that corner's 1/z in place of 1/near. njSprite.src loads
     * it over 1/near at 0x8c623fd0, and at 0x8ce0db64 in LOADER.BIN, and
     * sprite.src at 0x8c0a4b44 */
    clip->rhw = points[2].rhw;
    sq = sprite_send_corner(sq, &points[3], KM_VERTEXPARAM_NORMAL);
    return sprite_send_corner(sq, &points[2], KM_VERTEXPARAM_ENDOFSTRIP);
}

/* send the quad of corners as two triangles cut at the near clip, its first
 * three corners and then its last three, from sq on. return the vertex past
 * the last. nearer has a bit for each corner nearer than the clip, 8 for the
 * first down to 1 for the last. a crossing lies between its side's ends in
 * the view. the corners' x, y and 1/z on the screen give back their places in
 * the view */
static struct sprite_vertex *
sprite_clip_near(struct sprite_vertex *sq, Uint32 nearer,
                 const struct sprite_corner corners[4])
{
    shz_vec3_t views[4];
    struct sprite_clip clip;
    Float scale_x = port_draw_aspect * _nj_screen_.dist;
    Float scale_y = _nj_scr_aspect_y_ * _nj_screen_.dist;
    Float to_view_x = 1.0F / scale_x;
    Float to_view_y = 1.0F / scale_y;

    clip.near = _nj_near_clip_;
    clip.rhw = 1.0F / clip.near;
    clip.scale_x = scale_x * clip.rhw;
    clip.scale_y = scale_y * clip.rhw;
    clip.cx = _nj_screen_.cx;
    clip.cy = _nj_screen_.cy;
    for (int i = 0; i < 4; i++) {
        Float z = 1.0F / corners[i].rhw;

        views[i].x = (corners[i].x - clip.cx) * to_view_x * z;
        views[i].y = (corners[i].y - clip.cy) * to_view_y * z;
        views[i].z = z;
    }
    sq = sprite_clip_triangle(sq, sprite_clip_orders[nearer >> 1], &corners[0],
                              &views[0], &clip);
    return sprite_clip_triangle(sq, sprite_clip_orders[nearer & 7], &corners[1],
                                &views[1], &clip);
}

/* corner's place in the view: sp->p moved by its x and y, through the
 * current matrix */
static inline shz_vec3_t sprite_view(const NJS_SPRITE *sp,
                                     const struct sprite_corner *corner)
{
    return shz_xmtrx_transform_point3(
        shz_vec3_init(corner->x + sp->p.x, corner->y + sp->p.y, sp->p.z));
}

/* put corner where view lands on the screen, and give it the 1/z rhw */
static inline void sprite_project(struct sprite_corner *corner, shz_vec3_t view,
                                  Float rhw, Float scale_x, Float scale_y)
{
    corner->x = (view.x * rhw * scale_x) + _nj_screen_.cx;
    corner->y = (view.y * rhw * scale_y) + _nj_screen_.cy;
    corner->rhw = rhw;
}

struct sprite_vertex *sprite_send_plane(const NJS_SPRITE *sp, Int n,
                                        Uint32 attr, struct sprite_vertex *sq)
{
    struct sprite_corner corners[4];
    struct sprite_corner cut[4];
    shz_vec3_t views[4];
    Float far = _nj_far_clip_;
    Float near = _nj_near_clip_;
    Float scale_x;
    Float scale_y;
    Uint32 nearer;

    sprite_corners(sp, n, attr, sp->sy, 1.0F, corners);
    views[0] = sprite_view(sp, &corners[0]);
    views[1] = sprite_view(sp, &corners[1]);
    views[2] = sprite_view(sp, &corners[2]);
    views[3] = sprite_view(sp, &corners[3]);
    if (views[0].z > far && views[1].z > far && views[2].z > far &&
        views[3].z > far) {
        return NULL;
    }
    if (near > views[0].z && near > views[1].z && near > views[2].z &&
        near > views[3].z) {
        return NULL;
    }
    scale_x = _nj_screen_.dist * port_draw_aspect;
    scale_y = _nj_screen_.dist * _nj_scr_aspect_y_;
    if (near > views[0].z || near > views[1].z || near > views[2].z ||
        near > views[3].z) {
        nearer = (Uint32)(near > views[0].z) << 3 |
                 (Uint32)(near > views[1].z) << 2 |
                 (Uint32)(near > views[2].z) << 1 | (Uint32)(near > views[3].z);
        /* a corner of a quad the near clip cuts can lie behind the eye, and
         * its 1/z keeps its sign. sprite_clip_near takes a copy, cut, so
         * that corners can stay in registers. an array whose address leaves
         * the function lives in memory. with corners passed in cut's place,
         * this function takes 16 more instructions, and a frame 4 bytes
         * larger */
        cut[0] = corners[0];
        cut[1] = corners[1];
        cut[2] = corners[2];
        cut[3] = corners[3];
        sprite_project(&cut[0], views[0], 1.0F / views[0].z, scale_x, scale_y);
        sprite_project(&cut[1], views[1], 1.0F / views[1].z, scale_x, scale_y);
        sprite_project(&cut[2], views[2], 1.0F / views[2].z, scale_x, scale_y);
        sprite_project(&cut[3], views[3], 1.0F / views[3].z, scale_x, scale_y);
        if (sprite_off_screen(cut)) {
            return NULL;
        }
        return sprite_clip_near(sq, nearer, cut);
    }
    sprite_project(&corners[0], views[0], 1.0F / views[0].z, scale_x, scale_y);
    sprite_project(&corners[1], views[1], 1.0F / views[1].z, scale_x, scale_y);
    sprite_project(&corners[2], views[2], 1.0F / views[2].z, scale_x, scale_y);
    sprite_project(&corners[3], views[3], 1.0F / views[3].z, scale_x, scale_y);
    if (sprite_off_screen(corners)) {
        return NULL;
    }
    return sprite_send_quad(sq, corners);
}

/* do nothing, since the sprites keep no state */
void njInitSprite(void)
{
}

/* send frame n facing the screen at sp->p, through the current matrix, as
 * sprite_send_facing_at does. its y scale is sp->sy, and its z is the
 * point's z */
static struct sprite_vertex *sprite_send_facing(const NJS_SPRITE *sp, Int n,
                                                Uint32 attr,
                                                struct sprite_vertex *sq)
{
    shz_vec3_t view = shz_xmtrx_transform_point3(zam_load_vec3(&sp->p));

    return sprite_send_facing_at(sp, n, attr, sp->sy, view, view.z, sq);
}

Int (*port_sprite_record)(enum present_kind kind, const void *what, Int count,
                          Int a, Float pri, Uint32 attr) = NULL;

/* port.h's picture aspect and pillarbox, and the squeeze of the 2D draws,
 * its test and the cover rule. they lie here, not in wide.c, since
 * LOADER.BIN links this file's object but not wide.c's */
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

Int port_wide_covers(const Float *x, Int count, Int stride)
{
    Int left = FALSE;
    Int right = FALSE;

    if (port_pillarbox) {
        return FALSE;
    }
    for (; count > 0; count--, x += stride) {
        if (*x >= -1.0F && *x <= 1.0F) {
            left = TRUE;
        } else if (*x >= 639.0F && *x <= 641.0F) {
            right = TRUE;
        } else {
            return FALSE;
        }
    }
    return left && right;
}

/* send frame n on the screen, centered at sp->p.x and sp->p.y, from sq on, and
 * return the vertex past the last. each corner's 1/z is 1 where pri is over
 * -1, and -1/pri otherwise. njDrawPolygon2D gives the same 1/z at any pri but
 * NaN. where the 2D draws squeeze, port_wide_x squeezes the center, and the
 * corners' x scales about it by the aspect */
static struct sprite_vertex *sprite_send_2d(const NJS_SPRITE *sp, Int n,
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

/* draw frame n of sp's texture animation on the screen. the frame's center
 * lies at sp->p.x and sp->p.y, in pixels. its 1/z is 1 where pri is over -1,
 * and -1/pri otherwise. NJD_SPRITE_ALPHA sends it to the translucent list.
 * NJD_SPRITE_COLOR colors the strip with _nj_constant_material_ in place of
 * white */
void njDrawSprite2D(NJS_SPRITE *sp, Int n, Float pri, Uint32 attr)
{
    struct sprite_vertex *sq;

    if (port_sprite_record != NULL) {
        port_sprite_record(PRESENT_SPRITE_2D, sp, 0, n, pri, attr);
    }
    njGetkmPointer();
    sq = sprite_send_2d(sp, n, attr, pri, sprite_begin(sp, n, attr));
    njEndVertex((Uint32 *)sq);
    njSetkmPointer();
}

/* draw frame n of sp's texture animation at sp->p, through the current
 * matrix. with NJD_SPRITE_SCALE the frame faces the screen, as
 * sprite_send_facing sends it. without it the frame lies in the plane
 * z = sp->p.z, as sprite_send_plane sends it. NJD_SPRITE_ALPHA and
 * NJD_SPRITE_COLOR act as in njDrawSprite2D. when the frame isn't sent,
 * njEndVertex isn't called, and the list's write pointer stays at the
 * header's start. where port_sprite_record returns FALSE, it does
 * nothing */
void njDrawSprite3D(NJS_SPRITE *sp, Int n, Uint32 attr)
{
    struct sprite_vertex *sq;

    if (port_sprite_record != NULL &&
        !port_sprite_record(PRESENT_NJ_SPRITE, sp, 0, n, 0.0F, attr)) {
        return;
    }
    njGetkmPointer();
    sq = sprite_begin(sp, n, attr);
    if (attr & NJD_SPRITE_SCALE) {
        sq = sprite_send_facing(sp, n, attr, sq);
    } else {
        sq = sprite_send_plane(sp, n, attr, sq);
    }
    if (sq != NULL) {
        njEndVertex((Uint32 *)sq);
    }
    njSetkmPointer();
}
