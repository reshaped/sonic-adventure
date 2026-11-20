/* SA1's 3D lines on the Dreamcast: the send sa_present_live runs for a live
 * draw of SA_SA1_LINE_3D. its geometry is an NJS_POINT3COL of a line's two
 * ends in view space. the send reads it during the call alone.
 * sdk/ninja/njDrawLine3D.c is SA1's layer. njDrawLine3D opens the strip
 * through Ninja and moves each line's ends by the current matrix. through
 * Ninja, it skips a line with an end past the far clip. it clips the rest to
 * the near plane. it hands each line it keeps to sa_present_live, which
 * projects it and sends its strip before it returns. Ninja's screen and y
 * aspect come by screen.h's names. the picture's x aspect is dreamcast.h's
 * port_draw_aspect */
#ifndef SA1_LINE3D_H
#define SA1_LINE3D_H

#include <kamui.h>
/* ninja.h declares port_draw_aspect, through port.h and dreamcast.h */
#include <ninja.h>
#include <sg_xpt.h>

#include "sa1/screen.h"
#include "sa1/strip.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* an sa_draw_live of SA_SA1_LINE_3D's state. the send reads attr's
 * NJD_USE_TEXTURE alone */
struct sa1_live_line_3d {
    Uint32 attr;
};

/* send the line from end 0 of p to end 1 as a strip of four vertices. p's
 * ends lie in view space. each end projects by the screen's distance over
 * its z. the strip reaches SA1_HALF_WIDTH to one side of the line and twice
 * that to the other. so does njDrawLine3D.c's. each vertex takes its end's
 * 1/z */
ALWAYS_INLINE Uint32 *sa1_line_3d(Uint32 *sq, const NJS_POINT3COL *p,
                                  Int textured)
{
    Float scale_x = sa1_live_ninja_screen.dist * port_draw_aspect;
    Float scale_y = sa1_live_ninja_screen.dist * sa1_live_aspect_y;
    Float z0 = 1.0F / p->p[0].z;
    Float z1 = 1.0F / p->p[1].z;
    Float x0 = (p->p[0].x * scale_x * z0) + sa1_live_ninja_screen.cx;
    Float x1 = (p->p[1].x * scale_x * z1) + sa1_live_ninja_screen.cx;
    Float y0 = (p->p[0].y * scale_y * z0) + sa1_live_ninja_screen.cy;
    Float y1 = (p->p[1].y * scale_y * z1) + sa1_live_ninja_screen.cy;
    Float dx = x0 - x1;
    Float dy = y0 - y1;
    Float length2 = (dx * dx) + (dy * dy);
    Float inv_length;
    Float side_x;
    Float side_y;
    Float wide_x;
    Float wide_y;
    Float u0 = textured ? p->tex[0].tex.u * SA1_UV_SCALE : 0.0F;
    Float v0 = textured ? p->tex[0].tex.v * SA1_UV_SCALE : 0.0F;
    Float u1 = textured ? p->tex[1].tex.u * SA1_UV_SCALE : 0.0F;
    Float v1 = textured ? p->tex[1].tex.v * SA1_UV_SCALE : 0.0F;
    Uint32 color0 = p->col[0].color;
    Uint32 color1 = p->col[1].color;

    /* as sa1_send_edge: ends that project to one point send nothing */
    if (!(length2 > 0.0F)) {
        return sq;
    }
    inv_length = 1.0F / _builtin_sqrtf(length2);
    side_x = dy * inv_length * SA1_HALF_WIDTH;
    side_y = -(dx * inv_length * SA1_HALF_WIDTH);
    wide_x = side_x * 2.0F;
    wide_y = side_y * 2.0F;
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, x0 - side_x, y0 - side_y,
                         z0, textured, u0, v0, color0);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, x1 - side_x, y1 - side_y,
                         z1, textured, u1, v1, color1);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, x0 + wide_x, y0 + wide_y,
                         z0, textured, u0, v0, color0);
    return sa1_send_vertex(sq, KM_VERTEXPARAM_ENDOFSTRIP, x1 + wide_x,
                           y1 + wide_y, z1, textured, u1, v1, color1);
}

/* draw a 3D line as its state says, consumed in the call, into the strip at
 * sa1_live_sq. the call compiles into the layer's draw, as sprite.h's does.
 * each sa1_line_3d call passes textured as a constant, so its inlined copy
 * folds the texture tests. boundary.py fails an object that includes two
 * headers defining the call. the engine goes unread */
ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    const struct sa1_live_line_3d *state = draw->state;

    (void)engine;
    if (draw->format != SA_SA1_LINE_3D) {
        return SA_ERR_UNSUPPORTED;
    }
    sa1_live_sq = state->attr & NJD_USE_TEXTURE
                      ? sa1_line_3d(sa1_live_sq, draw->geometry, TRUE)
                      : sa1_line_3d(sa1_live_sq, draw->geometry, FALSE);
    return SA_OK;
}

#endif
