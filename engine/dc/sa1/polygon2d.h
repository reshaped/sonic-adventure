/* SA1's 2D polygons on the Dreamcast. sa_present_live here fills or outlines
 * a polygon of the game's NJS_POINT2COL, read during the call alone. SA1's
 * layer for it is sdk/ninja/njDrawPolygon2D.c. njDrawPolygon2D keeps each
 * draw for a picture to come by port.h's present_record. it squeezes x for
 * 16:9, and opens the strip through Ninja */
#ifndef SA1_POLYGON2D_H
#define SA1_POLYGON2D_H

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "sa1/strip.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* an sa_draw_live's state for SA_SA1_POLYGON_2D: njDrawPolygon2D's n, pri
 * and attr. sa_present_live reads NJD_FILL and NJD_USE_TEXTURE of attr */
struct sa1_live_polygon_2d {
    Int n;
    Float pri;
    Uint32 attr;
};

/* fill the polygon of p's n corners at z as one strip. the strip takes the
 * corners in the order 0, n - 1, 1, n - 2 */
ALWAYS_INLINE Uint32 *sa1_fill_2d(Uint32 *sq, const NJS_POINT2COL *p, Int n,
                                  Float z, Int textured)
{
    struct sa1_end end;
    Int i;

    for (i = 0; i < n; i++) {
        sa1_end_at(&end, p, i & 1 ? n - 1 - (i >> 1) : i >> 1, textured);
        sq = sa1_send_vertex(
            sq, i == n - 1 ? KM_VERTEXPARAM_ENDOFSTRIP : KM_VERTEXPARAM_NORMAL,
            end.x, end.y, z, textured, end.u, end.v, end.color);
    }
    return sq;
}

/* outline the polygon of p's n corners at z. ends[0] and ends[1] hold the
 * two newest corners, and place indexes the newer. ends[2] keeps the first
 * corner, which the last edge joins to the newest */
ALWAYS_INLINE Uint32 *sa1_outline_2d(Uint32 *sq, const NJS_POINT2COL *p, Int n,
                                     Float z, Int textured)
{
    struct sa1_end ends[3];
    Int place = 0;
    Int i;

    sa1_end_at(&ends[0], p, 0, textured);
    ends[2] = ends[0];
    for (i = 1; i < n; i++) {
        place = i % 2;
        sa1_end_at(&ends[place], p, i, textured);
        sq = sa1_send_edge(sq, &ends[0], &ends[1], z, textured);
    }
    return sa1_send_edge(sq, &ends[place], &ends[2], z, textured);
}

ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    const struct sa1_live_polygon_2d *state = draw->state;
    const NJS_POINT2COL *p = draw->geometry;
    Uint32 *sq = sa1_live_sq;
    Float z;

    (void)engine;
    if (draw->format != SA_SA1_POLYGON_2D) {
        return SA_ERR_UNSUPPORTED;
    }
    z = sa1_rhw_2d(state->pri);
    /* each call below passes textured as a constant, so its inlined copy
     * compiles without the tests of textured */
    if (state->attr & NJD_FILL) {
        sq = state->attr & NJD_USE_TEXTURE
                 ? sa1_fill_2d(sq, p, state->n, z, TRUE)
                 : sa1_fill_2d(sq, p, state->n, z, FALSE);
    } else {
        sq = state->attr & NJD_USE_TEXTURE
                 ? sa1_outline_2d(sq, p, state->n, z, TRUE)
                 : sa1_outline_2d(sq, p, state->n, z, FALSE);
    }
    sa1_live_sq = sq;
    return SA_OK;
}

#endif
