/* SA1's screen polygons on the Dreamcast. sa_present_live here sends a strip
 * of the game's NJS_POLYGON_VTX or NJS_TEXTURE_VTX, read during the call
 * alone. sdk/ninja/njDraw.c is SA1's layer for these draws. its entries keep
 * each draw for a picture to come by port.h's present_record. they open the
 * strip through Ninja. a vertex's z is the 1/z the tile accelerator takes, so
 * it goes as the game gave it */
#ifndef SA1_POLYGON_H
#define SA1_POLYGON_H

#include <kamui.h>
/* ninja.h gives dreamcast.h's 16:9 names, through port.h */
#include <ninja.h>
#include <sg_xpt.h>

#include "sa1/strip.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* an sa_draw_live's state for SA_SA1_POLYGON and SA_SA1_TEXTURE. squeeze is
 * TRUE where x takes the 2D draws' squeeze, port_wide_x */
struct sa1_live_polygon {
    Int count;
    Int squeeze;
};

ALWAYS_INLINE Float sa1_polygon_x(Float x, Int squeeze)
{
    return squeeze ? port_wide_x(x) : x;
}

ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    const struct sa1_live_polygon *state = draw->state;
    Uint32 *sq = sa1_live_sq;
    Int squeeze;
    Int last;
    Int i;

    (void)engine;
    if (draw->format != SA_SA1_POLYGON && draw->format != SA_SA1_TEXTURE) {
        return SA_ERR_UNSUPPORTED;
    }
    squeeze = state->squeeze;
    last = state->count - 1;
    if (draw->format == SA_SA1_POLYGON) {
        const NJS_POLYGON_VTX *v = draw->geometry;

        for (i = 0; i <= last; i++, v++) {
            sq = sa1_send_vertex(sq,
                                 i == last ? KM_VERTEXPARAM_ENDOFSTRIP
                                           : KM_VERTEXPARAM_NORMAL,
                                 sa1_polygon_x(v->x, squeeze), v->y, v->z,
                                 FALSE, 0.0F, 0.0F, v->col);
        }
    } else {
        const NJS_TEXTURE_VTX *v = draw->geometry;

        for (i = 0; i <= last; i++, v++) {
            sq = sa1_send_vertex(sq,
                                 i == last ? KM_VERTEXPARAM_ENDOFSTRIP
                                           : KM_VERTEXPARAM_NORMAL,
                                 sa1_polygon_x(v->x, squeeze), v->y, v->z, TRUE,
                                 v->u, v->v, v->col);
        }
    }
    sa1_live_sq = sq;
    return SA_OK;
}

#endif
