/* EV0088, Knuckles' story: after the fight with Chaos 2, Chaos's puddle flows
 * to Eggman. Eggman tells Knuckles that Sonic is after the Master Emerald
 * too */
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/models.h"
#include "chaos/puddle.h"
#include "ev0088/eggman_action.h"
#include "ev0088/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 45
#define TEXT_3_LENGTH 33
#define TEXT_4_LENGTH 41
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 37
#define TEXT_8_LENGTH 59
#define TEXT_9_LENGTH 15
#define TEXT_10_LENGTH 65
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 29
#define TEXT_13_LENGTH 1
#define TEXT_14_LENGTH 3
#define TEXT_15_LENGTH 12
#define TEXT_16_LENGTH 1
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 1
#define TEXT_20_LENGTH 25
#include "ev0088/script/text.inc"

/* the puddle task's work->mode: what its step does */
enum puddle_mode {
    /* swing the scale between 7/8 and 9/8, one swing each 60 steps */
    PUDDLE_WOBBLE,
    /* add record->speed to the position each step, as the y scale swings
     * between 1 and 2. once the count passes record->steps, wobble */
    PUDDLE_MOVE,
    /* shrink and fade by the record's shares each step, wobbling three times
     * as fast. once the count passes record->steps, stop */
    PUDDLE_VANISH,
    /* return at once, and leave the drawn puddle as the vanish left it */
    PUDDLE_GONE
};

/* what the puddle task keeps besides its work. ev0088_puddle_create
 * allocates it, unchecked, and the task's destructor frees it */
struct puddle_record {
    struct task *drawn; /* the task puddle_create made, which draws it */
    NJS_VECTOR speed; /* how far a move goes each step */
    /* the steps a move divides its distance by, or a vanish its scale and
     * alpha */
    Sint32 steps;
    Float scale; /* the vanish's factor on the wobble's scale, from 1 down */
    Float scale_step; /* how much the vanish takes from scale each step */
    Uint8 alpha; /* the alpha the draw gives chaos_puddle's material */
    Uint8 alpha_step; /* how much the vanish takes from alpha each step */
};

/* the puddle task's work->vars */
struct puddle_vars {
    Float phase; /* wobble phase, in degrees */
    Sint32 timer; /* the steps PUDDLE_MOVE or PUDDLE_VANISH has run */
    struct puddle_record *record;
};

#define VARS(work) ((struct puddle_vars *)&(work)->vars)
#define PHASE_ANG(work) ((Angle)(VARS(work)->phase * ANGLE_PER_DEGREE))

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_walk_action;
extern NJS_ACTION knuckles_run_action;
extern NJS_ACTION dat_8c59d224; /* an action that moves eggman_object */

extern NJS_TEXLIST event_chaos0_water_texlist;
extern NJS_ACTION ev0088_knuckles_action_0;
extern NJS_ACTION ev0088_knuckles_action_1;
extern NJS_ACTION ev0088_knuckles_action_2;
extern NJS_ACTION ev0088_knuckles_action_3;
extern NJS_ACTION ev0088_knuckles_action_4;
extern NJS_ACTION ev0088_knuckles_action_5;

/* player 6's pad as Eggman walks, a key a step. the 4th to 43rd keys push the
 * stick near full stroke at an ang of about -67. the 74th to 77th push it
 * lightly at about 56, nearly the opposite ang. the last 3 push it at 56 again,
 * harder each step */
static const struct event_pad_key ev0088_eggman_walk_keys[109] = {
#include "ev0088/script/ev0088_eggman_walk_keys.inc"
};

static const struct event_pad_recording ev0088_eggman_walk = {
    ev0088_eggman_walk_keys,
    sizeof(ev0088_eggman_walk_keys) / sizeof(ev0088_eggman_walk_keys[0])};

/* the black fade that ends the event */
static struct task *ev0088_fade = NULL;
/* the task that moves the puddle */
static struct task *ev0088_puddle = NULL;

static void ev0088_puddle_init(struct task *task);

/* start the puddle task wobbling at x, y, z, facing ang_y. puddle_create
 * makes the task that draws it. the tasks and the record are allocated
 * unchecked */
static void ev0088_puddle_create(Float x, Float y, Float z, Angle ang_y)
{
    struct puddle_record *record;
    struct task_work *work;

    ev0088_puddle = task_create(TASK_WORK, 5, ev0088_puddle_init);
    record = heap_alloc(sizeof(struct puddle_record));
    work = ev0088_puddle->work;
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

/* destroy the task that draws the puddle now. kill the puddle task, which
 * destroys itself on its next step, and clear its slot. without a puddle
 * task it does nothing */
static void ev0088_puddle_kill(void)
{
    struct task_work *work;
    struct puddle_record *record;

    if (ev0088_puddle != NULL) {
        work = ev0088_puddle->work;
        record = VARS(work)->record;
        if (record->drawn != NULL) {
            task_destroy(record->drawn);
            record->drawn = NULL;
        }
        /* @hack redundant: the slot tested again. only this file writes it, and
         * nothing does between the two tests, so this one always passes. SHC
         * then loads and tests the slot a second time (MOV.L @R14,R4 at
         * 0x0cb800aa and TST R4,R4 at 0x0cb800ac). without it,
         * ev0088_puddle_kill runs 8 bytes short, and 28 of its 30 words
         * differ */
        if (ev0088_puddle != NULL) {
            task_kill(ev0088_puddle);
            ev0088_puddle = NULL;
        }
    }
}

/* move the puddle a 1/steps share of the way to x, y, z each step, then
 * wobble. the move runs steps + 1 steps, so it stops one share past x, y,
 * z. steps must be positive, and the puddle task must exist */
static void ev0088_puddle_move(Float x, Float y, Float z, int steps)
{
    struct task_work *work = ev0088_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->speed.x = (x - work->pos.x) / (Float)steps;
    record->speed.y = (y - work->pos.y) / (Float)steps;
    record->speed.z = (z - work->pos.z) / (Float)steps;
    record->steps = steps;
    VARS(work)->timer = 0;
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    work->mode = PUDDLE_MOVE;
}

/* nothing calls it. SHC leaves out a static function its file never calls.
 * this one is extern so that it keeps its place in the image, at 0x0cb8010e */
void ev0088_puddle_vanish(int steps)
{
    struct task_work *work = ev0088_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->scale = 1.0F;
    record->scale_step = 1.0F / (Float)steps;
    record->steps = steps;
    record->alpha_step = record->alpha / steps;
    VARS(work)->timer = 0;
    work->mode = PUDDLE_VANISH;
}

static void ev0088_puddle_step(struct task *task);
static void ev0088_puddle_draw(struct task *task);
static void ev0088_puddle_destroy(struct task *task);

static void ev0088_puddle_init(struct task *task)
{
    task->step = ev0088_puddle_step;
    task->draw = ev0088_puddle_draw;
    task->destructor = ev0088_puddle_destroy;
}

static void ev0088_puddle_step(struct task *task)
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
    ev0088_puddle_draw(task);
}

/* give chaos_puddle's material the record's alpha. puddle_move then gives
 * the drawn puddle the puddle task's place and scale. the draw reads the
 * puddle task through ev0088_puddle, not through task */
static void ev0088_puddle_draw(struct task *task)
{
    struct task_work *work = ev0088_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    /* @hack spelling: chaos_puddle through its address, which keeps the
     * object's own address in the pool word at 0x0cb803f0. the code then
     * reads its model at offset 4, with MOV.L @(4,R2),R1 at 0x0cb80354. by
     * name, the pool word holds the model member's address, and 9 of 21
     * words differ */
    (&chaos_puddle)->model->mats->diffuse.argb.a = record->alpha;
    puddle_move(&work->pos, &work->scl);
}

static void ev0088_puddle_destroy(struct task *task)
{
    if (VARS(task->work)->record != NULL) {
        heap_free(VARS(task->work)->record);
    }
}

void ev0088_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_0087.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGGMAN);
        event_set_pos(event_get_player(0), 13.0F, 0.0F, 78.7F);
        event_set_ang(event_get_player(0), 0, 0x8200, 0);
        event_play_action(event_get_player(0), &ev0088_knuckles_action_2,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(1);
        ev0088_puddle_create(12.0F, event_get_player(0)->work->pos.y, 65.0F, 0);
        event_create_player(6, eggman_step, 3.04F, 0.0F, 31.07F, 0, 0x0600, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 22.5F, 8.55F, 86.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1b00, 0x0a00, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 22.5F, 8.55F, 86.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf600, 0x1f00, 0);
        event_wait(120);
        event_talk_stop(event_get_player(6));
        event_talk_start(event_get_player(6), TEXT_13);
        event_voice_play(1578);
        event_message_wait(90, TEXT_1);
        event_sound_start(0, 1226, 1800);
        /* the puddle flows to Eggman */
        ev0088_puddle_move(6.94F, 0.0F, 42.53F, 120);
        event_talk_stop(event_get_player(6));
        event_message_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 13.7F, 23.32F, 68.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc100, 0x0900, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xfc00, 0x0900, 0);
        event_wait(60);
        event_sound_stop(0);
        event_wait(1);
        event_voice_play(1579);
        event_play_action(event_get_player(6), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 16);
        event_play_action(event_get_player(6), &ev0088_eggman_action,
                          &eggman_texlist, 1.0F, 0, 8);
        event_message_wait(150, TEXT_2);
        event_talk_stop(event_get_player(6));
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_walk_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 10.0F, 0.0F, 68.0F, 0.0F, 0.0F);
        ev0088_puddle_kill();
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 11.21F, 5.5F, 20.83F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x7800, 0);
        event_clear_motions(event_get_player(6));
        event_camera_follow(event_get_player(0));
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, 6.14F, 5.5F, 21.19F);
        event_play_action(event_get_player(0), &ev0088_knuckles_action_0,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &ev0088_knuckles_action_1,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(16);
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1580);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_wait(20);
        event_talk_stop(event_get_player(6));
        event_talk_start(event_get_player(6), TEXT_15);
        event_voice_play(1581);
        event_message_wait(60, TEXT_4);
        event_talk_stop(event_get_player(6));
        event_message_close();
        event_voice_play(1582);
        event_message_show(TEXT_5);
        event_wait(1);
        event_set_pos(event_get_player(0), 13.0F, 0.0F, 78.7F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 21.05F, 3.88F, 69.36F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0x6500, 0);
        event_wait(20);
        event_message_clear();
        event_wait(1);
        event_voice_wait();
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1637);
        event_message_show(TEXT_6);
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_message_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 25.41F, 8.91F, 88.73F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf700, 0x1e00, 0);
        event_wait(10);
        event_play_action(event_get_player(0), &ev0088_knuckles_action_3,
                          &knuckles_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0088_knuckles_action_4,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1583);
        event_message_show(TEXT_7);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 40.15F, 2.63F, 83.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0800, 0x2600, 0);
        event_talk_stop(event_get_player(6));
        event_talk_start(event_get_player(6), TEXT_18);
        event_voice_play(1584);
        event_message_wait(140, TEXT_8);
        event_talk_stop(event_get_player(6));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, 0, 8);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(1585);
        event_message_show(TEXT_9);
        event_clear_motions(event_get_player(6));
        event_play_pad(6, &ev0088_eggman_walk);
        event_wait(20);
        event_wait(30);
        event_message_close();
        event_wait(30);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c59d224, &eggman_texlist,
                          1.0F, EVENT_REPEAT, 32);
        event_voice_play(1586);
        event_message_wait(120, TEXT_10);
        event_sound_start(1, 1226, 60);
        event_voice_play(1587);
        event_message_show(TEXT_11);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 31.47F, 2.47F, 73.47F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x4c00, 0);
        event_camera_follow(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 0.7F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 9.55F, 0.0F, 46.65F, 0.5F,
                           1.0F);
        event_wait(1);
        event_play_action(event_get_player(0), &ev0088_knuckles_action_5,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 9.55F, 0.0F, 46.65F, 0.1F,
                           0.3F);
        event_wait(20);
        event_message_close();
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, 0, 8);
        event_camera_follow_clear();
        event_wait(45);
        event_sound_stop(1);
        event_kill_player(6);
        event_look_at(event_get_player(0), event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 20.0F,
                      event_get_player(0)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 62.68F, 13.02F, 31.19F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x4800, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 400, -14.62F, 33.72F, -8.72F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 400, 0xf800, 0x8e00, 0);
        event_clear_motions(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1588);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        ev0088_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        EVENT_KILL(ev0088_fade);
        event_reset_player(0);
        event_kill_player(6);
        ev0088_puddle_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0088_textures[] = {
    {"EGGMAN", &eggman_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
