/* 3D triangles, moved by the current matrix and clipped to the near plane */
#include <ninja.h>
#include <sg_xpt.h>

/* the most corners a triangle has after clipping to the near plane */
#define CLIPPED_CORNERS 4

/* draw n triangles from the points of p, moved by the current matrix. triangle
 * i takes points 3i to 3i + 2, or under NJD_DRAW_CONNECTED points i to i + 2,
 * or under NJD_DRAW_FAN point 0 and points 2i + 1 and 2i + 2. a triangle is
 * skipped if any corner's z is beyond _nj_far_clip_, or if fewer than 3
 * corners are left after clipping to the near plane */
void njDrawTriangle3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    NJS_POINT3COL view;
    NJS_POINT3COL clip;
    NJS_POINT3 view_p[3];
    NJS_POINT3 clip_p[CLIPPED_CORNERS];
    NJS_COLOR view_col[3];
    NJS_COLOR view_tex[3];
    NJS_COLOR clip_col[CLIPPED_CORNERS];
    NJS_COLOR clip_tex[CLIPPED_CORNERS];
    Float *sq;
    Int i;
    Int step;
    Int first;
    Int first_col;
    Int next;
    Int count;

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
    if (attr & NJD_DRAW_CONNECTED) {
        step = 1;
    } else if (attr & NJD_DRAW_FAN) {
        step = 2;
    } else {
        step = 3;
    }
    for (i = 0; i < n; i++) {
        /* @hack spelling: first_col, a copy of first for the colors, which
         * keeps first at @(4,R15) and NJD_FILL's test at @(12,R15) (0x8c62497c,
         * 0x8c624920); without it, the two trade and 129 of 322 words differ */
        first_col = first = (attr & NJD_DRAW_FAN) ? 0 : i * step;
        njCalcPoint(NULL, &p->p[first], &view.p[0]);
        next = (i * step) + 1;
        njCalcPoint(NULL, &p->p[next], &view.p[1]);
        next++;
        njCalcPoint(NULL, &p->p[next], &view.p[2]);
        if (njFarClipping(&view, 3)) {
            if (attr & NJD_USE_TEXTURE) {
                view.tex[0].tex.u = p->tex[first].tex.u;
                view.tex[0].tex.v = p->tex[first].tex.v;
                /* @hack spelling: next computed again from i, here and for the
                 * colors, which keeps i * step + 1 in three frame slots, one
                 * for each computation (0x8c624918, 0x8c62491c, 0x8c62491e);
                 * with one index for all three, 247 of 322 words differ */
                next = (i * step) + 1;
                view.tex[1].tex.u = p->tex[next].tex.u;
                view.tex[1].tex.v = p->tex[next].tex.v;
                next++;
                view.tex[2].tex.u = p->tex[next].tex.u;
                view.tex[2].tex.v = p->tex[next].tex.v;
            }
            view.col[0].color = p->col[first_col].color;
            next = (i * step) + 1;
            view.col[1].color = p->col[next].color;
            /* @hack spelling: next = next + 1, which keeps i in R8 (MOV #0,R8
             * at 0x8c62490e); as next++, i is kept in the frame, the function
             * grows 12 bytes, and 244 of 322 words differ */
            next = next + 1;
            view.col[2].color = p->col[next].color;
            count = njNearClipping(&view, 3, &clip, attr);
            if (count >= 3) {
                if ((attr & NJD_FILL) && (attr & NJD_USE_TEXTURE)) {
                    njtri_filled_with_tex(count, &clip, &sq);
                } else if (attr & NJD_FILL) {
                    njtri_filled(count, &clip, &sq);
                } else if (attr & NJD_USE_TEXTURE) {
                    njtri_wiredpoly_with_tex(count, &clip, &sq);
                } else {
                    njtri_wiredpoly(count, &clip, &sq);
                }
            }
        }
    }
    njEndVertex((Uint32 *)sq);
    njSetkmPointer();
}
