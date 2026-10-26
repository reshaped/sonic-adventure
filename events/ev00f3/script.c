/* EV00F3: Knuckles finds Angel Island falling though the Master Emerald is
 * whole, and Eggman brings him word of Chaos */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/master_emerald.h"
#include "chaos/models.h"
#include "chaos/puddle.h"
#include "ev00f3/eggman_action.h"
#include "ev00f3/emerald_textures.h"
#include "ev00f3/script.h"
#include "ev00f3/yellow_emerald_object.h"
#include "ev00f3/yellow_emerald_textures.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/knuckles.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 46
#define TEXT_2_LENGTH 51
#define TEXT_3_LENGTH 28
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 63
#define TEXT_6_LENGTH 31
#define TEXT_7_LENGTH 32
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 30
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 4
#define TEXT_15_LENGTH 2
#define TEXT_16_LENGTH 4
#define TEXT_17_LENGTH 1
#define TEXT_18_LENGTH 1
#include "ev00f3/script/text.inc"

/* the Chaos Emeralds' prop slots */
enum {
    BLUE_EMERALD,
    GREEN_EMERALD,
    PURPLE_EMERALD,
    SKY_EMERALD,
    WHITE_EMERALD,
    YELLOW_EMERALD
};

/* the puddle's modes, in work->mode */
enum puddle_mode {
    PUDDLE_WOBBLE,
    /* the puddle moves by record->speed for record->steps + 1 steps, then
     * wobbles again. its y scale swings between 1 and 2 as it moves */
    PUDDLE_MOVE,
    /* the puddle shrinks and fades for record->steps + 1 steps */
    PUDDLE_VANISH,
    /* after a vanish the step changes nothing, so the puddle keeps its last
     * scale and alpha */
    PUDDLE_GONE
};

/* the puddle's heap record, which ev00f3_puddle_destroy frees */
struct puddle_record {
    struct task *drawn; /* the task puddle_create returned, which draws it */
    NJS_VECTOR speed; /* distance a move covers each step */
    Sint32 steps; /* steps a move or a vanish divides by */
    Float scale; /* scale factor during a vanish, from 1 down */
    Float scale_step; /* scale lost each step of a vanish */
    Uint8 alpha; /* the alpha the draw gives chaos_puddle's material */
    Uint8 alpha_step; /* alpha lost each step of a vanish */
};

/* the puddle's view of work->vars */
struct puddle_vars {
    Float phase; /* wobble phase, in degrees */
    Sint32 timer; /* steps the current move or vanish has run */
    struct puddle_record *record;
};

#define VARS(work) ((struct puddle_vars *)&(work)->vars)
#define PHASE_ANG(work) ((Angle)(VARS(work)->phase * ANGLE_PER_DEGREE))

extern NJS_ACTION knuckles_walk_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_TEXLIST event_chaos0_water_texlist;
extern NJS_ACTION ev00f3_knuckles_action_0;
extern NJS_ACTION ev00f3_knuckles_action_1;
extern NJS_ACTION ev00f3_knuckles_action_2;
extern NJS_ACTION ev00f3_knuckles_action_3;
extern NJS_ACTION ev00f3_knuckles_action_4;
extern NJS_ACTION ev00f3_knuckles_action_5;
extern NJS_ACTION ev00f3_knuckles_action_6;
extern NJS_ACTION ev00f3_knuckles_action_7;
extern NJS_TEXLIST ev00f3_blue_emerald_texlist;
extern NJS_OBJECT ev00f3_blue_emerald_object;
extern NJS_OBJECT ev00f3_green_emerald_object;
extern NJS_OBJECT ev00f3_purple_emerald_object;
extern NJS_OBJECT ev00f3_sky_emerald_object;
extern NJS_OBJECT ev00f3_white_emerald_object;

static struct task *ev00f3_puddle = NULL;
/* the black fade that ends the event */
static struct task *ev00f3_fade = NULL;

static void ev00f3_puddle_init(struct task *task);

/* make the wobbling puddle at x, y, z facing ang_y, and hold its task in
 * ev00f3_puddle. its record keeps the task puddle_create makes to draw it.
 * neither allocation is checked */
static void ev00f3_puddle_create(Float x, Float y, Float z, Angle ang_y)
{
    struct puddle_record *record;
    struct task_work *work;

    ev00f3_puddle = task_create(TASK_WORK, 5, ev00f3_puddle_init);
    record = heap_alloc(sizeof(struct puddle_record));
    work = ev00f3_puddle->work;
    VARS(work)->record = record;
    work->mode = PUDDLE_WOBBLE;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.y = ang_y;
    work->scl.x = 0.8F;
    work->scl.y = 0.8F;
    work->scl.z = 0.8F;
    VARS(work)->phase = 0.0F;
    record->drawn = puddle_create(work->ang.y);
    record->alpha = 0x99;
}

/* destroy the task that draws the puddle at once, and kill the puddle,
 * which destroys itself on its next step. with no puddle, do nothing */
static void ev00f3_puddle_kill(void)
{
    struct task_work *work;
    struct puddle_record *record;

    if (ev00f3_puddle != NULL) {
        work = ev00f3_puddle->work;
        record = VARS(work)->record;
        if (record->drawn != NULL) {
            task_destroy(record->drawn);
            record->drawn = NULL;
        }
        /* the image reloads the slot after task_destroy and tests it again
         * (0x0cb800aa, 0x0cb800ac) */
        if (ev00f3_puddle != NULL) {
            task_kill(ev00f3_puddle);
            ev00f3_puddle = NULL;
        }
    }
}

/* not static: the image keeps it at 0x0cb800c2, though nothing calls it. SHC
 * drops a static function nothing in its file calls */
void ev00f3_puddle_move(Float x, Float y, Float z, int steps)
{
    struct task_work *work = ev00f3_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->speed.x = (x - work->pos.x) / (Float)steps;
    record->speed.y = (y - work->pos.y) / (Float)steps;
    record->speed.z = (z - work->pos.z) / (Float)steps;
    record->steps = steps;
    VARS(work)->timer = 0;
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    work->mode = PUDDLE_MOVE;
}

/* not static: the image keeps it at 0x0cb8010e, though nothing calls it. SHC
 * drops a static function nothing in its file calls */
void ev00f3_puddle_vanish(int steps)
{
    struct task_work *work = ev00f3_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->scale = 1.0F;
    record->scale_step = 1.0F / (Float)steps;
    record->steps = steps;
    record->alpha_step = record->alpha / steps;
    VARS(work)->timer = 0;
    work->mode = PUDDLE_VANISH;
}

static void ev00f3_puddle_step(struct task *task);
static void ev00f3_puddle_draw(struct task *task);
static void ev00f3_puddle_destroy(struct task *task);

static void ev00f3_puddle_init(struct task *task)
{
    task->step = ev00f3_puddle_step;
    task->draw = ev00f3_puddle_draw;
    task->destructor = ev00f3_puddle_destroy;
}

static void ev00f3_puddle_step(struct task *task)
{
    struct task_work *work = task->work;
    struct puddle_record *record = VARS(work)->record;

    switch (work->mode) {
    case PUDDLE_WOBBLE:
        VARS(work)->phase += 6.0F;
        work->scl.x = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.y = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.z = 1.0F + (0.125F * njSin(PHASE_ANG(work)));
        break;
    case PUDDLE_MOVE:
        work->pos.x += record->speed.x;
        work->pos.y += record->speed.y;
        work->pos.z += record->speed.z;
        if (++VARS(work)->timer > record->steps) {
            work->mode = PUDDLE_WOBBLE;
        }
        VARS(work)->phase += 6.0F;
        work->scl.y = 1.5F + (0.5F * njCos(PHASE_ANG(work)));
        break;
    case PUDDLE_VANISH:
        VARS(work)->phase += 18.0F;
        record->scale -= record->scale_step;
        if (++VARS(work)->timer > record->steps) {
            work->mode = PUDDLE_GONE;
        }
        work->scl.x = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.y = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.z = 1.0F + (0.125F * njSin(PHASE_ANG(work)));
        work->scl.x *= record->scale;
        work->scl.y *= record->scale;
        work->scl.z *= record->scale;
        record->alpha -= record->alpha_step;
        break;
    case PUDDLE_GONE:
        return;
    }
    ev00f3_puddle_draw(task);
}

/* give chaos_puddle's material the puddle's alpha, and pass the puddle's
 * position and scale to the task puddle_create made. it reads the puddle
 * through ev00f3_puddle and ignores task */
static void ev00f3_puddle_draw(struct task *task)
{
    struct task_work *work = ev00f3_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    /* @hack spelling: the model as (&chaos_puddle)->model. SHC then pools
     * chaos_puddle's address (0x0cb803f0) and reads the model 4 bytes past
     * it (MOV.L @(4,R2),R1 at 0x0cb80354). as chaos_puddle.model, the pool
     * word holds the address of chaos_puddle's model field, and 9 of 21
     * words differ */
    (&chaos_puddle)->model->mats->diffuse.argb.a = record->alpha;
    puddle_move(&work->pos, &work->scl);
}

static void ev00f3_puddle_destroy(struct task *task)
{
    if (VARS(task->work)->record != NULL) {
        heap_free(VARS(task->work)->record);
    }
}

void ev00f3_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_00F3.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.67F, 14.19F, 2386.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xfc00, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xe600, 0xfc00, 0);
        event_prop_create(-5.77F, -152.58F, 2169.71F, 0, 0x4000, 0x2000, 1.0F,
                          1.0F, 1.0F, &ev00f3_blue_emerald_object,
                          &ev00f3_blue_emerald_texlist, BLUE_EMERALD);
        event_prop_create(-9.49F, -152.58F, 2170.79F, 0, 0xb000, 0xe000, 1.0F,
                          1.0F, 1.0F, &ev00f3_green_emerald_object,
                          &ev00f3_green_emerald_texlist, GREEN_EMERALD);
        event_prop_create(-13.94F, -152.58F, 2168.66F, 0, 0x2000, 0x2000, 1.0F,
                          1.0F, 1.0F, &ev00f3_purple_emerald_object,
                          &ev00f3_purple_emerald_texlist, PURPLE_EMERALD);
        event_prop_create(2.31F, -152.58F, 2169.24F, 0, 0x6000, 0xe000, 1.0F,
                          1.0F, 1.0F, &ev00f3_sky_emerald_object,
                          &ev00f3_sky_emerald_texlist, SKY_EMERALD);
        event_prop_create(6.98F, -152.58F, 2168.63F, 0, 0x8000, 0x2000, 1.0F,
                          1.0F, 1.0F, &ev00f3_white_emerald_object,
                          &ev00f3_white_emerald_texlist, WHITE_EMERALD);
        event_prop_create(11.79F, -152.58F, 2165.43F, 0, 0x4000, 0xe000, 1.0F,
                          1.0F, 1.0F, &ev00f3_yellow_emerald_object,
                          &ev00f3_yellow_emerald_texlist, YELLOW_EMERALD);
        event_set_pos(event_get_player(0), 151.09F, -179.73F, 2154.86F);
        event_create_player(2, knuckles_step, -2.96F, -157.08F, 2208.78F, 0, 0,
                            0);
        event_create_player(5, eggman_step, -38.05F, -262.58F, 2623.56F, 0x0884,
                            0x8000, 0x0044);
        event_set_flags(event_get_player(5), EVENT_IGNORE_GROUND);
        event_eggmobile_create(-22.02F, -258.52F, 2651.15F, 0x0884, 0xa000,
                               0x6000);
        event_wait(2);
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_WHOLE;
        event_play_action(event_get_player(5), &ev00f3_eggman_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        stream_request_music(MUSIC_THEKNKLS);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(118);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -17.73F, -68.6F, 2275.62F);
        event_talk_start(event_get_player(2), TEXT_11);
        event_message_show(TEXT_1);
        event_voice_play(1892);
        event_wait(1);
        event_voice_wait();
        event_message_clear();
        event_talk_stop(event_get_player(2));
        event_wait(30);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_1,
                          &knuckles_texlist, 0.3F, 0, 0);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -6.23F, -154.14F, 2215.12F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0xfa00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_wait(1);
        event_talk_start(event_get_player(2), TEXT_12);
        event_message_show(TEXT_2);
        event_voice_play(1893);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -3.39F, -150.72F,
                             2223.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0, 0xfe00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 100, 0x3111);
        event_voice_wait();
        event_message_clear();
        event_talk_stop(event_get_player(2));
        event_wait(30);
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1894);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_message_clear();
        event_talk_stop(event_get_player(2));
        event_wait(30);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_2,
                          &knuckles_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_talk_start(event_get_player(2), TEXT_14);
        event_message_show(TEXT_4);
        event_voice_play(1895);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_clear();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -9.38F, -149.77F, 2179.65F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 7.61F, -149.77F, 2179.65F);
        event_clear_motions(event_get_player(2));
        event_run_to_speed(event_get_player(2), -1.52F, -157.08F, 2180.37F,
                           0.3F, 0.0F);
        event_wait(10);
        event_message_show(TEXT_5);
        event_voice_play(1896);
        event_wait(1);
        event_voice_wait();
        event_message_clear();
        event_run_wait(event_get_player(2));
        event_wait(10);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_6,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 20.08F, -149.77F,
                             2189.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0, 0x2000, 0);
        event_talk_start(event_get_player(2), TEXT_15);
        event_message_show(TEXT_6);
        event_voice_play(1898);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_clear();
        event_wait(30);
        event_talk_start(event_get_player(2), TEXT_16);
        event_message_show(TEXT_7);
        event_voice_play(1900);
        event_wait(60);
        event_message_clear();
        event_wait(50);
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_4,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_5,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -20.23F, -124.58F, 2273.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf100, 0x7e00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x0222);
        event_wait(60);
        event_set_pos(event_get_player(2), -4.35F, -157.08F, 2246.73F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_6,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.82F, -143.55F, 2277.51F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf000, 0xfd00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_6,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(2), TEXT_17);
        event_message_show(TEXT_8);
        event_voice_play(1902);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_clear_motions(event_get_player(2));
        event_run_to_speed(event_get_player(2), -1.06F, -239.57F, 2398.72F,
                           3.0F, 3.0F);
        event_wait(20);
        event_run_clear(event_get_player(2));
        event_wait(40);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.915F, -256.77F,
                             2638.145F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe00, 0x2200, 0);
        event_set_pos(event_get_player(2), -19.96F, -257.14F, 2606.0F);
        event_set_ang(event_get_player(2), 0, 0xd000, 0);
        event_play_action(event_get_player(2), &knuckles_walk_action,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        ev00f3_puddle_create(12.58F, -264.7F, 2644.47F, 0);
        event_wait(1);
        event_run_to_speed(event_get_player(2), -29.0F, -260.6F, 2614.18F,
                           0.03F, 1.0F);
        event_run_wait(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_6,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(60);
        event_message_show(TEXT_9);
        event_voice_play(1903);
        event_wait(1);
        event_voice_wait();
        event_message_clear();
        event_wait(10);
        event_play_action(event_get_player(2), &ev00f3_knuckles_action_7,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(2), TEXT_18);
        event_message_show(TEXT_10);
        event_voice_play(1904);
        event_wait(1);
        event_voice_wait();
        event_message_clear();
        event_sound_start(0, 1225, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 26.59F, -261.28F, 2653.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0100, 0x2900, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 90, 0x1d28);
        event_wait(110);
        event_message_close();
        event_sound_stop(0);
        ev00f3_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        event_sound_play(1224, 0, 0, 0);
        if (ev00f3_fade != NULL) {
            event_fade_set_speed(ev00f3_fade, -1.0F);
        }
        event_wait(1);
        if (ev00f3_fade != NULL) {
            event_fade_set_speed(ev00f3_fade, 1.0F);
        }
        event_wait(60);
        event_sound_play(1223, 0, 0, 0);
        event_wait(10);
        EVENT_KILL(ev00f3_fade);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_eggmobile_kill();
        EVENT_KILL(ev00f3_fade);
        ev00f3_puddle_kill();
        event_prop_kill(BLUE_EMERALD);
        event_prop_kill(GREEN_EMERALD);
        event_prop_kill(PURPLE_EMERALD);
        event_prop_kill(SKY_EMERALD);
        event_prop_kill(WHITE_EMERALD);
        event_prop_kill(YELLOW_EMERALD);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(5);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f3_textures[] = {
    {"KNUCKLES", &knuckles_texlist},
    {"EGGMAN", &eggman_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"M_EM_BLUE", &ev00f3_blue_emerald_texlist},
    {"M_EM_GREEN", &ev00f3_green_emerald_texlist},
    {"M_EM_PURPLE", &ev00f3_purple_emerald_texlist},
    {"M_EM_SKY", &ev00f3_sky_emerald_texlist},
    {"M_EM_WHITE", &ev00f3_white_emerald_texlist},
    {"M_EM_YELLOW", &ev00f3_yellow_emerald_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
