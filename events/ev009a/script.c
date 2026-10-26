/* EV009A, Knuckles' story: Knuckles believes he has every piece of the Master
 * Emerald. the transparent monster still troubles him. a flash halts him, and
 * beams of light rise as the screen turns white */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/tree.h"
#include "ev009a/script.h"
#include "event/event.h"
#include "event/play.h"
#include "object/models.h"
#include "player/textures.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 70
#define TEXT_2_LENGTH 31
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 3
#define TEXT_7_LENGTH 1
#include "ev009a/script/text.inc"

enum shadow_mode {
    SHADOW_STILL,
    SHADOW_GROW, /* the shadow's scale grows to full, then shrinks */
    SHADOW_SHRINK /* the shadow's scale shrinks back to 1, then stays */
};

struct shadow_growth {
    Float grow; /* added to scale each step while it grows */
    Float shrink; /* taken from scale each step while it shrinks */
    Float scale;
    Float full; /* the scale it grows to */
};

struct shadow_vars {
    Uint8 unk_00[8];
    struct shadow_growth *growth;
};

#define VARS(work) ((struct shadow_vars *)&(work)->vars)

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_sprint_action;
extern NJS_ACTION knuckles_skid_action;

extern NJS_ACTION ev009a_knuckles_action_0;
extern NJS_ACTION ev009a_knuckles_action_1;
extern NJS_ACTION ev009a_knuckles_action_2;
extern NJS_ACTION ev009a_knuckles_action_3;

/* a run that nothing plays, a key a step. the stick is pushed nearly all
 * the way from the 5th key. it is let go for the 45th to 52nd keys and the
 * 58th to 61st */
static const struct event_pad_key ev009a_run_keys[87] = {
#include "ev009a/script/ev009a_run_keys.inc"
};

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct event_pad_recording ev009a_run = {
    ev009a_run_keys, sizeof(ev009a_run_keys) / sizeof(ev009a_run_keys[0])};

/* the light's diffuse and ambient intensities, in tenths: ev009a_light_init
 * passes njSetLightIntensity 12.1 and -6.9 */
static Sint32 ev009a_light_diffuse = 121;
static Sint32 ev009a_light_ambient = -69;
static struct task *ev009a_light_task = NULL;
static struct task *ev009a_shadow = NULL;

static NJS_LIGHT ev009a_light;

static void ev009a_light_idle(struct task *task)
{
}

static void ev009a_light_destroy(struct task *task)
{
    njDeleteLight(&ev009a_light);
}

static void ev009a_light_init(struct task *task)
{
    njCreateLight(&ev009a_light,
                  NJD_SIMPLE_LIGHT | NJD_AMBIENT | NJD_DIR_LIGHT);
    njSetLightIntensity(&ev009a_light, 0.0F, ev009a_light_diffuse * 0.1F,
                        ev009a_light_ambient * 0.1F);
    njSetLightColor(&ev009a_light, 1.0F, 1.0F, 1.0F);
    njSetLightDirection(&ev009a_light, 0.0F, 0.0F, 1.0F);
    task->step = ev009a_light_idle;
    task->draw = ev009a_light_idle;
    task->destructor = ev009a_light_destroy;
}

static void ev009a_shadow_init(struct task *task);
static void ev009a_shadow_step(struct task *task);
static void ev009a_shadow_draw(struct task *task);
static void ev009a_shadow_destroy(struct task *task);

/* create the shadow at x, y, z in ev009a_shadow, which must be empty. its
 * scale along z grows from 1 to full by (full - 1) / grow a step. it then
 * shrinks back to 1 by (full - 1) / shrink a step, and stays there. it
 * waits a step for ev009a_shadow_init, which allocates the shadow_growth it
 * then fills. so call it on the event's thread. neither task_create nor
 * heap_alloc is checked */
static void ev009a_shadow_create(Float x, Float y, Float z, int grow,
                                 int shrink, Float full)
{
    struct task_work *work;
    struct shadow_growth *growth;

    ev009a_shadow = task_create(TASK_WORK, 5, ev009a_shadow_init);
    event_wait(1);
    work = ev009a_shadow->work;
    work->mode = SHADOW_GROW;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    growth = VARS(work)->growth;
    growth->grow = (full - 1.0F) / (Float)grow;
    growth->shrink = (full - 1.0F) / (Float)shrink;
    growth->full = full;
    growth->scale = 1.0F;
}

static void ev009a_shadow_init(struct task *task)
{
    struct task_work *work = task->work;
    struct shadow_growth *growth = heap_alloc(sizeof(struct shadow_growth));

    VARS(work)->growth = growth;
    task->step = ev009a_shadow_step;
    task->draw = ev009a_shadow_draw;
    task->destructor = ev009a_shadow_destroy;
}

static void ev009a_shadow_step(struct task *task)
{
    struct task_work *work = task->work;
    struct shadow_growth *growth = VARS(work)->growth;

    switch (work->mode) {
    case SHADOW_GROW:
        growth->scale += growth->grow;
        if (growth->scale > growth->full) {
            growth->scale = growth->full;
            work->mode = SHADOW_SHRINK;
        }
        break;
    case SHADOW_SHRINK:
        growth->scale -= growth->shrink;
        if (growth->scale < 1.0F) {
            growth->scale = 1.0F;
            work->mode = SHADOW_STILL;
        }
        break;
    }
    ev009a_shadow_draw(task);
    /* @hack redundant: a return where the function ends, which keeps the
     * image's BRA to ev009a_shadow_draw, the next function, at 0x0cb801d0.
     * without it, SHC falls through into the draw, the code ends 4 bytes short,
     * and 2 of 43 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* draws ev009a_shadow, whatever task it's called for */
static void ev009a_shadow_draw(struct task *task)
{
    struct task_work *work = ev009a_shadow->work;
    struct shadow_growth *growth = VARS(work)->growth;

    texture_set_regular();
    njPushMatrix(NULL);
    /* @bug mismatched-call: njClearMatrix takes no argument, but the call
     * passes it a null in R4, which it never reads (MOV #0,R4 at 0x0cb801f0,
     * the slot of the JSR at 0x0cb801ee) */
    /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
    njClearMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y,
                work->pos.z + growth->scale - 1.0F);
    njScale(NULL, 1.0F, 1.0F, growth->scale);
    draw_tree(shadow_object);
    njPopMatrix(1);
}

static void ev009a_shadow_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct shadow_growth *growth = VARS(work)->growth;

    if (growth != NULL) {
        heap_free(growth);
    }
}

void ev009a_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0008.mlt");
        event_sound_init(5);
        stream_request_music(MUSIC_EGCARER1);
        event_set_pos(event_get_player(0), -1.76F, 734.6F, 1044.35F);
        event_set_ang(event_get_player(0), 0, 0x8f00, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.137F, 737.85F, 1039.165F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x8b28, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -2.133F, 737.7876F,
                             1039.233F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x1700, 0x8128, 0xfd00);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1657);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, -11.55F, 738.27F, 1034.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x0500, 0x9b28, 0);
        event_wait(10);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_sprint_action,
                          &knuckles_texlist, 1.4F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -84.27F, 733.54F, 983.22F, 1.4F,
                           0.0F);
        event_wait(10);
        event_camera_follow(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1658);
        event_message_show(TEXT_2);
        event_wait(1);
        event_run_wait(event_get_player(0));
        event_white_start(1, 1);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_skid_action,
                          &knuckles_texlist, 1.0F, 0, 8);
        if (ev009a_light_task != NULL) {
            task_destroy(ev009a_light_task);
            ev009a_light_task = NULL;
        }
        event_wait_motions(event_get_player(0));
        event_look_at(event_get_player(0), -90.475F, 740.284F, 971.8888F);
        event_play_action(event_get_player(0), &ev009a_knuckles_action_0,
                          &knuckles_texlist, 1.5F, 0, 8);
        event_play_action(event_get_player(0), &ev009a_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_turn_by(event_get_player(0), 0, 0x0428, 0);
        event_white_start(1, 1);
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(8);
        if (ev009a_light_task != NULL) {
            task_destroy(ev009a_light_task);
            ev009a_light_task = NULL;
        }
        event_voice_play(1659);
        event_message_show(TEXT_3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -87.46F, 736.0F, 994.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0900, 0x0328, 0x0100);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 95, -90.16F, 740.588F,
                             971.096F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 95, 0x1555);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 95, 0x0400, 0x0328, 0x0100);
        event_wait(90);
        event_white_start(80, 80);
        event_rays_create(-125.0655F, 805.31F, 285.231F, 4, 20, 3.0F, 1.0F,
                          0x88ffffff, 0x88ffffff);
        event_sound_start(0, 1228, 1800);
        ev009a_shadow_create(event_get_player(0)->work->pos.x,
                             event_get_player(0)->work->pos.y,
                             event_get_player(0)->work->pos.z, 80, 80, 8.0F);
        event_play_action(event_get_player(0), &ev009a_knuckles_action_2,
                          &knuckles_texlist, 1.5F, 0, 1);
        ev009a_light_task = task_create(0, 1, ev009a_light_init);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 20, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, -90.753F, 740.662F,
                             1004.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0xf900, 0xf528, -0x0100);
        event_wait(20);
        event_voice_play(671);
        event_message_show(TEXT_4);
        event_play_action(event_get_player(0), &ev009a_knuckles_action_3,
                          &knuckles_texlist, 1.5F, EVENT_REPEAT, 16);
        event_message_close();
        event_wait(30);
        event_sound_stop(0);
        event_rays_kill();
        event_wait(60);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 12);
        event_wait(50);
        if (ev009a_shadow != NULL) {
            task_kill(ev009a_shadow);
        }
        if (ev009a_light_task != NULL) {
            task_destroy(ev009a_light_task);
            ev009a_light_task = NULL;
        }
        event_wait(10);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -84.3F, 733.54F, 983.186F);
        event_set_ang(event_get_player(0), 0, 0x9e39, 0);
        event_reset_player(0);
        /* @bug memory: EVENT_PLAY's kill leaves ev009a_shadow set
         * (0x0cb8084c), and the task destroys itself on its next step. this
         * kill then writes task_destroy into the step field of the freed task
         * record (0x0cb808a6). any task the pool has made since in that
         * record is killed */
        if (ev009a_shadow != NULL) {
            task_kill(ev009a_shadow);
        }
        event_rays_kill();
        event_white_kill();
        if (ev009a_light_task != NULL) {
            task_destroy(ev009a_light_task);
            ev009a_light_task = NULL;
        }
        event_sound_stop(0);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev009a_textures[] = {{NULL, NULL}};
