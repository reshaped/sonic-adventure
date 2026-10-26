/* ADVERTISE's entry points, which start, step and end the menus */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/advertise.h"
#include "advertise/card.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "draw/fade.h"
#include "draw/screen.h"
#include "main.h"
#include "object/common.h"
#include "sound/stream.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/timer.h"

/* the entry table, at the module's start. #pragma section HEAD puts it in
 * DHEAD, SHC's name for HEAD's data. the main loop reads it at MODULE_AREA */
#pragma section HEAD
struct advertise advertise_entries = {
    advertise_start,
    advertise_end,
    advertise_step,
};
#pragma section

static void advertise_reset(void)
{
    NJS_COLOR color;

    stream_destroy();
    advertise_sound_init();
    frame_pacing_init(1, 1);
    heap_init();
    task_init();
    njSetBackColor(0xff000000, 0xff000000, 0xff000000);
    color.argb.a = 0;
    color.argb.r = 0;
    color.argb.g = 0;
    color.argb.b = 0;
    fade_set_cover(&color);
    texture_release_characters();
    njReleaseTextureAll();
    screen_quad_init();
}

static int advertise_frame(void)
{
    int result;

    pad_scan();
    pad_enable();
    task_step_all();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    njFogDisable();
    task_draw_all();
    draw_off = FALSE;
    if (pad_quit_pressed(0)) {
        result =
            ADVERTISE_WORK()->shown == SCREEN_CARD ? MENU_EXIT : OVERLAY_RESET;
        task_destroy_all();
        return result;
    }
    return OVERLAY_RUNNING;
}

int advertise_start(enum menu_entrance *entrance)
{
    /* @hack spelling: copy, a copy of entrance, into which SHC loads the
     * pointer from the stack (MOV.L @R15,R5 at 0x8c9000fc); without it, SHC
     * loads it into R0, and 2 of 34 words differ */
    enum menu_entrance *copy;
    struct screen_start start;

    advertise_reset();
    card_open();
    advertise_task = NULL;
    copy = entrance;
    switch (*copy) {
    case MENU_FROM_SUMMARY:
        start.entrance = MENU_FROM_SUMMARY;
        break;
    case MENU_FROM_TRIAL:
        start.entrance = MENU_FROM_TRIAL;
        break;
    default:
        menu_state.wheel_story = 0;
        start.entrance = MENU_FROM_TOP;
        break;
    }
    screen_create(&start);
    return OVERLAY_RUNNING;
}

int advertise_end(void *unused)
{
    screen_kill();
    card_close();
    return 0;
}

int advertise_step(enum menu_entrance *entrance)
{
    int result;

    /* @hack spelling: the frame's result assigned in the test. SHC tests it in
     * R0 (TST R0,R0 at 0x8c9001ba), and copies it to R4 in the slot of the
     * BF/S. assigned first, it is copied to R4 and tested there, and 3 of 13
     * words differ */
    if ((result = advertise_frame()) == OVERLAY_RUNNING) {
        result = ADVERTISE_WORK()->result;
    }
    return result;
}
