/* the port's 16:9 and rate as the Dreamcast draws them. this file also gives
 * 16:9's cover rule for the 2D draws and pillarbox's bands. frame_end, in
 * src/system/pace.c, asks the window and the mode once a frame. it applies
 * the 16:9 mode between frames by wide_apply. port_band draws the overlay's
 * and an event picture's bands too. make dc links this into 1ST_READ.BIN
 * after the image's sources */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "engine/dc/dreamcast.h"
#include "engine/dc/owner.h"
#include "main.h"
#include "sa_dc.h"
#include "system/frame.h"

enum wide_mode wide_applied;
/* the window the game last asked, SA_ASPECT_*, and the 16:9 mode
 * sa_present_mode last made */
static Uint32 window_asked;
static enum wide_mode wide_asked;

Int port_wide_covers(const Float *x, Int count, Int stride)
{
    Int left = FALSE;
    Int right = FALSE;

    if (port_pillarbox) {
        return FALSE;
    }
    for (; count > 0; count--, x += stride) {
        if (*x >= -1.0F && *x <= 1.0F) {
            left = TRUE;
        } else if (*x >= SCREEN_WIDTH - 1.0F && *x <= SCREEN_WIDTH + 1.0F) {
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

/* pillarbox's bands are opaque black */
#define BAND_COLOR 0xff000000

void wide_end(void)
{
    if (port_pillarbox) {
        port_pillarbox = FALSE;
        port_wide_cover = TRUE;
        /* bands down the sides, at a 1/z of 1, nearer than the game's 3D.
         * with pillarbox ended and port_wide_cover TRUE, they keep x */
        port_band(0.0F, 0.0F, SCREEN_MIDDLE_X * (1.0F - WIDE_ASPECT),
                  SCREEN_HEIGHT, BAND_COLOR, -1.0F, NJD_FILL);
        port_band(SCREEN_MIDDLE_X * (1.0F + WIDE_ASPECT), 0.0F, SCREEN_WIDTH,
                  SCREEN_HEIGHT, BAND_COLOR, -1.0F, NJD_FILL);
        port_wide_cover = FALSE;
    }
}

sa_result sa_present_window(sa_engine *engine, uint32_t aspect)
{
    (void)engine;
    if (aspect > SA_ASPECT_16_9) {
        return SA_ERR_INVALID;
    }
    window_asked = aspect;
    return SA_OK;
}

/* applied is asked, but at 30 where the 16:9 mode asked differs from the one
 * applied. dreamcast.h says why at wide_apply. held stays 0 even where the
 * rate is held. at 60, owner_sixty shows each picture when rendered */
sa_result sa_present_mode(sa_engine *engine, const sa_mode *asked,
                          sa_mode *applied)
{
    Int sixty;

    (void)engine;
    if (asked == NULL || applied == NULL || asked->aspect > SA_ASPECT_16_9 ||
        (asked->rate != 30 && asked->rate != 60) || asked->held != 0 ||
        asked->reserved != 0) {
        return SA_ERR_INVALID;
    }
    if (asked->aspect == SA_ASPECT_4_3 && window_asked == SA_ASPECT_16_9) {
        return SA_ERR_UNSUPPORTED;
    }
    wide_asked = asked->aspect == SA_ASPECT_4_3  ? WIDE_OFF
                 : window_asked == SA_ASPECT_4_3 ? WIDE_PILLARBOX
                                                 : WIDE_ON;
    sixty = asked->rate == 60 && wide_asked == wide_applied;
    owner_sixty(sixty);
    *applied = *asked;
    applied->rate = sixty ? 60 : 30;
    return SA_OK;
}

void wide_apply(void)
{
    enum wide_mode mode = wide_asked;
    NJS_VECTOR light;

    port_pillarbox = mode == WIDE_PILLARBOX;
    if (mode != wide_applied) {
        wide_applied = mode;
        port_draw_aspect = mode != WIDE_OFF ? WIDE_ASPECT : 1.0F;
        /* the model draws project by draw_model_init's copy of the aspect,
         * and the easy draws by njSetEasyLight's. play sets the light each
         * step, but a pause runs no step. so set the light again as it
         * stands */
        draw_model_init();
        light = *port_easy_light();
        njSetEasyLight(light.x, light.y, light.z);
    }
}
