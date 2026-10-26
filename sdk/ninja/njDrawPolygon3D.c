/* 3D polygons, moved by the current matrix and clipped to the near plane */
#include <ninja.h>
#include <sg_xpt.h>

/* the most points p may hold. clipping to the near plane can add one, so
 * clipped holds one more */
#define POINTS_MAX 32

/* draw the polygon of p's n points, moved by the current matrix. it draws
 * nothing if any point's z is beyond _nj_far_clip_, or if fewer than 3 points
 * are left after clipping to the near plane */
void njDrawPolygon3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    NJS_POINT3COL view;
    NJS_POINT3COL clipped;
    NJS_POINT3COL spare;
    NJS_POINT3 view_p[POINTS_MAX];
    NJS_POINT3 clipped_p[POINTS_MAX + 1];
    NJS_COLOR view_col[POINTS_MAX];
    NJS_COLOR view_tex[POINTS_MAX];
    NJS_COLOR clipped_col[POINTS_MAX + 1];
    NJS_COLOR clipped_tex[POINTS_MAX + 1];
    NJS_POINT3 spare_p[POINTS_MAX + 2];
    NJS_COLOR spare_col[POINTS_MAX + 2];
    NJS_COLOR spare_tex[POINTS_MAX + 2];
    Float *vertex;
    Int i;
    Int left;

    view.p = view_p;
    view.col = view_col;
    view.tex = view_tex;
    clipped.p = clipped_p;
    clipped.col = clipped_col;
    clipped.tex = clipped_tex;
    /* @hack redundant: spare's three pointers, which nothing reads
     * (0x8c614e34 to 0x8c614e3e); without them, 211 of 237 words differ */
    spare.p = spare_p;
    spare.col = spare_col;
    spare.tex = spare_tex;
    for (i = 0; i < n; i++) {
        njCalcPoint(NULL, &p->p[i], &view.p[i]);
    }
    if (!njFarClipping(&view, n)) {
        return;
    }
    for (i = 0; i < n; i++) {
        if (attr & NJD_USE_TEXTURE) {
            view.tex[i].tex.u = p->tex[i].tex.u;
            view.tex[i].tex.v = p->tex[i].tex.v;
            if (p->col) {
                view.col[i].color = p->col[i].color;
            }
        } else {
            view.col[i].color = p->col[i].color;
        }
    }
    left = njNearClipping(&view, n, &clipped, attr);
    if (left < 3) {
        return;
    }
    njGetkmPointer();
    if ((attr & NJD_USE_TEXTURE) && (attr & NJD_TRANSPARENT)) {
        njSetkmColorMode(COLOR_MODE_TRANS | COLOR_MODE_TEXTURE);
        vertex = njStartVertexTexTrans();
    } else if (attr & NJD_USE_TEXTURE) {
        njSetkmColorMode(COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE);
        vertex = njStartVertexTexOpaque();
    } else if (attr & NJD_TRANSPARENT) {
        njSetkmColorMode(COLOR_MODE_TRANS);
        vertex = njStartVertexNonTexTrans();
    } else {
        njSetkmColorMode(COLOR_MODE_OPAQUE);
        vertex = njStartVertexNonTexOpaque();
    }
    if ((attr & NJD_FILL) && (attr & NJD_USE_TEXTURE)) {
        njtri_filled_with_tex(left, &clipped, &vertex);
    } else if (attr & NJD_FILL) {
        njtri_filled(left, &clipped, &vertex);
    } else if (attr & NJD_USE_TEXTURE) {
        njtri_wiredpoly_with_tex(left, &clipped, &vertex);
    } else {
        njtri_wiredpoly(left, &clipped, &vertex);
    }
    njEndVertex((Uint32 *)vertex);
    njSetkmPointer();
}
