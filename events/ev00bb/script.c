/* EV00BB, E-102's story: Eggman takes Froggy from E-102 and beams E-103,
 * E-104 and E-105 away. he sends E-102 for the bird a girl hides in a
 * cell */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00bb/frog_action_1.h"
#include "ev00bb/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/beam.h"
#include "shared/e103.h"
#include "shared/e104.h"
#include "shared/e105.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 58
#define TEXT_3_LENGTH 53
#define TEXT_4_LENGTH 59
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 28
#define TEXT_7_LENGTH 34
#define TEXT_8_LENGTH 44
#define TEXT_9_LENGTH 43
#define TEXT_10_LENGTH 58
#define TEXT_11_LENGTH 45
#define TEXT_12_LENGTH 66
#define TEXT_13_LENGTH 11
#include "ev00bb/script/text.inc"

/* @hack type: e102_hold_stand_action declared as an array, which script.c
 * passes by its name. SHC then keeps the addresses of ev00bb_frog_1, _2 and _3
 * in R11, R9 and R10 (0x0cb8043c to 0x0cb8044c) and 0 in R14 for EVENT_END's
 * clears (0x0cb81a42). as one NJS_ACTION passed as &e102_hold_stand_action, the
 * three addresses take R9, R10 and R11, R14 takes event_beam_kill's address,
 * the code runs 8 bytes long, and 89 of ev00bb_script's 2293 words differ */
extern NJS_ACTION e102_hold_stand_action[];
extern NJS_ACTION e102_run_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION eggman_walk_action;
extern NJS_ACTION eggman_skid_action;
extern NJS_ACTION eggman_stand_action;
/* two more of Eggman's actions */
extern NJS_ACTION dat_8c59d224;
extern NJS_ACTION dat_8c5a0b78;

extern NJS_TEXLIST ev00bb_frog_texlist;
extern NJS_TEXLIST event_e103_texlist;
extern NJS_TEXLIST event_e104_texlist;
extern NJS_TEXLIST event_e105_texlist;
extern NJS_ACTION ev00bb_e102_action_0;
extern NJS_ACTION ev00bb_e102_action_1;
extern NJS_ACTION ev00bb_e103_action_0;
extern NJS_ACTION ev00bb_e103_action_1;
extern NJS_ACTION ev00bb_e104_action_0;
extern NJS_ACTION ev00bb_e104_action_1;
extern NJS_ACTION ev00bb_e105_action_0;
extern NJS_ACTION ev00bb_e105_action_1;
extern NJS_ACTION ev00bb_eggman_action_0;
extern NJS_ACTION ev00bb_eggman_action_1;
extern NJS_ACTION ev00bb_eggman_action_2;
extern NJS_ACTION ev00bb_eggman_action_3;
extern NJS_ACTION ev00bb_eggman_action_4;
extern NJS_ACTION ev00bb_frog_action_0;
extern NJS_OBJECT ev00bb_frog_3_object;
extern NJS_MOTION ev00bb_frog_1_motion;
extern NJS_MOTION ev00bb_frog_2_motion;
extern NJS_MOTION ev00bb_frog_3_motion;

struct stretch {
    Sint32 index; /* ev00bb_stretches[index] holds the stretch's task */
    /* the slot of the target, the task whose object the stretch scales */
    struct task **target_slot;
};

/* the stretch's task keeps these in its work's vars. its work's timer holds
 * the steps the stretch was given, and its scl what each scaling adds to
 * the object's scale */
struct stretch_vars {
    Uint32 scalings; /* the scalings made so far */
    NJS_OBJECT *object; /* the object of the target's action */
    struct stretch *stretch;
};

#define VARS(work) ((struct stretch_vars *)&(work)->vars)

/* STRETCH_START waits a step. STRETCH_RUN scales the object each step, the
 * last time when its count of scalings passes the timer. STRETCH_END kills
 * the stretch's task */
enum stretch_mode { STRETCH_START, STRETCH_RUN, STRETCH_END };

static struct task *ev00bb_white = NULL;
static struct task *ev00bb_black = NULL;
static struct task *ev00bb_e103 = NULL;
static struct task *ev00bb_e104 = NULL;
static struct task *ev00bb_e105 = NULL;
/* Froggy, whom E-102 holds, then the frogs of E-103, E-104 and E-105 */
static struct task *ev00bb_frog = NULL;
static struct task *ev00bb_frog_1 = NULL;
static struct task *ev00bb_frog_2 = NULL;
static struct task *ev00bb_frog_3 = NULL;
static struct task *ev00bb_e103_beam = NULL;
static struct task *ev00bb_e104_beam = NULL;
static struct task *ev00bb_e105_beam = NULL;
/* each running stretch's task at its index, or null. the script uses
 * indexes 0 to 2 */
static struct task *ev00bb_stretches[5] = {NULL};

static void ev00bb_stretch_init(struct task *task);
static void ev00bb_stretch_step(struct task *task);
static void ev00bb_stretch_draw(struct task *task);
static void ev00bb_stretch_destroy(struct task *task);

/* stretch the target, the task in *target_slot, which must be live and
 * playing an action. the stretch scales the action's object from the third
 * step of its own task, which ev00bb_stretches[index] holds. it scales the
 * object steps + 1 times, a step apart. each time adds (x - 1) / steps to
 * the x scale, and likewise for y and z. as the stretch ends, it kills the
 * target, sets the object's scale back to 1 and clears *target_slot and
 * ev00bb_stretches[index]. a null from task_create or heap_alloc is
 * dereferenced */
static void ev00bb_stretch(struct task **target_slot, Float x, Float y, Float z,
                           int steps, int index)
{
    struct task_work *work;
    struct stretch *stretch;
    struct task *target;
    struct event_work *event;
    struct event_motion *motion;

    ev00bb_stretches[index] = task_create(TASK_WORK, 5, ev00bb_stretch_init);
    stretch = heap_alloc(sizeof(struct stretch));
    work = ev00bb_stretches[index]->work;
    VARS(work)->stretch = stretch;
    /* @hack spelling: work read again, which keeps &ev00bb_stretches[index]
     * in R14 (0x0cb80012 to 0x0cb80024); read once, R8 and a stack slot hold
     * it, the code runs 2 bytes long and 79 of 81 words differ */
    work = ev00bb_stretches[index]->work;
    work->mode = STRETCH_START;
    VARS(work)->scalings = 0;
    work->scl.x = (x - 1.0F) / (Float)steps;
    work->scl.y = (y - 1.0F) / (Float)steps;
    work->scl.z = (z - 1.0F) / (Float)steps;
    TASK_TIMER(work) = steps;
    stretch->target_slot = target_slot;
    stretch->index = index;
    /* @hack spelling: the target's motion read through the locals target
     * and event, which spills x to @(8,R15) and z to @(12,R15) (0x0cb8001a,
     * 0x0cb80022); read as one expression, the two trade, the reads take
     * other registers and 10 of 81 words differ */
    target = *stretch->target_slot;
    event = target->work->event;
    motion = event->motions;
    motion->action.object->evalflags &= ~NJD_EVAL_UNIT_SCL;
    VARS(work)->object = motion->action.object;
}

/* end the stretch at ev00bb_stretches[index] now, if there is one.
 * destroying its task kills the target and sets the object's scale back,
 * as the stretch's own end does */
static void ev00bb_stretch_stop(int index)
{
    if (ev00bb_stretches[index] != NULL) {
        task_destroy(ev00bb_stretches[index]);
    }
}

static void ev00bb_stretch_init(struct task *task)
{
    task->step = ev00bb_stretch_step;
    task->draw = ev00bb_stretch_draw;
    task->destructor = ev00bb_stretch_destroy;
}

static void ev00bb_stretch_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    switch (work->mode) {
    case STRETCH_START:
        work->mode = STRETCH_RUN;
        break;
    case STRETCH_RUN:
        VARS(work)->scalings++;
        if (VARS(work)->scalings > TASK_TIMER(work)) {
            work->mode = STRETCH_END;
        }
        object = VARS(work)->object;
        object->scl[0] += work->scl.x;
        object->scl[1] += work->scl.y;
        object->scl[2] += work->scl.z;
        break;
    case STRETCH_END:
        if (task != NULL) {
            task_kill(task);
            /* @hack redundant: the parameter cleared, which nothing reads,
             * and which keeps task_kill a call that saves PR (STS.L
             * PR,@-R15 at 0x0cb800d4); without it, task_kill becomes a tail
             * jump, the code runs 4 bytes short and 50 of 52 words differ */
            task = NULL;
        }
        break;
    }
}

static void ev00bb_stretch_draw(struct task *task)
{
}

/* the stretch's destructor: set the object's scale back to 1 and kill the
 * target. it clears the target's slot and ev00bb_stretches[index], and
 * frees the struct stretch */
static void ev00bb_stretch_destroy(struct task *task)
{
    struct stretch *stretch = VARS(task->work)->stretch;
    /* @hack spelling: the target's motion read through the locals target
     * and event, which loads the target's work into R3 (MOV.L @(32,R4),R3
     * at 0x0cb80150); read as one expression, R4 takes it and 2 of 44 words
     * differ */
    struct task *target = *stretch->target_slot;
    struct event_work *event = target->work->event;
    struct event_motion *motion = event->motions;

    motion->action.object->evalflags |= NJD_EVAL_UNIT_SCL;
    motion->action.object->scl[0] = 1.0F;
    motion->action.object->scl[1] = 1.0F;
    motion->action.object->scl[2] = 1.0F;
    event_kill_task(stretch->target_slot);
    *stretch->target_slot = NULL;
    ev00bb_stretches[stretch->index] = NULL;
    if (stretch != NULL) {
        heap_free(stretch);
    }
}

void ev00bb_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00BB.mlt");
        event_sound_init(2);
        ev00bb_white = event_fade_create(1.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        stream_request_music(MUSIC_THEE102);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3c72);
        event_set_pos(event_get_player(0), -32.0F, -5.1F, -42.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_place_task(&ev00bb_e103, event_e103_create, -11.0F, 8.05F, -42.0F,
                         0, 0x8000, 0);
        event_place_task(&ev00bb_e104, event_e104_create, 33.0F, -5.1F, -42.0F,
                         0, 0x8000, 0);
        event_place_task(&ev00bb_e105, event_e105_create, 11.0F, -5.1F, -42.0F,
                         0, 0x8000, 0);
        event_create_player(6, eggman_step, 0.0F, 6.7F, -193.0F, 0, 0, 0);
        event_place_actor(&ev00bb_frog, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y + 10.5F,
                          event_get_player(0)->work->pos.z - 3.5F, 0, 0, 0);
        event_place_actor(&ev00bb_frog_1, -11.0F, 0.0F, -50.5F, 0, 0, 0);
        event_place_actor(&ev00bb_frog_2, 21.0F, -5.1F, -50.5F, 0, 0, 0);
        event_place_actor(&ev00bb_frog_3, -1.0F, -5.1F, -50.5F, 0, 0, 0);
        event_wait(2);
        event_set_flags(ev00bb_e103, EVENT_IGNORE_GROUND);
        event_set_flags(ev00bb_e104, EVENT_IGNORE_GROUND);
        event_set_flags(ev00bb_e105, EVENT_IGNORE_GROUND);
        event_set_flags(ev00bb_frog, EVENT_IGNORE_GROUND);
        event_set_flags(ev00bb_frog_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev00bb_frog_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev00bb_frog_3, EVENT_IGNORE_GROUND);
        event_set_pos(ev00bb_e103, -11.0F, 8.05F, -42.0F);
        event_set_ang(ev00bb_e103, 0, 0x8000, 0);
        event_set_pos(ev00bb_e104, 33.0F, 8.05F, -42.0F);
        event_set_ang(ev00bb_e104, 0, 0x8000, 0);
        event_set_pos(ev00bb_e105, 11.0F, 8.05F, -42.0F);
        event_set_ang(ev00bb_e105, 0, 0x8000, 0);
        event_set_pos(ev00bb_frog, -44.0F, -4.6F, -51.5F);
        event_set_ang(ev00bb_frog, 0, 0x4700, 0);
        event_set_pos(ev00bb_frog_1, -11.0F, 4.6F, -55.0F);
        event_set_ang(ev00bb_frog_1, 0, 0x4800, -0x0500);
        event_set_pos(ev00bb_frog_2, 33.0F, 4.6F, -55.0F);
        event_set_ang(ev00bb_frog_2, 0, 0x4800, -0x0500);
        event_set_pos(ev00bb_frog_3, 11.0F, 4.6F, -55.0F);
        event_set_ang(ev00bb_frog_3, 0, 0x4800, -0x0500);
        event_play_action(event_get_player(0), e102_hold_stand_action,
                          &e102_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00bb_frog, &ev00bb_frog_action_1,
                          &ev00bb_frog_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00bb_e103, &ev00bb_e103_action_0,
                          &event_e103_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_motion(ev00bb_frog_1, &ev00bb_frog_1_object,
                          &ev00bb_frog_1_motion, &ev00bb_frog_1_texlist, 1.6F,
                          EVENT_REPEAT, 0);
        event_play_action(ev00bb_e104, &ev00bb_e104_action_0,
                          &event_e104_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_motion(ev00bb_frog_2, &ev00bb_frog_2_object,
                          &ev00bb_frog_2_motion, &ev00bb_frog_2_texlist, 1.6F,
                          EVENT_REPEAT, 0);
        event_play_action(ev00bb_e105, &ev00bb_e105_action_0,
                          &event_e105_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_motion(ev00bb_frog_3, &ev00bb_frog_3_object,
                          &ev00bb_frog_3_motion, &ev00bb_frog_3_texlist, 1.6F,
                          EVENT_REPEAT, 0);
        event_play_action(event_get_player(6), &eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_sound_start(1, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(1, -120, 120);
        event_voice_play(1728);
        event_wait(1);
        event_voice_play(1729);
        event_wait(1);
        event_voice_play(1730);
        event_set_pos(event_get_player(0), 0.0F, 0.0F, 159.0F);
        event_set_pos(ev00bb_frog, -12.0F, -0.5F, 149.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe900, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 20.0F, 152.0F);
        event_wait(10);
        if (ev00bb_white != NULL) {
            event_fade_set_speed(ev00bb_white, -1.0F / 60.0F);
        }
        event_wait(50);
        event_play_action(event_get_player(0), e102_hold_stand_action,
                          &e102_texlist, 0.1F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0x0b00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.7F, 6.3F, 173.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 19.4F, 6.3F, 170.0F);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00bb_frog);
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          0.8F, EVENT_REPEAT, 0);
        event_move(event_get_player(0), 0.0F, 0.0F, 159.0F, 0.0F, -4.0F, 0.0F,
                   80);
        event_wait(50);
        ev00bb_black = event_fade_create(0.05F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(20);
        event_move_stop_all();
        event_clear_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -32.0F, -5.1F, -42.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_set_pos(ev00bb_frog, -44.0F, -4.6F, -51.5F);
        event_set_ang(ev00bb_frog, 0, 0x4700, 0);
        event_play_action(event_get_player(0), e102_hold_stand_action,
                          &e102_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00bb_frog, &ev00bb_frog_action_1,
                          &ev00bb_frog_texlist, 0.8F, EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x7300, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.62F, -3.57F, -72.9F);
        if (ev00bb_black != NULL) {
            event_fade_set_speed(ev00bb_black, -1.0F / 30.0F);
        }
        event_wait(30);
        event_sound_stop(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x7300, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.62F, -3.57F, -72.9F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -10.7F, -3.73F, -70.65F);
        ev00bb_e103_beam = event_beam_create(-6.0F, 23.0F, -42.0F, 0, 0, 0);
        ev00bb_e104_beam = event_beam_create(38.0F, 23.0F, -42.0F, 0, 0, 0);
        ev00bb_e105_beam = event_beam_create(16.0F, 23.0F, -42.0F, 0, 0, 0);
        event_wait(1);
        event_beam_set_shape(ev00bb_e103_beam, 0.15F, 0.0F, 0.15F, 0.0F, 0.0F,
                             0);
        event_beam_set_shape(ev00bb_e104_beam, 0.15F, 0.0F, 0.15F, 0.0F, 0.0F,
                             0);
        event_beam_set_shape(ev00bb_e105_beam, 0.15F, 0.0F, 0.15F, 0.0F, 0.0F,
                             0);
        event_wait(1);
        event_beam_set_shape(ev00bb_e103_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.08F,
                             0);
        event_beam_set_shape(ev00bb_e104_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.08F,
                             0);
        event_beam_set_shape(ev00bb_e105_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.08F,
                             0);
        event_wait(1);
        event_beam_set_shape(ev00bb_e103_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev00bb_e104_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev00bb_e105_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_wait(87);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff00, 0xed00, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -4.15F, 8.107F, -185.2F);
        event_camera_follow(event_get_player(6));
        event_run_to_speed(event_get_player(6), 0.0F, 3.5F, -60.0F, 0.6F, 0.0F);
        event_wait(70);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -10.7F, -3.73F, -70.65F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1100, 0x7300, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 10.79F, -3.73F, -70.65F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x1100, 0x6c00, 0x0100);
        event_wait(10);
        event_run_wait(event_get_player(6));
        event_clear_motions(event_get_player(6));
        event_wait(30);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, event_get_player(6),
                                  0.0F, 8.0F, 0.0F, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0xf600, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 9.82F, -2.75F, -24.67F);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 100, 0x31c7);
        event_wait(10);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &ev00bb_eggman_action_4,
                          &eggman_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(6), &ev00bb_eggman_action_4,
                          &eggman_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(6), &dat_8c5a0b78, &eggman_texlist,
                          1.0F, 0, 3);
        event_wait(20);
        event_voice_play(1734);
        event_message_show(TEXT_1);
        event_wait(20);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0xfc00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 26.4F, -1.75F, -23.5F);
        event_set_pos(event_get_player(6), 27.5F, -3.5F, -68.0F);
        event_set_ang(event_get_player(6), 0, -0x4000, 0);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &ev00bb_eggman_action_1,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_follow(event_get_player(6));
        event_run_to_speed(event_get_player(6), -20.0F, -3.5F, -68.0F, 0.3F,
                           0.5F);
        event_voice_play(1736);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_run_clear(event_get_player(6));
        event_turn_by(event_get_player(6), 0, 0x8000, 0);
        event_wait(15);
        event_camera_follow_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0xaf00, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -49.683F, 7.806F, -67.193F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0xaf00, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -47.0F, 7.44F, -66.0F);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &ev00bb_eggman_action_2,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1737);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0xf500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -27.677F, 9.01F, -24.65F);
        event_clear_motions(event_get_player(6));
        event_voice_play(1738);
        event_message_show(TEXT_4);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0300, 0xee00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -48.16F, 9.18F, -20.95F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 80, 0x3c72);
        event_play_action(event_get_player(6), &eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(6), -32.0F, 3.5F, -68.0F, 0.18F,
                           0.0F);
        event_run_wait(event_get_player(6));
        event_turn_by(event_get_player(6), 0, 0x8000, 0);
        event_voice_wait();
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_skid_action,
                          &eggman_texlist, 1.0F, 0, 0);
        event_voice_play(1739);
        event_message_show(TEXT_5);
        event_wait_motions(event_get_player(6));
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x29f5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0140, 0xe700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -48.2F, 2.55F, -27.45F);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(20);
        event_message_close();
        event_play_action(event_get_player(6), &ev00bb_eggman_action_0,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(8);
        event_voice_play(1741);
        event_message_show(TEXT_6);
        event_wait_motions(event_get_player(6));
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c5a0b78, &eggman_texlist,
                          1.0F, 0, 0);
        event_voice_play(1743);
        event_message_show(TEXT_7);
        event_wait_motions(event_get_player(6));
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa40, 0x5d00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -13.46F, 19.19F, -72.66F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x0340, 0x5f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -11.79F, 0.82F, -74.16F);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 12);
        event_voice_play(1745);
        event_message_show(TEXT_8);
        event_wait_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c59d224, &eggman_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(0), e102_hold_stand_action,
                          &e102_texlist, 3.0F, EVENT_REPEAT, 16);
        event_play_action(ev00bb_frog, &ev00bb_frog_action_1,
                          &ev00bb_frog_texlist, 3.0F, EVENT_REPEAT, 16);
        event_message_close();
        event_wait(60);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00bb_frog);
        event_play_action(ev00bb_frog, &ev00bb_frog_action_1,
                          &ev00bb_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), e102_hold_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0xf440, 0x7800, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 4.33F, 21.4F, -93.65F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 70, 0x471c);
        event_voice_play(1748);
        event_message_show(TEXT_9);
        event_wait(18);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &ev00bb_eggman_action_3,
                          &eggman_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(25);
        event_message_close();
        event_beam_set_mode(ev00bb_e103_beam, BEAM_OPEN);
        event_wait(1);
        event_beam_set_mode(ev00bb_e104_beam, BEAM_OPEN);
        event_wait(1);
        event_beam_set_mode(ev00bb_e105_beam, BEAM_OPEN);
        event_sound_start(0, 1223, 1800);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0240, 0x8200, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 36.11F, 6.76F, -68.72F);
        event_kill_task(&ev00bb_frog_2);
        event_kill_task(&ev00bb_frog_3);
        event_kill_task(&ev00bb_frog_1);
        event_set_pos(ev00bb_e103, -6.0F, 15.05F, -42.0F);
        event_play_action(ev00bb_e103, &ev00bb_e103_action_1,
                          &event_e103_texlist, 1.0F, EVENT_REPEAT, 8);
        event_set_pos(ev00bb_e104, 38.0F, 14.55F, -42.0F);
        event_play_action(ev00bb_e104, &ev00bb_e104_action_1,
                          &event_e104_texlist, 1.0F, EVENT_REPEAT, 8);
        event_set_pos(ev00bb_e105, 16.0F, 15.05F, -42.0F);
        event_play_action(ev00bb_e105, &ev00bb_e105_action_1,
                          &event_e105_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(50);
        event_set_pos(ev00bb_frog, -28.155F, 5.23F, -69.0F);
        event_set_ang(ev00bb_frog, 0, 0, 0);
        event_play_action(ev00bb_frog, &ev00bb_frog_action_0,
                          &ev00bb_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_turn_by(event_get_player(0), 0, 0x7600, 0);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -16.75F, 6.76F, -64.82F);
        ev00bb_stretch(&ev00bb_e104, 0.1F, 2.5F, 0.1F, 30, 0);
        event_wait(30);
        event_beam_set_mode(ev00bb_e104_beam, BEAM_CLOSE);
        event_wait(5);
        ev00bb_stretch(&ev00bb_e105, 0.1F, 2.5F, 0.1F, 30, 1);
        event_wait(30);
        event_beam_set_mode(ev00bb_e105_beam, BEAM_CLOSE);
        event_wait(20);
        event_sound_stop(0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0540, 0xa300, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -38.0F, 9.243F, -59.056F);
        event_wait(30);
        event_set_ang(ev00bb_e103, 0, 0x5000, 0);
        event_set_ang(ev00bb_e104, 0, -0x7000, 0);
        event_set_ang(ev00bb_e105, 0, -0x6500, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -36.5F, 9.5F, -57.76F);
        event_wait(10);
        ev00bb_stretch(&ev00bb_e103, 0.1F, 2.5F, 0.1F, 30, 2);
        event_wait(30);
        event_beam_set_mode(ev00bb_e103_beam, BEAM_CLOSE);
        event_wait(70);
        event_voice_play(1754);
        event_message_show(TEXT_10);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0c40, 0x5900, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 1.37F, -3.5F, -79.2F);
        event_play_action(event_get_player(0), &ev00bb_e102_action_1,
                          &e102_texlist, 1.0F, 0, 8);
        event_wait_motions(event_get_player(0));
        event_turn_by(event_get_player(0), 0, 0x8000, 0);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.8F, EVENT_REPEAT, 0);
        event_voice_wait();
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xfc41, 0x7700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -18.8F, 12.956F, -85.765F);
        event_wait(60);
        event_voice_play(1755);
        event_message_show(TEXT_11);
        event_play_action(event_get_player(0), &ev00bb_e102_action_0,
                          &e102_texlist, 1.0F, 0, 8);
        event_wait_motions(event_get_player(0));
        event_turn_by(event_get_player(0), 0, 0x4000, 0);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 0.8F, 0, 0);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.8F, EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 210, 0x0040, 0x8400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 210, -71.6F, 10.0F, 21.4F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 210, 0x1c72);
        event_wait(70);
        event_voice_play(1756);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_clear_motions(event_get_player(6));
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f40, 0x1a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -12.22F, -3.925F, -19.748F);
        event_kill_task(&ev00bb_frog);
        event_play_action(event_get_player(6), &ev00bb_eggman_action_0,
                          &eggman_texlist, 1.0F, 0, 8);
        event_voice_play(1757);
        event_message_show(TEXT_13);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f40, 0x1a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -12.22F, -3.925F, -19.748F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0640, 0x1600, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -3.538F, -1.83F, 5.25F);
        event_wait(30);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_sound_stop_all();
        event_reset_player(0);
        event_kill_player(6);
        ev00bb_stretch_stop(0);
        ev00bb_stretch_stop(1);
        ev00bb_stretch_stop(2);
        event_kill_task(&ev00bb_e103);
        event_kill_task(&ev00bb_e104);
        event_kill_task(&ev00bb_e105);
        event_kill_task(&ev00bb_frog);
        event_kill_task(&ev00bb_frog_1);
        event_kill_task(&ev00bb_frog_2);
        event_kill_task(&ev00bb_frog_3);
        /* @hack spelling: the kill and the clear as one statement. SHC then
         * sets R14 to 0 ahead of the first event_beam_kill (0x0cb81a42). as two
         * statements, SHC sets it after that call, and 5 of 2293 words
         * differ */
        event_beam_kill(ev00bb_e103_beam), ev00bb_e103_beam = NULL;
        event_beam_kill(ev00bb_e104_beam);
        ev00bb_e104_beam = NULL;
        event_beam_kill(ev00bb_e105_beam);
        ev00bb_e105_beam = NULL;
        EVENT_KILL(ev00bb_white);
        EVENT_KILL(ev00bb_black);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00bb_textures[] = {
    {"FROG", &ev00bb_frog_texlist},
    {"FROG1", &ev00bb_frog_1_texlist},
    {"FROG2", &ev00bb_frog_2_texlist},
    {"FROG3", &ev00bb_frog_3_texlist},
    {"CAPTUREBEAM", &event_beam_texlist},
    {"EV_E103_BODY", &event_e103_texlist},
    {"EV_E104_BODY", &event_e104_texlist},
    {"EV_E105_BODY", &event_e105_texlist},
    {"EGGMAN", &eggman_texlist},
    {NULL, NULL},
};
