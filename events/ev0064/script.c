/* EV0064, Amy's story: Zero catches Amy and carries her off */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/twinkle_elevator.h"
#include "draw/tree.h"
#include "enemy/zero.h"
#include "ev0064/amy_action_5.h"
#include "ev0064/bird_action.h"
#include "ev0064/script.h"
#include "event/event.h"
#include "event/play.h"
#include "object/models.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/zero.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 53
#define TEXT_2_LENGTH 32
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 48
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 6
#include "ev0064/script/text.inc"

/* @hack type: FLAG_ELEVATOR's request as a Sint8, though event_flags holds
 * Uint8s. SHC then loads the flag's address into R11 (0x0cb8026c) and the
 * angle 0xc000 into R10 (0x0cb80296). as event_flags[FLAG_ELEVATOR], the two
 * registers trade, and 18 of ev0064_script's 2165 words differ */
#define ELEVATOR_REQUEST (((Sint8 *)event_flags)[FLAG_ELEVATOR])

enum shadow_mode {
    SHADOW_STILL,
    SHADOW_GROW, /* to its full scale, then it shrinks */
    SHADOW_SHRINK /* back to a scale of 1, then it stays */
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

extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev0064_amy_action_0;
extern NJS_ACTION ev0064_amy_action_1;
extern NJS_ACTION ev0064_amy_action_2;
extern NJS_ACTION ev0064_amy_action_3;
extern NJS_ACTION ev0064_amy_action_4;

static struct task *ev0064_zero = NULL;
static struct task *ev0064_bird = NULL;
static struct task *ev0064_shadows[2] = {NULL, NULL};

static void ev0064_shadow_init(struct task *task);
static void ev0064_shadow_step(struct task *task);
static void ev0064_shadow_draw(struct task *task);
static void ev0064_shadow_destroy(struct task *task);

/* create a shadow at x, y, z in ev0064_shadows[slot], which must be NULL.
 * it grows from a scale of 1 to full over grow_steps steps. it then shrinks
 * back to 1 over shrink_steps steps. a grow_steps of 0, as the second shadow
 * has, divides by 0. the quotient, +inf, takes the shadow to full at its
 * first step. it stores through NULL when task_create or heap_alloc fails */
static void ev0064_shadow_create(Sint8 slot, Float x, Float y, Float z,
                                 int grow_steps, int shrink_steps, Float full)
{
    struct task_work *work;
    struct shadow_growth *growth;

    ev0064_shadows[slot] = task_create(TASK_WORK, 6, ev0064_shadow_init);
    ev0064_shadow_init(ev0064_shadows[slot]);
    work = ev0064_shadows[slot]->work;
    work->mode = SHADOW_GROW;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    growth = VARS(work)->growth;
    /* @bug arithmetic: divides by 0 for the second shadow, whose grow_steps
     * is 0 (FDIV FR2,FR1 at 0x0cb80072) */
    growth->grow = (full - 1.0F) / (Float)grow_steps;
    growth->shrink = (full - 1.0F) / (Float)shrink_steps;
    growth->full = full;
    growth->scale = 1.0F;
}

static void ev0064_shadow_kill(Sint8 slot)
{
    if (ev0064_shadows[slot] != NULL) {
        task_destroy(ev0064_shadows[slot]);
        ev0064_shadows[slot] = NULL;
    }
}

static void ev0064_shadow_init(struct task *task)
{
    struct task_work *work = task->work;
    struct shadow_growth *growth = heap_alloc(sizeof(struct shadow_growth));

    VARS(work)->growth = growth;
    task->step = ev0064_shadow_step;
    task->draw = ev0064_shadow_draw;
    task->destructor = ev0064_shadow_destroy;
}

static void ev0064_shadow_step(struct task *task)
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
    ev0064_shadow_draw(task);
    /* @hack redundant: a return where control returns anyway. it keeps the tail
     * call's BRA to ev0064_shadow_draw at 0x0cb80134. without it, SHC drops
     * the BRA and falls through into ev0064_shadow_draw, which follows. the
     * function then runs 4 bytes short, and 2 of its 43 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void ev0064_shadow_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct shadow_growth *growth = VARS(work)->growth;

    texture_set_regular();
    njPushMatrix(NULL);
    /* @bug mismatched-call: njClearMatrix takes no argument, and gets NULL in
     * R4 (MOV #0,R4 at 0x0cb80152), which it ignores */
    /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
    njClearMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y,
                work->pos.z + growth->scale - 1.0F);
    njScale(NULL, growth->scale * 0.5F, 1.0F, growth->scale);
    draw_tree(shadow_object);
    njPopMatrix(1);
}

static void ev0064_shadow_destroy(struct task *task)
{
    if (VARS(task->work)->growth != NULL) {
        heap_free(VARS(task->work)->growth);
    }
}

void ev0064_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        ELEVATOR_REQUEST = ELEVATOR_TO_TOP;
        sound_load_file("e_0064.mlt");
        event_reset_player(0);
        stream_request_music(MUSIC_S_SQUARE);
        event_set_pos(event_get_player(0), 389.5F, 44.5F, 1607.3F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.5F, EVENT_REPEAT, 0);
        event_place_actor(
            &ev0064_bird, event_get_player(0)->work->pos.x - 0.5F,
            event_get_player(0)->work->pos.y + 0.9F,
            event_get_player(0)->work->pos.z, event_get_player(0)->work->pos.x,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_wait(1);
        event_set_flags(ev0064_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0064_bird, &ev0064_bird_action,
                          &ev0064_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0064_bird, event_get_player(0)->work->pos.x + 2.5F,
                      event_get_player(0)->work->pos.y + 6.8F,
                      event_get_player(0)->work->pos.z - 3.5F);
        event_set_ang(ev0064_bird, 0, 0xc000, 0);
        event_wait(1);
        event_follow(ev0064_bird, 2.5F, 6.8F, -3.5F, 200, event_get_player(0));
        event_create_player(3, zero_event_init, 395.1F, 0.0F, 1351.0F, 0,
                            0xc000, 0);
        event_wait(1);
        event_set_pos(event_get_player(3), 395.1F, 0.0F, 1251.0F);
        event_set_ang(event_get_player(3), 0, 0xc000, 0);
        ELEVATOR_REQUEST = ELEVATOR_GO;
        event_wait(1);
        ELEVATOR_REQUEST = ELEVATOR_STAY;
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 333.3F, 73.0F, 1571.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a00, 0xa7f4, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 331.5F, 2.0F, 1574.7F);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, 333.5F, 1.5F, 1583.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x0a00, 0x89f4, 0);
        event_play_action(event_get_player(0), &ev0064_amy_action_1,
                          &amy_texlist, 1.5F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0),
                           event_get_player(0)->work->pos.x - 48.0F,
                           event_get_player(0)->work->pos.y,
                           event_get_player(0)->work->pos.z, 0.6F, 0.06F);
        event_follow(ev0064_bird, 2.5F, 6.8F, -3.0F, 120, event_get_player(0));
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.2F, EVENT_REPEAT, 0);
        event_look_at(
            event_get_player(0), event_get_player(0)->work->pos.x - 32.0F,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 24.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 16.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(20);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 16.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 24.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(
            event_get_player(0), event_get_player(0)->work->pos.x - 32.0F,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 24.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 16.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 2.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(20);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 16.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 24.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_wait(1);
        event_look_at(
            event_get_player(0), event_get_player(0)->work->pos.x - 32.0F,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 24.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 16.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(30);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 16.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(event_get_player(0),
                      event_get_player(0)->work->pos.x - 24.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 10.0F);
        event_wait(1);
        event_look_at(
            event_get_player(0), event_get_player(0)->work->pos.x - 32.0F,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_play_action(event_get_player(0), &ev0064_amy_action_1,
                          &amy_texlist, 1.1F, EVENT_REPEAT, 0);
        event_wait(2);
        event_set_pos(ev0064_bird, event_get_player(0)->work->pos.x + 3.5F,
                      event_get_player(0)->work->pos.y + 6.8F,
                      event_get_player(0)->work->pos.z + 3.5F);
        event_set_ang(ev0064_bird, 0, 0x8000, 0);
        event_follow(ev0064_bird, 3.5F, 6.8F, 3.5F, 200, event_get_player(0));
        event_run_to_speed(
            event_get_player(0), event_get_player(0)->work->pos.x,
            event_get_player(0)->work->pos.y,
            event_get_player(0)->work->pos.z - 40.0F, 0.6F, 0.06F);
        event_look_clear(event_get_player(0));
        event_wait(1);
        event_wait(40);
        event_run_clear(event_get_player(0));
        event_set_pos(event_get_player(0), 333.8F, 0.0F, 1420.3F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_wait(1);
        event_set_pos(ev0064_bird, event_get_player(0)->work->pos.x + 3.5F,
                      event_get_player(0)->work->pos.y + 6.8F,
                      event_get_player(0)->work->pos.z + 3.5F);
        event_set_ang(ev0064_bird, 0, 0x8000, 0);
        event_follow(ev0064_bird, 3.5F, 6.8F, 3.5F, 200, event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 330.3F, 1.2F, 1405.55F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x8534, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 20, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_follow(event_get_player(0));
        event_run_to_speed(event_get_player(0), 333.0F, 0.0F, 1338.1F, 0.4F,
                           0.06F);
        event_voice_play(1408);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_message_show(TEXT_1);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 43,
                             event_get_player(0)->work->pos.x - 9.0F,
                             event_get_player(0)->work->pos.y + 8.0F,
                             event_get_player(0)->work->pos.z - 2.0F);
        event_run_wait(event_get_player(0));
        event_voice_play(1409);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_message_show(TEXT_2);
        event_play_action(event_get_player(0), &ev0064_amy_action_2,
                          &amy_texlist, 1.1F, 0, 0);
        event_play_action(event_get_player(0), &ev0064_amy_action_3,
                          &amy_texlist, 1.1F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 325.6F, 18.5F, 1356.4F);
        event_wait(120);
        event_camera_look_clear();
        event_talk_stop(event_get_player(0));
        event_wait(15);
        event_message_close();
        event_set_pos(ev0064_bird, event_get_player(0)->work->pos.x + 3.5F,
                      event_get_player(0)->work->pos.y + 6.8F,
                      event_get_player(0)->work->pos.z + 3.5F);
        event_set_ang(ev0064_bird, 0, 0x8000, 0);
        event_follow(ev0064_bird, 3.5F, 6.8F, 3.5F, 200, event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 327.5F, 18.5F, 1349.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xe49c, 0xf586, 0xf500);
        event_sound_play(1229, 128, 0, 0);
        ev0064_shadow_create(0, event_get_player(0)->work->pos.x - 1.0F,
                             event_get_player(0)->work->pos.y + 12.0F,
                             event_get_player(0)->work->pos.z + 10.0F, 100, 300,
                             6.0F);
        event_wait(10);
        stream_stop_music();
        event_wait(100);
        event_play_action(event_get_player(0), &ev0064_amy_action_4,
                          &amy_texlist, 1.1F, 0, 0);
        event_play_action(event_get_player(0), &ev0064_amy_action_5,
                          &amy_texlist, 1.1F, EVENT_REPEAT, 0);
        event_wait(40);
        ev0064_shadow_kill(0);
        ev0064_shadow_create(1, event_get_player(0)->work->pos.x,
                             event_get_player(0)->work->pos.y + 0.1F,
                             event_get_player(0)->work->pos.z + 5.0F, 0, 3600,
                             7.0F);
        event_set_pos(event_get_player(3), 333.0F, 10.0F, 1359.0F);
        event_set_ang(event_get_player(3), 0, 0x8000, 0);
        event_set_ang(event_get_player(0), 0, 0x3200, 0);
        event_clear_motions(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F,
                           10.0F, 0.0F);
        event_set_pos(ev0064_bird, event_get_player(0)->work->pos.x + 6.3F,
                      event_get_player(0)->work->pos.y + 6.8F,
                      event_get_player(0)->work->pos.z - 4.4F);
        /* the bird's angles are Amy's position, which the prototype converts
         * implicitly to Angle. her x and z, near 333 and 1338, tilt the bird
         * by about 2 and 7 degrees */
        /* @hack redundant: 0x0000 subtracted from her y. SHC compiles the
         * subtraction as the FADD at 0x0cb81162, which adds the -0.0 at
         * 0x0cb812b8. without it, ev0064_script runs 12 bytes short, and 535
         * of its 2165 words differ */
        event_set_ang(ev0064_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0x0000,
                      event_get_player(0)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 335.1F, 8.1F, 1346.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x189c, 0x7186, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 16, 341.6F, 0.7F, 1329.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 16, 0x0f9c, 0x7186, 0xfa00);
        event_wait(30);
        event_sound_play(16, 60, 0, 0);
        vibration_start(0, 2, 70, 2);
        event_wait(30);
        stream_request_music(MUSIC_EGGROBO);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 333.3F, 1.3F, 1328.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x069c, 0x7f86, 0);
        event_set_pos(ev0064_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 3.8F,
                      event_get_player(0)->work->pos.z - 0.5F);
        event_set_ang(ev0064_bird, 0, 0x8000, 0);
        event_follow(ev0064_bird, 0.0F, 3.8F, -0.5F, 40, event_get_player(0));
        event_look_clear(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 332.5F, 12.0F, 1328.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x039c, 0x7f86, 0);
        event_play_action(event_get_player(0), &ev0064_amy_action_0,
                          &amy_texlist, 3.0F, EVENT_REPEAT, 0);
        event_voice_play(1410);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_message_show(TEXT_3);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, 332.5F, 14.0F, 1349.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0x049c, 0x7f86, 0);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_kill_player(3);
        event_kill_task(&ev0064_bird);
        event_place_task(&ev0064_zero, event_zero_create, 330.0F, 11.0F,
                         1205.9F, 0, 0xd000, 0);
        event_wait(1);
        event_set_flags(ev0064_zero, EVENT_IGNORE_GROUND);
        event_zero_set_speed(1.1F);
        event_set_pos(ev0064_zero, 330.0F, 11.0F, 1205.9F);
        event_set_ang(ev0064_zero, 0, 0x9000, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x399a);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 323.05F, 24.1F, 1187.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1211, 0xae2c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 323.5F, 14.1F, 1198.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1211, 0xae2c, 0);
        event_wait(70);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x33e9);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 324.5F, 17.3F, 1177.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0011, 0x882c, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 323.5F, 4.4F, 1170.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1411, 0x882c, 0xff00);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 143.2F, 4.0F, 1644.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a00, 0x07f4, 0);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 197.3F, 3.8F, 1622.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0500, 0x1500, 0);
        event_move_turn(ev0064_zero, 192.6F, 16.0F, 1617.2F, 71.5F, 24.8F,
                        1461.4F, 0, 0xd000, 0, 0, 0xd000, 0, 140);
        event_voice_play(1411);
        event_message_show(TEXT_4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 161.5F, 4.8F, 1577.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x0500, 0x1800, 0);
        event_voice_play(1412);
        event_wait(60);
        event_move_turn(ev0064_zero, 71.5F, 18.8F, 1461.4F, 48.5F, 35.8F,
                        1398.4F, 0, 0xd000, 0, 0, 0xd000, 0, 75);
        event_wait(75);
        event_move_stop_all();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        ELEVATOR_REQUEST = ELEVATOR_STAY;
        event_reset_player(0);
        event_kill_player(3);
        event_kill_task(&ev0064_bird);
        event_kill_task(&ev0064_zero);
        ev0064_shadow_kill(0);
        ev0064_shadow_kill(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0064_textures[] = {
    {"VER2_WING", &ev0064_bird_texlist},
    {"AMY_EGGROBO", &event_zero_texlist},
    {"EGGROB", &zero_texlist},
    {NULL, NULL},
};
