/* frame pacing: the vsyncs and steps of a frame, its display, and the vsync
 * callback */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "adv02/fog.h"
#include "progress/demo.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg02/windy.h"
#include "stg06/sky_deck.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/timer.h"

/* the fog color and table in the ruin's module. the hook that reads them
 * runs only in its stage, while the module is loaded */
extern Uint32 ruin_fog_color;
extern NJS_FOG_TABLE ruin_fog;

static void act_idle(void);
static void windy_set_fog(void);
static void sky_deck_set_fog(void);
static void ruin_set_fog(void);
static void mystic_ruins_set_fog(void);

int frame_interval;
/* @hack type: frame_countdown isn't volatile, though frame_vsync counts it
 * down while frame_wait and frame_wait_step spin on it. SHC reads it again on
 * each pass of a spin all the same (MOV.L @R4,R2 at 0x8c054a14). declared
 * volatile, 101 of the 124 words of frame_pacing_init, frame_wait,
 * frame_wait_step and frame_vsync differ */
#ifdef GCC
volatile int frame_countdown;
#else
int frame_countdown;
#endif
int frame_step_count;
int frame_step;

/* the hook frame_vsync runs at a frame's last vsync, by stage and act */
static void (*act_hooks[STAGE_ROWS][STAGE_ACTS])(void) = {
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {windy_set_fog, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {sky_deck_set_fog, sky_deck_set_fog, sky_deck_set_fog, act_idle, act_idle},
    {act_idle, ruin_set_fog, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, mystic_ruins_set_fog, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
    {act_idle, act_idle, act_idle, act_idle, act_idle, act_idle},
};

void frame_pacing_init(int interval, int steps)
{
    frame_interval = interval;
    frame_countdown = frame_interval;
    frame_step_count = steps;
    frame_step = 0;
    njSetVSyncFunction(frame_vsync);
    bitmap_clear_queue();
}

void frame_set_interval(int interval)
{
    frame_interval = interval;
    frame_countdown = frame_interval;
}

void frame_wait(void)
{
#ifdef GCC
    Uint32 wait = syTmrGetCount();
    /* a flip waits out the first vsync of the frame it starts. a frame that
     * ends in time then finds 0 vsyncs left or more, and a frame n vsyncs
     * late finds -n */
    int left = frame_countdown;

#endif
    while (frame_countdown > 0) {
    }
    frame_countdown = frame_interval;
#ifdef GCC
    frame_end(wait, left < 0 ? -left : 0);
#endif
    /* @hack spelling: draw_off compared with TRUE, CMP/EQ #1 (0x8c054a24).
     * every store to draw_off is TRUE or FALSE. as a plain test, SHC writes TST
     * and 10 of 51 words differ */
#ifdef GCC
    /* a split frame that dropped its second picture holds its first */
    if (draw_off == TRUE || present_hold()) {
#else
    if (draw_off == TRUE) {
#endif
        kmResetPointer();
    } else {
        bitmap_flush_queue();
        _nj_wait_vsync_in_ = syTmrGetCount();
        kmRender();
        _nj_calc_vertex_ = 0;
        _nj_calc_polygon_ = 0;
        _nj_draw_polygon_ = 0;
        _nj_vsync_count_++;
        kmFlipFrameBuffer();
    }
    pdExecPeripheralServer();
#ifdef GCC
    overlay_pad();
#endif
    _nj_wait_vsync_out_ = syTmrGetCount();
}

void frame_wait_step(int left)
{
    while (frame_countdown > left) {
    }
    pdExecPeripheralServer();
#ifdef GCC
    overlay_pad();
#endif
    sound_send_queue();
}

static void act_idle(void)
{
}

void frame_vsync(void)
{
    Uint16 stage_act;
    Uint16 act;
    Uint16 stage;

    frame_countdown--;
#ifdef GCC
    /* at a skip's interval of 0, frame_wait sets frame_countdown to 0, so it
     * is below 0 at every test here. the act's hook and the sound server
     * then run at each vsync, as at an interval of 1, and so they do while
     * frames split, at each picture's vsync */
    if (frame_countdown == 0 || frame_interval == 0 || frame_split) {
#else
    if (frame_countdown == 0) {
#endif
        stage_act = stage_get_stage_act();
        act = stage_act & 0xff;
        stage = STAGE_OF(stage_act);
        act_hooks[stage][act]();
        sdSysServer();
    }
    buExecServer();
    loader_poll_drive();
}

/* set windy's fog. while a demo plays, it sets only a white fog */
static void windy_set_fog(void)
{
    Uint32 color;

    color = windy_fog_shown;
    if (!demo_playing || color == 0xffffffff) {
        njSetFogColor(color);
        njSetFogTable(windy_fog_table);
    }
}

static void sky_deck_set_fog(void)
{
    njSetFogColor(sky_deck_fog_color);
    njSetFogTable(sky_deck_fog_table);
}

static void ruin_set_fog(void)
{
    njSetFogColor(ruin_fog_color);
    njSetFogTable(ruin_fog);
}

static void mystic_ruins_set_fog(void)
{
    njSetFogTable(jungle_fog_table);
    njSetFogColor(jungle_fog_shown);
}

void fun_8c054be0(void)
{
}

void fun_8c054be4(void)
{
}
