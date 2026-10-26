/* outline drawing of a clipped polygon */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* store a word in the store queue, just below the word stored before it */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

/* how far an edge's strip reaches to one side of the edge, in pixels. it
 * reaches 2 * LINE_SIDE to the other side, so it is 3 * LINE_SIDE wide */
#define LINE_SIDE 0.525F

/* draw the outline of the n points of p through the store queue at *vertex,
 * as a strip of four KMVERTEX0 for each edge. n must be at least 2, and p->col
 * must not be null. the closing edge, from the last point to the first, goes
 * first, and then the edge from each point to the next. an edge with both ends
 * at z = _nj_near_clip_, which a near clip adds, is skipped. *vertex ends past
 * the strips */
void njtri_wiredpoly(Int n, NJS_POINT3COL *p, Float **vertex)
{
    /* @hack spelling: i set to 0 here and incremented where the closing edge
     * joins the loop (ADD #1,R7 at 0x8c6252dc in 1ST_READ.BIN); as Int i = 1,
     * the function is 16 bytes shorter and 228 of 420 words differ */
    Int i = 0;
    Int last = n - 1;
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
    Uint32 col0;
    Uint32 col1;
    /* @hack redundant: register on sq, which keeps i in R7 and last in R13
     * (MOV #0,R7 at 0x8c625152, ADD #-1,R13 at 0x8c62515e in
     * 1ST_READ.BIN); without it, i and
     * last take R14 and R11, the function is 2 bytes shorter, and 247 of 420
     * words differ */
    register Float *sq;
    NJS_POINT3 *first;

    scale_x = _nj_screen_.dist * _nj_scr_aspect_x_;
    scale_y = _nj_screen_.dist * _nj_scr_aspect_y_;
    sq = *vertex;
    if (!(p->p[0].z == _nj_near_clip_ && p->p[last].z == _nj_near_clip_)) {
        /* @hack spelling: the first point read through first, which keeps the
         * closing edge's inv_z0 in FR10 and inv_z1 in FR9 (FDIV FR3,FR10 at
         * 0x8c6251c8, FDIV FR3,FR9 at 0x8c6251d8 in 1ST_READ.BIN); through
         * p->p[0], the two trade and 94 of 420 words differ */
        first = p->p;
        inv_z0 = 1.0F / first->z;
        inv_z1 = 1.0F / p->p[last].z;
        x0 = (first->x * scale_x * inv_z0) + _nj_screen_.cx;
        x1 = (p->p[last].x * scale_x * inv_z1) + _nj_screen_.cx;
        y0 = (first->y * scale_y * inv_z0) + _nj_screen_.cy;
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
        sq += 7;
        SQ_PUT(Uint32, col0);
        sq -= 2;
        SQ_PUT(Float, inv_z0);
        SQ_PUT(Float, y0 - side_y);
        SQ_PUT(Float, x0 - side_x);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8 + 7;
        SQ_PUT(Uint32, col1);
        sq -= 2;
        SQ_PUT(Float, inv_z1);
        SQ_PUT(Float, y1 - side_y);
        SQ_PUT(Float, x1 - side_x);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8 + 7;
        SQ_PUT(Uint32, col0);
        sq -= 2;
        SQ_PUT(Float, inv_z0);
        SQ_PUT(Float, y0 + wide_y);
        SQ_PUT(Float, x0 + wide_x);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8 + 7;
        SQ_PUT(Uint32, col1);
        sq -= 2;
        SQ_PUT(Float, inv_z1);
        SQ_PUT(Float, y1 + wide_y);
        SQ_PUT(Float, x1 + wide_x);
        SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
        prefetch(sq);
        sq += 8;
    }
    i++;
    do {
        /* @hack spelling: the empty branch for an edge the clipping made, with
         * the edge drawn in the else, which keeps p->p in R3 and (i - 1) * 12
         * in R2 (0x8c62532c to 0x8c625344 in 1ST_READ.BIN); as one negated
         * test, the two trade and 13 of 420 words differ */
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
            col0 = p->col[i - 1].color;
            col1 = p->col[i].color;
            sq += 7;
            SQ_PUT(Uint32, col0);
            sq -= 2;
            SQ_PUT(Float, inv_z0);
            SQ_PUT(Float, y0 - side_y);
            SQ_PUT(Float, x0 - side_x);
            SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
            prefetch(sq);
            sq += 8 + 7;
            SQ_PUT(Uint32, col1);
            sq -= 2;
            SQ_PUT(Float, inv_z1);
            SQ_PUT(Float, y1 - side_y);
            SQ_PUT(Float, x1 - side_x);
            SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
            prefetch(sq);
            sq += 8 + 7;
            SQ_PUT(Uint32, col0);
            sq -= 2;
            SQ_PUT(Float, inv_z0);
            SQ_PUT(Float, y0 + wide_y);
            SQ_PUT(Float, x0 + wide_x);
            SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
            prefetch(sq);
            sq += 8 + 7;
            SQ_PUT(Uint32, col1);
            sq -= 2;
            SQ_PUT(Float, inv_z1);
            SQ_PUT(Float, y1 + wide_y);
            SQ_PUT(Float, x1 + wide_x);
            SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
            prefetch(sq);
            sq += 8;
        }
        i++;
    } while (i <= last);
    *vertex = sq;
}
