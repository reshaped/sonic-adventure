/* SA1's 2D lines on the Dreamcast: the sends sa_present_live runs for a live
 * draw of SA_SA1_LINE_2D. its geometry is the game's own NJS_POINT2COL, the
 * lines' ends on the screen. the sends read it during the call alone.
 * sdk/ninja/njDrawLine2D.c is SA1's layer. njDrawLine2D opens the strip
 * through Ninja and hands the points to sa_present_live. sa_present_live
 * sends each line before it returns */
#ifndef SA1_LINE2D_H
#define SA1_LINE2D_H

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "sa1/strip.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* an sa_draw_live of SA_SA1_LINE_2D's state. n counts the lines for pairs
 * and the points for a fan or a strip. the lines' 1/z comes from pri. attr
 * holds NJD_USE_TEXTURE and the NJD_DRAW_MASK bits that say how the points
 * join */
struct sa1_live_line_2d {
    Int n;
    Float pri;
    Uint32 attr;
};

/* send lines at z from p's points as how, NJD_DRAW_MASK's bits, joins them.
 * NJD_DRAW_FAN joins the first point to each of the others.
 * NJD_DRAW_CONNECTED joins each point to the one before, and each point takes
 * the older end's place. otherwise each line takes two points of its own */
ALWAYS_INLINE Uint32 *sa1_lines_2d(Uint32 *sq, const NJS_POINT2COL *p, Int n,
                                   Uint32 how, Float z, Int textured)
{
    struct sa1_end ends[2];
    Int i;

    if (how == NJD_DRAW_FAN || how == NJD_DRAW_CONNECTED) {
        sa1_end_at(&ends[0], p, 0, textured);
        for (i = 1; i < n; i++) {
            sa1_end_at(&ends[how == NJD_DRAW_FAN ? 1 : i % 2], p, i, textured);
            sq = sa1_send_edge(sq, &ends[0], &ends[1], z, textured);
        }
        return sq;
    }
    for (i = 0; i < n; i++) {
        sa1_end_at(&ends[0], p, 2 * i, textured);
        sa1_end_at(&ends[1], p, (2 * i) + 1, textured);
        sq = sa1_send_edge(sq, &ends[0], &ends[1], z, textured);
    }
    return sq;
}

/* draw 2D lines as their state says, consumed in the call, into the strip
 * at sa1_live_sq. the call compiles into the layer's draw, as sprite.h's
 * does. each sa1_lines_2d call passes textured as a constant, so its inlined
 * copy folds the texture tests. boundary.py fails an object that includes
 * two headers defining the call. the engine goes unread */
ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    const struct sa1_live_line_2d *state = draw->state;
    const NJS_POINT2COL *p = draw->geometry;
    Uint32 how;
    Float z;

    (void)engine;
    if (draw->format != SA_SA1_LINE_2D) {
        return SA_ERR_UNSUPPORTED;
    }
    how = state->attr & NJD_DRAW_MASK;
    z = sa1_rhw_2d(state->pri);
    sa1_live_sq = state->attr & NJD_USE_TEXTURE
                      ? sa1_lines_2d(sa1_live_sq, p, state->n, how, z, TRUE)
                      : sa1_lines_2d(sa1_live_sq, p, state->n, how, z, FALSE);
    return SA_OK;
}

#endif
