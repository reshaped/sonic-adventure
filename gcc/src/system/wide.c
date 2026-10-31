/* the port's 16:9: the window and the picture's aspect, which frame_end
 * applies between frames, and pillarbox's bands. make gcc links this into
 * 1ST_READ.BIN after the image's sources */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "system/frame.h"

/* 16:9's aspect. a projection of x by _nj_screen_.dist times 3/4 fits a
 * view a third wider into the screen's 640 pixels */
#define WIDE_ASPECT 0.75F

enum wide_mode wide_applied;
Float port_view_aspect = 1.0F;

Float port_view_x(Float x)
{
    /* at 1, x itself, which cx + (x - cx) can round away from */
    if (port_view_aspect == 1.0F) {
        return x;
    }
    return _nj_screen_.cx + ((x - _nj_screen_.cx) * port_view_aspect);
}

/* an opaque black band from x0 to x1, down the screen, at depth 1, nearer
 * than the game's 3D */
static void wide_band(Float x0, Float x1)
{
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_POINT2COL band;
    int i;

    points[0].x = points[3].x = x0;
    points[1].x = points[2].x = x1;
    points[0].y = points[1].y = 0.0F;
    points[2].y = points[3].y = 480.0F;
    for (i = 0; i < 4; i++) {
        colors[i].color = 0xff000000;
    }
    band.p = points;
    band.col = colors;
    band.tex = NULL;
    band.num = 4;
    njDrawPolygon2D(&band, 4, -1.0F, NJD_FILL);
}

void wide_end(void)
{
    /* put back what the draws outside tasks, as the stage loops' hud, leave
     * set, as present_run and present_draw do around each task's step and
     * callback */
    port_wide_anchor = WIDE_MIDDLE;
    port_wide_cover = FALSE;
    if (port_pillarbox) {
        port_pillarbox = FALSE;
        port_wide_cover = TRUE;
        wide_band(0.0F, 320.0F * (1.0F - WIDE_ASPECT));
        wide_band(320.0F * (1.0F + WIDE_ASPECT), 640.0F);
        port_wide_cover = FALSE;
    }
}

void wide_apply(enum wide_mode mode)
{
    NJS_VECTOR light;

    port_pillarbox = mode == WIDE_PILLARBOX;
    if (mode != wide_applied) {
        wide_applied = mode;
        port_draw_aspect = mode != WIDE_OFF ? WIDE_ASPECT : 1.0F;
        port_view_aspect = mode == WIDE_ON ? WIDE_ASPECT : 1.0F;
        /* the model draws project by draw_model_init's copy of the aspect,
         * and the easy draws by njSetEasyLight's, which play calls each step
         * and a pause never */
        draw_model_init();
        light = *port_easy_light();
        njSetEasyLight(light.x, light.y, light.z);
    }
}
