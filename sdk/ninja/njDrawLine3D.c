/* 3D lines, each moved by the current matrix and drawn as a strip of two
 * triangles */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* step sq back a word and store value there */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

/* how far a line's strip reaches to one side of the line, in pixels. it reaches
 * twice as far to the other side, so the strip is 3 * LINE_SIDE wide */
#define LINE_SIDE 0.525F

/* draw n lines from the points of p: two points each, a strip under
 * NJD_DRAW_CONNECTED, or a fan from the first point under NJD_DRAW_FAN. a line
 * with an end's z beyond _nj_far_clip_ is skipped, and the rest are clipped to
 * the near plane. each line goes out as a strip of four 32-byte vertices */
void njDrawLine3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    Int i;
    Int first;
    Int first_col;
    Int second;
    Int step;
    Int second_tex;
    Int second_col;
    Float scale_x;
    Float scale_y;
    /* @hack spelling: the screen through a pointer, which puts n in R9 and
     * KM_VERTEXPARAM_NORMAL in R10 (0x8c62424e, 0x8c624364); without it, 21 of
     * 697 words differ */
    NJS_SCREEN *screen;
    /* @hack redundant: register on sq, which puts attr in R11, screen in R12
     * and scale_x in FR15 (0x8c624246, 0x8c624248, 0x8c62426e); without it,
     * 661 of 697 words differ */
    register Uint32 *sq;
    /* @hack spelling: the texture scale through a local, which the image keeps
     * in the frame at 20 (FMOV.S FR3,@(R0,R15) at 0x8c624268); without it, 657
     * of 697 words differ */
    const Float tex_scale = 1.0F / 256.0F;
    NJS_POINT3COL view;
    NJS_POINT3COL clip;
    NJS_POINT3 view_p[2];
    NJS_POINT3 clip_p[2];
    NJS_COLOR view_col[2];
    NJS_COLOR view_tex[2];
    NJS_COLOR clip_col[2];
    NJS_COLOR clip_tex[2];

    screen = &_nj_screen_;
    scale_x = screen->dist * _nj_scr_aspect_x_;
    scale_y = screen->dist * _nj_scr_aspect_y_;
    view.p = view_p;
    view.col = view_col;
    view.tex = view_tex;
    clip.p = clip_p;
    clip.col = clip_col;
    clip.tex = clip_tex;
    njGetkmPointer();
    if ((attr & NJD_USE_TEXTURE) && (attr & NJD_TRANSPARENT)) {
        njSetkmColorMode(COLOR_MODE_TRANS | COLOR_MODE_TEXTURE);
        sq = njStartVertexTexTrans();
    } else if (attr & NJD_USE_TEXTURE) {
        njSetkmColorMode(COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE);
        sq = njStartVertexTexOpaque();
    } else if (attr & NJD_TRANSPARENT) {
        njSetkmColorMode(COLOR_MODE_TRANS);
        sq = njStartVertexNonTexTrans();
    } else {
        njSetkmColorMode(COLOR_MODE_OPAQUE);
        sq = njStartVertexNonTexOpaque();
    }
    step = (attr & NJD_DRAW_MASK) ? 1 : 2;
    if (attr & NJD_USE_TEXTURE) {
        for (i = 0; i < n; i++) {
            /* @hack spelling: first_col, a copy of first, and second_tex and
             * second_col, second computed again, here and in the plain loop,
             * which keep i * step + 1 in two frame slots (0x8c624386 and
             * 0x8c624388, 0x8c624632 and 0x8c624634); with first and second
             * alone, the loops keep it in one and 581 of 697 words differ */
            first_col = first = (attr & NJD_DRAW_FAN) ? 0 : i * step;
            njCalcPoint(NULL, &p->p[first], &view.p[0]);
            second = (i * step) + 1;
            njCalcPoint(NULL, &p->p[second], &view.p[1]);
            if (njFarClipping(&view, 2)) {
                view.tex[0].tex.u = p->tex[first].tex.u;
                view.tex[0].tex.v = p->tex[first].tex.v;
                second_tex = (i * step) + 1;
                view.tex[1].tex.u = p->tex[second_tex].tex.u;
                view.tex[1].tex.v = p->tex[second_tex].tex.v;
                view.col[0].color = p->col[first_col].color;
                second_col = (i * step) + 1;
                view.col[1].color = p->col[second_col].color;
                if (njNearClippingLine(&view, &clip, attr)) {
                    Float z0;
                    Float z1;
                    Float x0;
                    Float y0;
                    Float x1;
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
                    Float u0;
                    Float v0;
                    Float u1;
                    Float v1;

                    z0 = 1.0F / clip.p[0].z;
                    z1 = 1.0F / clip.p[1].z;
                    x0 = (clip.p[0].x * scale_x * z0) + screen->cx;
                    x1 = (clip.p[1].x * scale_x * z1) + screen->cx;
                    y0 = (clip.p[0].y * scale_y * z0) + screen->cy;
                    y1 = (clip.p[1].y * scale_y * z1) + screen->cy;
                    dx = x0 - x1;
                    dy = y0 - y1;
                    inv_length = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
                    side_x = dy * inv_length * LINE_SIDE;
                    side_y = -(dx * inv_length * LINE_SIDE);
                    wide_x = side_x * 2.0F;
                    wide_y = side_y * 2.0F;
                    u0 = clip.tex[0].tex.u * tex_scale;
                    v0 = clip.tex[0].tex.v * tex_scale;
                    u1 = clip.tex[1].tex.u * tex_scale;
                    v1 = clip.tex[1].tex.v * tex_scale;
                    col0 = clip.col[0].color;
                    col1 = clip.col[1].color;
                    /* @hack redundant: a copy of side_y to v0 and wide_y that
                     * never runs: the stack slots of spilled floats are ordered
                     * by use count, and these extra uses put v0 and wide_y in
                     * the slots of the image's frame; without it, 12 of 697
                     * words differ */
                    if (0) {
                        v0 = wide_y = side_y;
                    }
                    sq += 7;
                    SQ_PUT(Uint32, col0);
                    SQ_PUT(Float, v0);
                    SQ_PUT(Float, u0);
                    SQ_PUT(Float, z0);
                    SQ_PUT(Float, y0 - side_y);
                    SQ_PUT(Float, x0 - side_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
                    prefetch(sq);
                    sq += 8 + 7;
                    SQ_PUT(Uint32, col1);
                    SQ_PUT(Float, v1);
                    SQ_PUT(Float, u1);
                    SQ_PUT(Float, z1);
                    SQ_PUT(Float, y1 - side_y);
                    SQ_PUT(Float, x1 - side_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
                    prefetch(sq);
                    sq += 8 + 7;
                    SQ_PUT(Uint32, col0);
                    SQ_PUT(Float, v0);
                    SQ_PUT(Float, u0);
                    SQ_PUT(Float, z0);
                    SQ_PUT(Float, y0 + wide_y);
                    SQ_PUT(Float, x0 + wide_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
                    prefetch(sq);
                    sq += 8 + 7;
                    SQ_PUT(Uint32, col1);
                    SQ_PUT(Float, v1);
                    SQ_PUT(Float, u1);
                    SQ_PUT(Float, z1);
                    SQ_PUT(Float, y1 + wide_y);
                    SQ_PUT(Float, x1 + wide_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
                    prefetch(sq);
                    sq += 8;
                }
            }
        }
    } else {
        for (i = 0; i < n; i++) {
            first = (attr & NJD_DRAW_FAN) ? 0 : i * step;
            njCalcPoint(NULL, &p->p[first], &view.p[0]);
            second = (i * step) + 1;
            njCalcPoint(NULL, &p->p[second], &view.p[1]);
            if (njFarClipping(&view, 2)) {
                view.col[0].color = p->col[first].color;
                second_col = (i * step) + 1;
                view.col[1].color = p->col[second_col].color;
                if (njNearClippingLine(&view, &clip, attr)) {
                    Float z0;
                    Float z1;
                    Float x0;
                    Float y0;
                    Float x1;
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

                    z0 = 1.0F / clip.p[0].z;
                    z1 = 1.0F / clip.p[1].z;
                    x0 = (clip.p[0].x * scale_x * z0) + screen->cx;
                    x1 = (clip.p[1].x * scale_x * z1) + screen->cx;
                    y0 = (clip.p[0].y * scale_y * z0) + screen->cy;
                    y1 = (clip.p[1].y * scale_y * z1) + screen->cy;
                    dx = x0 - x1;
                    dy = y0 - y1;
                    inv_length = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
                    side_x = dy * inv_length * LINE_SIDE;
                    side_y = -(dx * inv_length * LINE_SIDE);
                    wide_x = side_x * 2.0F;
                    wide_y = side_y * 2.0F;
                    col0 = clip.col[0].color;
                    col1 = clip.col[1].color;
                    /* @hack redundant: a copy of side_y to wide_y that never
                     * runs, a use that, as above, places wide_y in its slot of
                     * the image's frame; without it, 6 of 697 words differ */
                    if (0) {
                        wide_y = side_y;
                    }
                    /* without a texture, u and v, each vertex's words 4 and 5,
                     * go unwritten */
                    sq += 7;
                    SQ_PUT(Uint32, col0);
                    sq -= 2;
                    SQ_PUT(Float, z0);
                    SQ_PUT(Float, y0 - side_y);
                    SQ_PUT(Float, x0 - side_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
                    prefetch(sq);
                    sq += 8 + 7;
                    SQ_PUT(Uint32, col1);
                    sq -= 2;
                    SQ_PUT(Float, z1);
                    SQ_PUT(Float, y1 - side_y);
                    SQ_PUT(Float, x1 - side_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
                    prefetch(sq);
                    sq += 8 + 7;
                    SQ_PUT(Uint32, col0);
                    sq -= 2;
                    SQ_PUT(Float, z0);
                    SQ_PUT(Float, y0 + wide_y);
                    SQ_PUT(Float, x0 + wide_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
                    prefetch(sq);
                    sq += 8 + 7;
                    SQ_PUT(Uint32, col1);
                    sq -= 2;
                    SQ_PUT(Float, z1);
                    SQ_PUT(Float, y1 + wide_y);
                    SQ_PUT(Float, x1 + wide_x);
                    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
                    prefetch(sq);
                    sq += 8;
                }
            }
        }
    }
    njEndVertex(sq);
    njSetkmPointer();
}
