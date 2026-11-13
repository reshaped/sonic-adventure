/* SA1's sprites' out-of-line send on the Dreamcast: a frame lying in its
 * plane, and the near clip that cuts it, which sprite.h's sa1_live_sprite
 * calls. Ninja's sprite entries, engine/dc/njSprite.c, include it, so it
 * links in their object, where njSprite.src's sprite_emit_transformed lay,
 * and the game's sprites call it from theirs. it reads Ninja's state only
 * through the names screen.h lends */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <sh4zam/shz_xmtrx.h>

#include "sa1/sprite.h"

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
    Float near; /* the near clip */
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
    Float scale_x = port_draw_aspect * sa1_live_ninja_screen.dist;
    Float scale_y = sa1_live_aspect_y * sa1_live_ninja_screen.dist;
    Float to_view_x = 1.0F / scale_x;
    Float to_view_y = 1.0F / scale_y;

    clip.near = sa1_live_near_clip;
    clip.rhw = 1.0F / clip.near;
    clip.scale_x = scale_x * clip.rhw;
    clip.scale_y = scale_y * clip.rhw;
    clip.cx = sa1_live_ninja_screen.cx;
    clip.cy = sa1_live_ninja_screen.cy;
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
    corner->x = (view.x * rhw * scale_x) + sa1_live_ninja_screen.cx;
    corner->y = (view.y * rhw * scale_y) + sa1_live_ninja_screen.cy;
    corner->rhw = rhw;
}

struct sprite_vertex *sprite_send_plane(const NJS_SPRITE *sp, Int n,
                                        Uint32 attr, struct sprite_vertex *sq)
{
    struct sprite_corner corners[4];
    struct sprite_corner cut[4];
    shz_vec3_t views[4];
    Float far = sa1_live_far_clip;
    Float near = sa1_live_near_clip;
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
    scale_x = sa1_live_ninja_screen.dist * port_draw_aspect;
    scale_y = sa1_live_ninja_screen.dist * sa1_live_aspect_y;
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
