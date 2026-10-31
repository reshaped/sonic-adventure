/* filled drawing of a clipped polygon */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* store a word in the store queue, just below the word stored before it */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

/* opaque white, as a packed ARGB color */
#define WHITE 0xffffffff

/* project the n points of p onto the screen, and send them through the store
 * queue at *vertex as one strip of KMVERTEX0. n must be at least 2. the strip
 * takes the points from both ends toward the middle: point 0, n - 1, 1, n - 2
 * and so on. a null p->col makes every point WHITE. *vertex ends past the
 * strip */
void njtri_filled(Int n, NJS_POINT3COL *p, Float **vertex)
{
    Int first = 0;
    Int last = n - 1;
    Float scale_x;
    /* @hack redundant: register on scale_y, which keeps it in FR4 and last in
     * R12 (FMOV.S @R14,FR4 at 0x8c624e3e, ADD #-1,R12 at 0x8c624e36 in
     * 1ST_READ.BIN); without
     * it, scale_y takes FR6 and 1.0F FR4, last takes R13 and WHITE R12, and 25
     * of 184 words differ */
    register Float scale_y;
    Float inv_z;
    Float *sq;

    scale_x = scale_y = _nj_screen_.dist;
#ifdef GCC
    /* the picture's aspect, which 16:9 sets in place of Ninja's (port.h) */
    scale_x *= port_draw_aspect;
#else
    scale_x *= _nj_scr_aspect_x_;
#endif
    scale_y *= _nj_scr_aspect_y_;
    sq = *vertex;
    for (;;) {
        inv_z = 1.0F / p->p[first].z;
        sq += 7;
        if (p->col != NULL) {
            SQ_PUT(Uint32, p->col[first].color);
        } else {
            SQ_PUT(Uint32, WHITE);
        }
        sq -= 2;
        SQ_PUT(Float, inv_z);
        SQ_PUT(Float, (p->p[first].y * scale_y * inv_z) + _nj_screen_.cy);
        SQ_PUT(Float, (p->p[first].x * scale_x * inv_z) + _nj_screen_.cx);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8;
        first++;
        if (first == last) {
            break;
        }
        inv_z = 1.0F / p->p[last].z;
        sq += 7;
        if (p->col != NULL) {
            SQ_PUT(Uint32, p->col[last].color);
        } else {
            SQ_PUT(Uint32, WHITE);
        }
        sq -= 2;
        SQ_PUT(Float, inv_z);
        SQ_PUT(Float, (p->p[last].y * scale_y * inv_z) + _nj_screen_.cy);
        SQ_PUT(Float, (p->p[last].x * scale_x * inv_z) + _nj_screen_.cx);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8;
        last--;
        /* @hack spelling: the continue and break in place of a do while, which
         * keep the last point's offset, first * 12, in R3 (0x8c624f1e to
         * 0x8c624f30 in 1ST_READ.BIN); as a do while, it takes R2 and 9 of 184
         * words differ */
        if (last != first) {
            continue;
        }
        break;
    }
    inv_z = 1.0F / p->p[first].z;
    sq += 7;
    if (p->col != NULL) {
        SQ_PUT(Uint32, p->col[first].color);
    } else {
        SQ_PUT(Uint32, WHITE);
    }
    sq -= 2;
    SQ_PUT(Float, inv_z);
    SQ_PUT(Float, (p->p[first].y * scale_y * inv_z) + _nj_screen_.cy);
    SQ_PUT(Float, (p->p[first].x * scale_x * inv_z) + _nj_screen_.cx);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(sq);
    sq += 8;
    *vertex = sq;
}
