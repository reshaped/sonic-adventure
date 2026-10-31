/* filled, textured drawing of a clipped polygon */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* store a word in the store queue, just below the word stored before it */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

/* send the n points of p as njtri_filled does, but as one strip of KMVERTEX3.
 * each point takes its u and v from p->tex, in units of 1/256, and leaves
 * uOffsetRGB unwritten. p->col and p->tex must not be null */
void njtri_filled_with_tex(Int n, NJS_POINT3COL *p, Float **vertex)
{
    Int first = 0;
    Int last = n;
    Float scale_x;
    /* @hack redundant: register on scale_y, which keeps it in FR4, scale_x in
     * FR6 and first in R7 (0x8c624fa6 to 0x8c624fc6 in
     * 1ST_READ.BIN); without it, first takes
     * R14, the float registers rotate, and 42 of 201 words differ */
    register Float scale_y;
    Float inv_z;
    Float tex_scale = 1.0F / 256.0F;
    Float *sq;

    /* @hack spelling: last decremented here, not in its initializer, which
     * keeps last * 12 in R10 and last * 4 in R1 (0x8c625048 to 0x8c6250a4 in
     * 1ST_READ.BIN); as Int
     * last = n - 1, the two trade and 14 of 201 words differ */
    last--;
    scale_x = scale_y = _nj_screen_.dist;
#ifdef GCC
    /* the picture's aspect, which 16:9 sets in place of Ninja's (port.h) */
    scale_x *= port_draw_aspect;
#else
    scale_x *= _nj_scr_aspect_x_;
#endif
    scale_y *= _nj_scr_aspect_y_;
    sq = *vertex;
    do {
        inv_z = 1.0F / p->p[first].z;
        sq += 7;
        SQ_PUT(Uint32, p->col[first].color);
        SQ_PUT(Float, p->tex[first].tex.v * tex_scale);
        SQ_PUT(Float, p->tex[first].tex.u * tex_scale);
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
        SQ_PUT(Uint32, p->col[last].color);
        SQ_PUT(Float, p->tex[last].tex.v * tex_scale);
        SQ_PUT(Float, p->tex[last].tex.u * tex_scale);
        SQ_PUT(Float, inv_z);
        SQ_PUT(Float, (p->p[last].y * scale_y * inv_z) + _nj_screen_.cy);
        SQ_PUT(Float, (p->p[last].x * scale_x * inv_z) + _nj_screen_.cx);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8;
        last--;
    } while (last != first);
    inv_z = 1.0F / p->p[first].z;
    sq += 7;
    SQ_PUT(Uint32, p->col[first].color);
    SQ_PUT(Float, p->tex[first].tex.v * tex_scale);
    SQ_PUT(Float, p->tex[first].tex.u * tex_scale);
    SQ_PUT(Float, inv_z);
    SQ_PUT(Float, (p->p[first].y * scale_y * inv_z) + _nj_screen_.cy);
    SQ_PUT(Float, (p->p[first].x * scale_x * inv_z) + _nj_screen_.cx);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(sq);
    sq += 8;
    *vertex = sq;
}
