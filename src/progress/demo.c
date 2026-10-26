/* Sega logo and paired attract-mode demos */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "draw/fade.h"
#include "draw/text.h"
#include "event/play.h"
#include "hud/hud.h"
#include "main.h"
#include "object/download.h"
#include "player/player.h"
#include "progress/demo.h"
#include "progress/progress.h"
#include "progress/recorder.h"
#include "progress/title.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/sbinit.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/timer.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 16
#include "progress/demo/text.inc"

/* the demos, in pairs of an event's demo and then a demo of recorded pads */
#define DEMO_COUNT 12

/* the logo in six pieces, three across and two down */
#define LOGO_PIECES 6

/* the logo fades in and shows for LOGO_FRAMES + 2 frames, then fades out.
 * the 2 come from logo_step's test, which reads the count before it rises
 * and passes only above LOGO_FRAMES. each fade moves LOGO_FADE_SPEED a
 * frame */
#define LOGO_FRAMES 150
#define LOGO_FADE_SPEED 8

/* whether demo n is the second of its pair, at an odd index. it compares n
 * with each odd index, as the image does, rather than test its low bit */
#define SECOND_OF_PAIR(n)                                                      \
    ((n) == 1 || (n) == 3 || (n) == 5 || (n) == 7 || (n) == 9 || (n) == 11)

enum logo_mode {
    LOGO_IDLE = 0,
    LOGO_SHOW = 1,
    LOGO_FADE_OUT = 2,
    LOGO_END = 15
};

struct demo {
    Sint16 stage;
    Sint16 act;
    Sint16 character;
    Sint16 event; /* or DEMO_NO_EVENT, for recorded pads */
};

static NJS_TEXNAME logo_textures[LOGO_PIECES] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
static NJS_TEXLIST logo_texlist = {
#include "progress/demo/logo_texlist.inc"
};

static NJS_TEXNAME press_start_textures[] = {{TEXT_7, 0, 0}};
NJS_TEXLIST press_start_texlist = {
#include "progress/demo/press_start_texlist.inc"
};

static NJS_TEXANIM logo_texanims[LOGO_PIECES] = {
#include "progress/demo/logo_texanims.inc"
};

Bool demo_playing = FALSE;
int demo_event = 0;
int demo_event_ended = 0;

/* in the order they play */
static struct demo demos[DEMO_COUNT] = {
    {STAGE_EGG_CARRIER, 2, CHARACTER_SONIC, 27},
    {STAGE_HIGHWAY, 0, CHARACTER_SONIC, DEMO_NO_EVENT},
    {STAGE_MYSTIC_RUINS, 0, CHARACTER_SONIC, 13},
    {STAGE_ICECAP, 2, CHARACTER_TAILS, DEMO_NO_EVENT},
    {STAGE_MYSTIC_RUINS, 0, CHARACTER_SONIC, 40},
    {STAGE_MOUNTAIN, 2, CHARACTER_KNUCKLES, DEMO_NO_EVENT},
    {STAGE_STATION_SQUARE, 1, CHARACTER_SONIC, 18},
    {STAGE_SHELTER, 0, CHARACTER_AMY, DEMO_NO_EVENT},
    {STAGE_EGG_CARRIER_INSIDE, 1, CHARACTER_E102, 183},
    {STAGE_WINDY, 0, CHARACTER_E102, DEMO_NO_EVENT},
    {STAGE_BEACH, 2, CHARACTER_BIG, 212},
    {STAGE_BEACH, 2, CHARACTER_BIG, DEMO_NO_EVENT},
};

static enum logo_mode logo_mode;
/* nothing reads or writes it. it holds the word before logo_frames in
 * bss */
static int dat_8c7a234c;
static int logo_frames;

static void logo_draw(void)
{
    NJS_SPRITE sprite;
    int i;

    sprite.tlist = &logo_texlist;
    sprite.tanim = logo_texanims;
    njSetTexture(sprite.tlist);
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 0.0F;
    for (i = 0; i < LOGO_PIECES; i++) {
        njDrawSprite2D(&sprite, i, -10.0F, 0);
    }
}

void logo_start(void)
{
    NJS_COLOR color;

    heap_init();
    task_init();
    title_fade_set(0);
    njSetBackColor(0, 0, 0);
    texture_load_pvm("SEGALOGO", &logo_texlist);
    logo_mode = LOGO_SHOW;
    logo_frames = 0;
    frame_pacing_init(1, 1);
    color.argb.a = 0;
    color.argb.r = 0;
    color.argb.g = 0;
    color.argb.b = 0;
    fade_set_cover(&color);
}

void boot_init(void)
{
    logo_start();
    demo_next = 0;
    demo_playing = FALSE;
    demo_event = 0;
    save_slot = 0;
    save_reset_all();
}

int logo_step(void)
{
    pad_enable();
    pad_scan();
    if (pad_quit_pressed(0)) {
        sbExitSystem();
        syBtExit();
    }
    task_step_all();
    switch (logo_mode) {
    case LOGO_SHOW:
        title_fade_step(LOGO_FADE_SPEED);
        if (logo_frames++ > LOGO_FRAMES) {
            logo_mode = LOGO_FADE_OUT;
        }
        title_fade_draw();
        logo_draw();
        break;
    case LOGO_FADE_OUT:
        if (title_fade_step(-LOGO_FADE_SPEED)) {
            logo_mode = LOGO_END;
        }
        title_fade_draw();
        logo_draw();
        break;
    case LOGO_END:
        logo_mode = LOGO_IDLE;
        njReleaseTexture(&logo_texlist);
        /* movie 0 plays before the first and fourth pairs, demos 0 and 6 */
        switch (demo_next) {
        case 0:
        case 6:
            return LOGO_MOVIE;
        default:
            return LOGO_ADVERTISE;
        }
    default:
        break;
    }
    return 0;
}

void demo_set_flags(void)
{
    event_flag_set(FLAG_SONIC_CLEAR_BEACH);
    event_flag_set(FLAG_KNUCKLES_SHOVEL_CLAWS);
    /* only for recorded pads */
    if (demo_event < 0) {
        event_flag_set(FLAG_E102_BOOSTER);
    }
    event_flag_set(FLAG_BIG_LIFE_BELT);
}

void demo_advance(void)
{
    Sint16 stage;
    Sint16 act;
    Sint16 character;

    card_init();
    stage = demos[demo_next].stage;
    act = demos[demo_next].act;
    character = demos[demo_next].character;
    demo_event = demos[demo_next].event;
    demo_playing = TRUE;
    hud_reset();
    progress_init();
    demo_set_flags();
    recording_kind = RECORDING_PLAY;
    demo_next++;
    if (demo_next >= DEMO_COUNT) {
        demo_next = 0;
    }
    stage_set_stage_act(stage, act);
    set_character(character);
    hud_hidden = -1;
    if (demo_event < 0) {
        switch (character) {
        case CHARACTER_SONIC:
            loader_read_file("K_SONIC.bin", DEMO_RECORDING);
            break;
        case CHARACTER_TAILS:
            loader_read_file("K_MILES.bin", DEMO_RECORDING);
            break;
        case CHARACTER_KNUCKLES:
            loader_read_file("K_KNUCK.bin", DEMO_RECORDING);
            break;
        case CHARACTER_AMY:
            loader_read_file("K_AMY.bin", DEMO_RECORDING);
            break;
        case CHARACTER_E102:
            loader_read_file("K_E102.bin", DEMO_RECORDING);
            break;
        case CHARACTER_BIG:
            loader_read_file("K_BIG.bin", DEMO_RECORDING);
            break;
        }
    }
}

void menu_begin(void)
{
    NJS_COLOR color;

    heap_init();
    task_init();
    demo_playing = FALSE;
    demo_event = 0;
    demo_event_ended = 0;
    recording_on = FALSE;
    recording_kind = RECORDING_OFF;
    text_style_init(NULL, 0, 0);
    download_reset();
    hud_hidden = 0;
    draw_off = FALSE;
    story_disable();
    story_event_delay = 0;
    if (save_read()) {
        save_load();
    }
    color.argb.a = 0;
    color.argb.r = 0;
    color.argb.g = 0;
    color.argb.b = 0;
    fade_set_cover(&color);
    stage_set_stage_act(STAGE_PRACTICE, 0);
}

int demo_next_mode(void)
{
    if (recording_kind == RECORDING_PLAY) {
        if (SECOND_OF_PAIR(demo_next)) {
            demo_advance();
            draw_off = TRUE;
            if (stage_begin() == STAGE_GROUP_FIELD) {
                return GAME_MODE_FIELD;
            }
            return GAME_MODE_ACTION;
        }
        logo_start();
        return GAME_MODE_LOGO;
    }
    if (recording_kind == RECORDING_OFF) {
        if (SECOND_OF_PAIR(demo_next)) {
            demo_next++;
            if (demo_next >= DEMO_COUNT) {
                demo_next = 0;
            }
        }
    }
    return 0;
}

static void demo_event_wait(struct task *task)
{
    if (!event_in_control()) {
        task_kill(task);
        demo_event_ended = -1;
    }
}

void demo_event_start(struct task *task)
{
    task->step = demo_event_wait;
    stream_destroy();
    /* @hack spelling: the two calls as one expression, which keeps the call
     * to event_start a JSR (0x8c0949d4); as two statements, SHC makes it a
     * tail jump and 5 of 15 words differ */
    event_load(demo_event), event_start();
}
