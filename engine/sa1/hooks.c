/* 16:9's picture on the PC: what engine/dc/dreamcast.h declares, which make
 * dc takes from the Dreamcast engine. the PC has no pillarbox, so
 * port_pillarbox stays FALSE. where the game holds the window at 4:3, the
 * picture is 4:3 too. the PC's window then bands it. src/system/pace.c
 * defines the window */
#include <ninja.h>

#include "draw/model.h"
#include "system/port.h"

Float port_draw_aspect = 1.0F;
Int port_pillarbox;
enum wide_mode wide_applied;
Uint32 port_sprites_sent;
Uint32 port_draws_sent;

Float port_wide_x(Float x)
{
    /* the x the squeeze leaves in place: the pivot, which port_wide_anchor
     * counts from the screen's middle, where x is 320 */
    Float at = 320.0F + port_wide_anchor;

    return at + ((x - at) * port_draw_aspect);
}

Int port_wide_squeezes(void)
{
    return port_draw_aspect != 1.0F && !port_wide_cover;
}

Int port_wide_covers(const Float *x, Int count, Int stride)
{
    Int left = FALSE;
    Int right = FALSE;

    for (; count > 0; count--, x += stride) {
        if (*x >= -1.0F && *x <= 1.0F) {
            left = TRUE;
        } else if (*x >= 639.0F && *x <= 641.0F) {
            right = TRUE;
        } else {
            return FALSE;
        }
    }
    return left && right;
}

void port_band(Float x0, Float y0, Float x1, Float y1, Uint32 color, Float pri,
               Uint32 attr)
{
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_POINT2COL band;
    int i;

    points[0].x = points[3].x = x0;
    points[1].x = points[2].x = x1;
    points[0].y = points[1].y = y0;
    points[2].y = points[3].y = y1;
    for (i = 0; i < 4; i++) {
        colors[i].color = color;
    }
    band.p = points;
    band.col = colors;
    band.tex = NULL;
    band.num = 4;
    njDrawPolygon2D(&band, 4, pri, attr);
}

void wide_apply(void)
{
    enum wide_mode mode = port_view_aspect != 1.0F ? WIDE_ON : WIDE_OFF;

    if (mode != wide_applied) {
        wide_applied = mode;
        port_draw_aspect = port_view_aspect;
        /* the model draws project by draw_model_init's copy of the aspect.
         * the easy draws need no call, since ninja.c's clip reads
         * port_draw_aspect at each draw */
        draw_model_init();
    }
}

/* no pillarbox, so no bands */
void wide_end(void)
{
}
