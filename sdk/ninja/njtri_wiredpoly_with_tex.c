/* outline drawing of a clipped, textured polygon */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* store a word in the store queue, just below the word stored before it */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

/* store a KMVERTEX3 through the store queue, from uBaseRGB down to its control
 * word, and send it. uOffsetRGB goes unwritten, and sq ends past the vertex */
#define PUT_VERTEX(control, x, y, inv_z, u, v, col)                            \
    do {                                                                       \
        sq += 7;                                                               \
        SQ_PUT(Uint32, col);                                                   \
        SQ_PUT(Float, v);                                                      \
        SQ_PUT(Float, u);                                                      \
        SQ_PUT(Float, inv_z);                                                  \
        SQ_PUT(Float, y);                                                      \
        SQ_PUT(Float, x);                                                      \
        SQ_PUT(Uint32, control);                                               \
        prefetch(sq);                                                          \
        sq += 8;                                                               \
    } while (0)

/* how far an edge's strip reaches to one side of the edge, in pixels. it
 * reaches 2 * LINE_SIDE to the other side, so it is 3 * LINE_SIDE wide */
#define LINE_SIDE 0.525F

/* draw njtri_wiredpoly's outline, but as strips of four KMVERTEX3. each point
 * takes its u and v from p->tex, in units of 1/256, and leaves uOffsetRGB
 * unwritten. p->col and p->tex must not be null */
void njtri_wiredpoly_with_tex(Int n, NJS_POINT3COL *p, Float **vertex)
{
    /* @hack spelling: i set to 0 here and incremented where the closing edge
     * joins the loop (ADD #1,R7 at 0x8c6256c4 in 1ST_READ.BIN); as Int i = 1,
     * the function is 16 bytes shorter and 297 of 550 words differ */
    Int i = 0;
    Int last = n;
    Float tex_scale = 1.0F / 256.0F;
    Float scale_x;
    Float scale_y;
    Float inv_z0;
    Float inv_z1;
    Float x0;
    Float x1;
    Float y0;
    Float y1;
    Float dx;
    Float dy;
    Float inv_length;
    Float side_x;
    Float side_y;
    Float wide_x;
    Float wide_y;
    Float u0;
    Float v0;
    Float u1;
    Float v1;
    Uint32 col0;
    Uint32 col1;
    /* @hack redundant: register on sq, which keeps i in R7, last in R13 and
     * the frame at 112 bytes (0x8c6254ba, 0x8c6254c2, 0x8c6254da in
     * 1ST_READ.BIN); without it, i and last take R14 and R11, the frame is 108
     * bytes, the function is 8 bytes shorter, and 348 of 550 words differ */
    register Float *sq;

    /* @hack spelling: last decremented here, not in its initializer, which
     * keeps the closing edge's inv_z0 in FR11 and inv_z1 in FR10 (0x8c62552e,
     * 0x8c62553e in 1ST_READ.BIN); as Int last = n - 1, the two trade and 45 of
     * 550 words differ */
    --last;
    scale_x = _nj_screen_.dist * _nj_scr_aspect_x_;
    scale_y = _nj_screen_.dist * _nj_scr_aspect_y_;
    sq = *vertex;
    if (!(p->p[0].z == _nj_near_clip_ && p->p[last].z == _nj_near_clip_)) {
        inv_z0 = 1.0F / p->p[0].z;
        inv_z1 = 1.0F / p->p[last].z;
        x0 = (p->p[0].x * scale_x * inv_z0) + _nj_screen_.cx;
        x1 = (p->p[last].x * scale_x * inv_z1) + _nj_screen_.cx;
        y0 = (p->p[0].y * scale_y * inv_z0) + _nj_screen_.cy;
        y1 = (p->p[last].y * scale_y * inv_z1) + _nj_screen_.cy;
        dx = x0 - x1;
        dy = y0 - y1;
        inv_length = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
        side_x = dy * inv_length * LINE_SIDE;
        side_y = -(dx * inv_length * LINE_SIDE);
        wide_x = side_x * 2.0F;
        wide_y = side_y * 2.0F;
        col0 = p->col[0].color;
        col1 = p->col[last].color;
        u0 = p->tex[0].tex.u * tex_scale;
        v0 = p->tex[0].tex.v * tex_scale;
        u1 = p->tex[last].tex.u * tex_scale;
        v1 = p->tex[last].tex.v * tex_scale;
        /* @hack redundant: a store to v0 that never runs, which keeps u0 at
         * @(88,R15) and v0 at @(92,R15) (MOV #88,R0 at 0x8c6255ea, MOV #92,R0
         * at 0x8c6255fa in 1ST_READ.BIN); without it, the two trade and 6 of
         * 550 words differ */
        if (0) {
            v0 = 0.0F;
        }
        /* @hack redundant: a second store to col1 that never runs, which
         * keeps p->col at @R15, last * 4 at @(8,R15) and p->tex at @(4,R15)
         * (0x8c6255d2, 0x8c6255d6, 0x8c6255e0 in 1ST_READ.BIN); without it,
         * those slots trade here and in the loop, and 23 of 550 words differ */
        if (0) {
            col1 = p->col[last].color;
        }
        PUT_VERTEX(KM_VERTEXPARAM_NORMAL, x0 - side_x, y0 - side_y, inv_z0, u0,
                   v0, col0);
        PUT_VERTEX(KM_VERTEXPARAM_NORMAL, x1 - side_x, y1 - side_y, inv_z1, u1,
                   v1, col1);
        PUT_VERTEX(KM_VERTEXPARAM_NORMAL, x0 + wide_x, y0 + wide_y, inv_z0, u0,
                   v0, col0);
        PUT_VERTEX(KM_VERTEXPARAM_ENDOFSTRIP, x1 + wide_x, y1 + wide_y, inv_z1,
                   u1, v1, col1);
    }
    ++i;
    do {
        /* @hack spelling: the empty branch for an edge the clipping made, with
         * the edge drawn in the else, which keeps p->p in R3 and (i - 1) * 12
         * in R2 (0x8c625714 to 0x8c625726 in 1ST_READ.BIN); as one negated
         * test, the two trade and 14 of 550 words differ */
        if (p->p[i].z == _nj_near_clip_ && p->p[i - 1].z == _nj_near_clip_) {
        } else {
            inv_z0 = 1.0F / p->p[i - 1].z;
            inv_z1 = 1.0F / p->p[i].z;
            x0 = (p->p[i - 1].x * scale_x * inv_z0) + _nj_screen_.cx;
            x1 = (p->p[i].x * scale_x * inv_z1) + _nj_screen_.cx;
            y0 = (p->p[i - 1].y * scale_y * inv_z0) + _nj_screen_.cy;
            y1 = (p->p[i].y * scale_y * inv_z1) + _nj_screen_.cy;
            dx = x0 - x1;
            dy = y0 - y1;
            inv_length = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
            side_x = dy * inv_length * LINE_SIDE;
            side_y = -(dx * inv_length * LINE_SIDE);
            wide_x = side_x * 2.0F;
            wide_y = side_y * 2.0F;
            /* @hack spelling: the texture coordinates read before the colors,
             * the reverse of the closing edge's order, which keeps the frame
             * at 112 bytes (0x8c6254da in 1ST_READ.BIN); in the closing edge's
             * order, it is 116 bytes and 168 of 550 words differ */
            u0 = p->tex[i - 1].tex.u * tex_scale;
            v0 = p->tex[i - 1].tex.v * tex_scale;
            u1 = p->tex[i].tex.u * tex_scale;
            v1 = p->tex[i].tex.v * tex_scale;
            col0 = p->col[i - 1].color;
            col1 = p->col[i].color;
            /* @hack redundant: a store to u0 and inv_z0 that never runs, which
             * keeps i * 4 in R8 and (i - 1) * 4 in R9 (0x8c6256c6 to
             * 0x8c6256ce in 1ST_READ.BIN), wide_y at @(48,R15) and u0 at
             * @(52,R15), and the closing edge's wide_x and wide_y at @(96,R15)
             * and @(100,R15); without it, each pair trades and 23 of 550 words
             * differ */
            if (0) {
                u0 = inv_z0 = 0.0F;
            }
            PUT_VERTEX(KM_VERTEXPARAM_NORMAL, x0 - side_x, y0 - side_y, inv_z0,
                       u0, v0, col0);
            PUT_VERTEX(KM_VERTEXPARAM_NORMAL, x1 - side_x, y1 - side_y, inv_z1,
                       u1, v1, col1);
            PUT_VERTEX(KM_VERTEXPARAM_NORMAL, x0 + wide_x, y0 + wide_y, inv_z0,
                       u0, v0, col0);
            PUT_VERTEX(KM_VERTEXPARAM_ENDOFSTRIP, x1 + wide_x, y1 + wide_y,
                       inv_z1, u1, v1, col1);
        }
    } while (++i <= last);
    *vertex = sq;
}
