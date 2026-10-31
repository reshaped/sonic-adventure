/* rockets: one rises out of its launcher, and a player who takes its grip
 * rides it toward a target, along or up, until the player lets go */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/helpers.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/rocket.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

#define SE_LAUNCHER_OPEN 705
#define SE_ROCKET_FIRE 706

/* the frames of its action a launcher plays each step */
#define LAUNCHER_PACE 0.05F

/* a new rocket rises for RISE_STEPS steps for each frame of its launcher's
 * action, to lift its grip GRIP_HEIGHT above where the rocket started. the
 * grip is node GRIP_NODE of the rocket's action */
#define RISE_STEPS 20.0F
#define GRIP_HEIGHT 8.0F
#define GRIP_NODE 3

/* how far along its launcher's x a rocket that flies up starts */
#define UP_OFFSET 4.0F

/* a player takes hold once its hands are within TAKE_RANGE of the grip,
 * and within TAKE_HEIGHT of its height. the pull onto the grip then takes
 * PULL_STEPS steps */
#define TAKE_RANGE 5.5F
#define TAKE_HEIGHT 2.0F
#define PULL_STEPS 6.0F

/* a flying rocket's speed grows by FLY_ACCEL each step until it passes
 * FLY_SPEED */
#define FLY_SPEED 12.0F
#define FLY_ACCEL 0.9F

/* the step of the flight from which the rider may let go, along and up */
#define ALONG_LET_GO 10
#define UP_LET_GO 15
/* how far above the target the rider of a rocket that flies up lets go */
#define UP_REACH 300.0F

/* once the rider has let go, a rocket that flies along blinks after
 * ALONG_BLINK steps and ends at ALONG_VANISH. one that flies up waits for
 * the rider to land after UP_VANISH steps */
#define ALONG_BLINK 30
#define ALONG_VANISH 60
#define UP_VANISH 30

/* bits of a launcher's state */
#define LAUNCHER_FIRED 0x4 /* its rocket has taken off */
#define LAUNCHER_GONE 0x8 /* it has left range, which ends its rocket */

/* a launcher's own layout of task_work. target is the placed scl */
struct launcher_work {
    Sint8 mode;
    /* the switch that opens ROCKET HS and VS: the placed ang.x's low four
     * bits */
    Uint8 id;
    Uint8 unk_02;
    Uint8 state;
    Sint16 flags;
    Sint16 timer;
    NJS_ACTION *action;
    NJS_OBJECT *world; /* its model, in the collision world */
    Float frame;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 target; /* where its rockets fly to */
    struct collision_work *collision;
    struct event_work *event;
};

/* what a launcher keeps in its task's extra */
struct launcher_extra {
    Float reach; /* how far across its rockets fly, squared */
    Angle climb; /* the turn about z that, with ang.y, aims at the target */
};

enum launcher_mode {
    LAUNCHER_INIT,
    LAUNCHER_READY, /* closed, and waits to open */
    LAUNCHER_OPEN, /* its action plays on, to its last frame */
    LAUNCHER_WAIT, /* open, and waits for its rocket to take off */
    LAUNCHER_CLOSE /* its action plays back */
};

/* what a rocket keeps in its task's motion */
struct rocket_motion {
    struct launcher_work *launcher; /* the work of the launcher that made it */
    NJS_VECTOR speed; /* as it rises, and as it pulls its rider on */
    /* where its grip is in the world, once it has risen. once it flies, x
     * is its speed */
    NJS_POINT3 grip;
    Uint8 unk_1c[24];
    Float reach; /* how far it flies across, squared */
    Float steps; /* the steps left to pull the rider on */
    Float count; /* steps left to rise, then its action's last frame */
};

/* where a rocket starts, in its task's force */
struct rocket_origin {
    Uint8 unk_00[16];
    NJS_POINT3 pos;
};

/* a rocket's jet, in its task's extra: lit once it flies, and its frame */
struct rocket_jet {
    Uint8 lit;
    Float frame;
};

/* a rocket's work mode, which changes along with its task's step */
enum rocket_mode {
    ROCKET_START,
    ROCKET_RISE,
    ROCKET_WAIT, /* waits for a player to take hold */
    ROCKET_PULL, /* pulls the player onto its grip */
    ROCKET_TAKE_OFF, /* its action plays as it takes off */
    ROCKET_FLY,
    ROCKET_VANISH, /* blinks out once the rider has let go */
    ROCKET_END,
    ROCKET_DROPPED
};

/* the rocket's shape: a capsule turned 270 degrees about z, to lie along
 * the rocket's x */
/* clang-format off */
static const struct collision_shape rocket_shape = {
    SHAPE_CAPSULE | SHAPE_TURN_YZX, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {2.0F, 0.0F, 0.0F}, 3.5F, 3.5F, 0.0F, {0, 0, NJM_DEG_ANG(270)}, 0, NULL};
/* clang-format on */

/* where each character's hands are, from its feet, as it takes hold, and
 * as it hangs on along or up */
static const NJS_POINT3 rocket_grip_hands[CHARACTER_COUNT] = {
#include "object/rocket/rocket_grip_hands.inc"
};
static const NJS_POINT3 rocket_along_hands[CHARACTER_COUNT] = {
#include "object/rocket/rocket_along_hands.inc"
};
static const NJS_POINT3 rocket_up_hands[CHARACTER_COUNT] = {
#include "object/rocket/rocket_up_hands.inc"
};

/* the jet, EFF_REGULAR's pan_jetb01 to 03 */
static NJS_TEXLIST rocket_jet_texlist = {
#include "object/rocket/rocket_jet_texlist.inc"
};
static NJS_TEXANIM rocket_jet_frames[3] = {
#include "object/rocket/rocket_jet_frames.inc"
};
static NJS_SPRITE rocket_jet_sprite = {
#include "object/rocket/rocket_jet_sprite.inc"
};

/* its smoke, EFF_REGULAR's roketthrust01 to 08 */
NJS_TEXLIST rocket_thrust_texlist = {
#include "object/rocket/rocket_thrust_texlist.inc"
};
static NJS_TEXANIM rocket_thrust_frames[8] = {
#include "object/rocket/rocket_thrust_frames.inc"
};
static struct particle_anim rocket_thrust_anim = {
    &rocket_thrust_texlist, rocket_thrust_frames, 8,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};
struct particle_pool_spec rocket_thrust_pool = {&rocket_thrust_anim, 0, NULL,
                                                NULL};

/* how a puff of smoke grows and moves: dust_step reads its growth, rate,
 * drag and lift */
static struct dust_params rocket_thrust_params = {
    1.0F,
    0.4F,
    0.19F,
    0.06F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {5.0F, 0.3F, 0.3F, 1.0F},
};

struct rocket_work *rockets[ROCKET_COUNT];

static void rocket_launcher_init_h(struct task *task);
static void rocket_launcher_init_v(struct task *task);
static void rocket_launcher_open_h(struct task *task);
static void rocket_launcher_open_v(struct task *task);
static void rocket_launcher_opened(struct task *task);
static void rocket_launcher_close(struct task *task);
static void rocket_launcher_draw(struct task *task);
static void rocket_launcher_destroy(struct task *task);
static void rocket_step(struct task *task);
static void rocket_end_step(struct task *task);
static void rocket_land_step(struct task *task);
static void rocket_drop_step(struct task *task);
static void rocket_start(struct task *task);
static void rocket_rise(struct rocket_work *work, struct rocket_motion *motion);
static void rocket_fly_h(struct task *task);
static void rocket_fly_v(struct task *task);
static Bool rocket_take(struct task *task, Uint8 player);
static Bool rocket_pull(struct task *task);
static Bool rocket_take_off(struct task *task);
static void rocket_move(struct task *task);
static void rocket_get_grip(struct rocket_work *work, NJS_ACTION *action,
                            Float frame, NJS_POINT3 *out);
static void rocket_draw(struct task *task);
static void rocket_destroy(struct task *task);
static void rocket_thrust_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                 Float scale);
static void rocket_jet_draw(struct task *task);
static void rocket_thrust_emit(struct task *task);

/* leave the launcher closed, at frame 0, to wait for its switch */
static void rocket_launcher_rest(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    work->mode = LAUNCHER_READY;
    work->frame = 0.0F;
}

/* ROCKET H's step. the launcher opens with a rocket in it, closes once the
 * rocket takes off, and opens again with the next. its rockets fly along,
 * toward its target */
void rocket_h_step(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(17, 19), "Target Position");
    }
    if (placement_try_leave(task)) {
        work->state |= LAUNCHER_GONE;
        return;
    }
    switch (work->mode) {
    case LAUNCHER_INIT:
        rocket_launcher_init_h(task);
        rocket_launcher_open_h(task);
        break;
    case LAUNCHER_READY:
        if (work->state & LAUNCHER_FIRED) {
            rocket_launcher_open_h(task);
        }
        break;
    case LAUNCHER_OPEN:
        /* @hack spelling: the frame stepped in the test, which compares the
         * FR3 it stores (FCMP/GT FR3,FR2 at 0x8c06680c); stepped apart, SHC
         * loads the frame back, task and work trade R14 and R13, and 63 of
         * 107 words differ */
        if ((work->frame += LAUNCHER_PACE) >=
            (Float)work->action->motion->nbFrame - 1.0F) {
            rocket_launcher_opened(task);
        }
        break;
    case LAUNCHER_WAIT:
        if (work->state & LAUNCHER_FIRED) {
            rocket_launcher_close(task);
        }
        break;
    case LAUNCHER_CLOSE:
        /* @hack spelling: the frame stepped in the test, which stores it in the
         * branch's delay slot (FMOV.S FR3,@(R0,R13) at 0x8c066834); stepped
         * apart, task and work trade R14 and R13 and 24 of 107 words
         * differ */
        if ((work->frame -= LAUNCHER_PACE) < 0.0F) {
            rocket_launcher_open_h(task);
        }
        break;
    }
    object_world_test((struct task_work *)work, work->world);
    object_step_draw(task);
}

/* ROCKET HS's step: as ROCKET H's, but the launcher opens only while its
 * switch is on, and turns the switch off once it has closed */
void rocket_hs_step(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(17, 16), "<- ID Number");
        njPrintC(NJM_LOCATION(17, 19), "Target Position");
    }
    if (placement_try_leave(task)) {
        work->state |= LAUNCHER_GONE;
        return;
    }
    switch (work->mode) {
    case LAUNCHER_INIT:
        rocket_launcher_init_h(task);
        break;
    case LAUNCHER_READY:
        if (switch_is_on(work->id)) {
            rocket_launcher_open_h(task);
        }
        break;
    case LAUNCHER_OPEN:
        /* @hack spelling: the frame stepped in the test, as in rocket_h_step
         * (FCMP/GT FR3,FR2 at 0x8c066934); stepped apart, SHC loads the
         * frame back, the function runs 4 bytes long and 59 of 121 words
         * differ */
        if ((work->frame += LAUNCHER_PACE) >=
            (Float)work->action->motion->nbFrame - 1.0F) {
            rocket_launcher_opened(task);
        }
        break;
    case LAUNCHER_WAIT:
        if (work->state & LAUNCHER_FIRED) {
            rocket_launcher_close(task);
        }
        break;
    case LAUNCHER_CLOSE:
        work->frame -= LAUNCHER_PACE;
        if (work->frame < 0.0F) {
            switch_set(work->id, FALSE);
            rocket_launcher_rest(task);
        }
        break;
    }
    object_world_test((struct task_work *)work, work->world);
    object_step_draw(task);
}

/* ROCKET V's step: as ROCKET H's, but its rockets fly up */
void rocket_v_step(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(17, 19), "Target Position");
    }
    if (placement_try_leave(task)) {
        work->state |= LAUNCHER_GONE;
        return;
    }
    switch (work->mode) {
    case LAUNCHER_INIT:
        rocket_launcher_init_v(task);
        rocket_launcher_open_v(task);
        break;
    case LAUNCHER_READY:
        if (work->state & LAUNCHER_FIRED) {
            rocket_launcher_open_v(task);
        }
        break;
    case LAUNCHER_OPEN:
        /* @hack spelling: the frame stepped in the test, as in rocket_h_step
         * (FCMP/GT FR3,FR2 at 0x8c066a6c); stepped apart, 63 of 107 words
         * differ */
        if ((work->frame += LAUNCHER_PACE) >=
            (Float)work->action->motion->nbFrame - 1.0F) {
            rocket_launcher_opened(task);
        }
        break;
    case LAUNCHER_WAIT:
        if (work->state & LAUNCHER_FIRED) {
            rocket_launcher_close(task);
        }
        break;
    case LAUNCHER_CLOSE:
        /* @hack spelling: the frame stepped in the test, as in rocket_h_step
         * (FMOV.S FR3,@(R0,R13) at 0x8c066a94); stepped apart, 24 of 107
         * words differ */
        if ((work->frame -= LAUNCHER_PACE) < 0.0F) {
            rocket_launcher_open_v(task);
        }
        break;
    }
    object_world_test((struct task_work *)work, work->world);
    object_step_draw(task);
}

/* ROCKET VS's step: as ROCKET HS's, but its rockets fly up */
void rocket_vs_step(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(17, 16), "<- ID Number");
        njPrintC(NJM_LOCATION(17, 19), "Target Position");
    }
    if (placement_try_leave(task)) {
        work->state |= LAUNCHER_GONE;
        return;
    }
    switch (work->mode) {
    case LAUNCHER_INIT:
        rocket_launcher_init_v(task);
        break;
    case LAUNCHER_READY:
        if (switch_is_on(work->id)) {
            rocket_launcher_open_v(task);
        }
        break;
    case LAUNCHER_OPEN:
        /* @hack spelling: the frame stepped in the test, as in rocket_hs_step
         * (FCMP/GT FR3,FR2 at 0x8c066b94); stepped apart, 59 of 121 words
         * differ */
        if ((work->frame += LAUNCHER_PACE) >=
            (Float)work->action->motion->nbFrame - 1.0F) {
            rocket_launcher_opened(task);
        }
        break;
    case LAUNCHER_WAIT:
        if (work->state & LAUNCHER_FIRED) {
            rocket_launcher_close(task);
        }
        break;
    case LAUNCHER_CLOSE:
        work->frame -= LAUNCHER_PACE;
        if (work->frame < 0.0F) {
            switch_set(work->id, FALSE);
            rocket_launcher_rest(task);
        }
        break;
    }
    object_world_test((struct task_work *)work, work->world);
    object_step_draw(task);
}

void rocket_reset(void)
{
    struct rocket_work **slot = rockets;
    int i;

    for (i = 0; i < ROCKET_COUNT; i++) {
        *slot++ = NULL;
    }
}

/* put the rocket in the first free slot of rockets. TRUE when there is
 * one, else FALSE, and the rocket's slot stays as it was */
static Bool rocket_enter(struct rocket_work *work)
{
    struct rocket_work **slot = rockets;
    int i;

    for (i = 0; i < ROCKET_COUNT; i++) {
        if (*slot == NULL) {
            *slot = work;
            work->slot = i;
            return TRUE;
        }
        slot++;
    }
    return FALSE;
}

/* set up a launcher of ROCKET H or HS, facing its target. extra keeps how
 * far across the target is, squared */
static void rocket_launcher_init_h(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;
    struct launcher_extra *extra = (struct launcher_extra *)task->extra;
    NJS_OBJECT *world;

    work->action = &rocket_launcher_action;
    task->draw = rocket_launcher_draw;
    task->destructor = rocket_launcher_destroy;
    work->mode = LAUNCHER_READY;
    work->id = work->ang.x & 0xf;
    work->ang.x = 0;
    scratch_vector.x = work->target.x - work->pos.x;
    scratch_vector.y = work->target.y - work->pos.y;
    scratch_vector.z = work->target.z - work->pos.z;
    vector_aim_x(&scratch_vector, &work->ang.y, &extra->climb);
    extra->reach = (scratch_vector.x * scratch_vector.x) +
                   (scratch_vector.z * scratch_vector.z);
    /* @hack spelling: world set in the test, which tests R0 and copies it in
     * the branch's delay slot (TST R0,R0 at 0x8c066cf8); set apart, SHC
     * tests the copy and 3 of 80 words differ */
    if ((world = object_add_world_model(task, rocket_launcher_object->model)) !=
        NULL) {
        work->world = world;
        object_world_follow((struct task_work *)work, world);
    }
}

/* set up a launcher of ROCKET V or VS */
static void rocket_launcher_init_v(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;
    NJS_OBJECT *world;

    work->action = &rocket_launcher_action;
    task->draw = rocket_launcher_draw;
    task->destructor = rocket_launcher_destroy;
    work->mode = LAUNCHER_READY;
    work->id = work->ang.x & 0xf;
    work->ang.x = 0;
    /* @hack spelling: world set in the test, which keeps it in R13 (MOV R0,R13
     * at 0x8c066d4a); set apart, it stays in R5, the function runs short
     * and 33 of 36 words differ */
    if ((world = object_add_world_model(task, rocket_launcher_object->model)) !=
        NULL) {
        work->world = world;
        object_world_follow((struct task_work *)work, world);
    }
}

/* open the launcher and put a new rocket in it, facing the target. in the
 * set editor the launcher opens empty. the rocket rises for RISE_STEPS steps
 * for each frame of the launcher's action. its speed would lift its grip
 * GRIP_HEIGHT above its start in that time, but it moves on all but the
 * last step */
static void rocket_launcher_open_h(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;
    struct launcher_extra *extra = (struct launcher_extra *)task->extra;
    struct task *rocket;
    struct rocket_work *rocket_work;
    struct rocket_motion *motion;
    Float rise;

    work->mode = LAUNCHER_OPEN;
    work->frame = 0.0F;
    work->state &= ~LAUNCHER_FIRED;
    if (placement_editing(task)) {
        return;
    }
    /* @hack spelling: rocket set in the test, which tests R0 and copies it in
     * the branch's delay slot (TST R0,R0 at 0x8c066df6); set apart, SHC
     * tests the copy and 3 of 123 words differ */
    if ((rocket = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA,
                              3, rocket_step)) == NULL) {
        return;
    }
    rocket_work = (struct rocket_work *)rocket->work;
    motion = (struct rocket_motion *)rocket->motion;
    /* @hack redundant: mode stored again, as the image stores it twice
     * (MOV.B at 0x8c066dd8 and 0x8c066e06); stored once, 93 of 123 words
     * differ */
    work->mode = LAUNCHER_OPEN;
    rocket->parent = task;
    rocket_work->action = &rocket_h_action;
    rocket_work->pos.x = work->pos.x;
    rocket_work->pos.y = work->pos.y;
    rocket_work->pos.z = work->pos.z;
    rocket_work->target.x = work->target.x;
    rocket_work->target.y = work->target.y;
    rocket_work->target.z = work->target.z;
    rocket_work->ang.y = work->ang.y;
    rocket_work->ang.z = extra->climb;
    motion->launcher = work;
    motion->count = (Float)work->action->motion->nbFrame * RISE_STEPS;
    motion->reach = extra->reach;
    rocket_get_grip(rocket_work, rocket_work->action, rocket_work->frame,
                    &scratch_vector);
    /* @hack spelling: the rise through a local, which works it out in FR4
     * and loads the count into FR3 (FDIV FR3,FR4 at 0x8c066e7c); as one
     * expression, 36 of 123 words differ */
    rise = GRIP_HEIGHT - scratch_vector.y;
    motion->speed.y = rise / motion->count;
    rocket_enter(rocket_work);
    sound_play_at(SE_LAUNCHER_OPEN, 0, 0, 0, work->pos.x, work->pos.y,
                  work->pos.z);
}

/* as rocket_launcher_open_h, for a rocket that flies up. it starts
 * UP_OFFSET along the launcher's x, faces back along the launcher, stands
 * on its tail, and takes no slot in rockets */
static void rocket_launcher_open_v(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;
    struct task *rocket;
    struct rocket_work *rocket_work;
    struct rocket_motion *motion;
    Float rise;

    work->mode = LAUNCHER_OPEN;
    work->frame = 0.0F;
    work->state &= ~LAUNCHER_FIRED;
    if (placement_editing(task)) {
        return;
    }
    /* @hack spelling: rocket set in the test, as in rocket_launcher_open_h
     * (TST R0,R0 at 0x8c066f3a); set apart, 3 of 167 words differ */
    if ((rocket = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA,
                              3, rocket_step)) == NULL) {
        return;
    }
    rocket_work = (struct rocket_work *)rocket->work;
    motion = (struct rocket_motion *)rocket->motion;
    /* @hack redundant: mode stored again, as in rocket_launcher_open_h
     * (MOV.B at 0x8c066f18 and 0x8c066f4a); stored once, 123 of 167 words
     * differ */
    work->mode = LAUNCHER_OPEN;
    rocket->parent = task;
    rocket_work->state |= ROCKET_UP;
    rocket_work->action = &rocket_v_action;
    rocket_work->frame = 2.0F;
    scratch_vector.x = UP_OFFSET;
    scratch_vector.y = 0.0F;
    scratch_vector.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.y);
    njCalcVector(NULL, &scratch_vector, &scratch_vector2);
    njPopMatrix(1);
    rocket_work->pos.x = work->pos.x + scratch_vector2.x;
    rocket_work->pos.y = work->pos.y;
    rocket_work->pos.z = work->pos.z + scratch_vector2.z;
    rocket_work->target.x = work->target.x;
    rocket_work->target.y = work->target.y;
    rocket_work->target.z = work->target.z;
    rocket_work->ang.y = work->ang.y + NJM_DEG_ANG(180);
    rocket_work->ang.z = NJM_DEG_ANG(90);
    motion->launcher = work;
    motion->count = (Float)work->action->motion->nbFrame * RISE_STEPS;
    rocket_get_grip(rocket_work, rocket_work->action, rocket_work->frame,
                    &scratch_vector);
    /* @hack spelling: the rise through a local, as in
     * rocket_launcher_open_h (FDIV FR3,FR4 at 0x8c06701c); as one
     * expression, 23 of 167 words differ */
    rise = GRIP_HEIGHT - scratch_vector.y;
    motion->speed.y = rise / motion->count;
    sound_play_at(SE_LAUNCHER_OPEN, 0, 0, 0, work->pos.x, work->pos.y,
                  work->pos.z);
}

/* hold the launcher open, at its action's last frame */
static void rocket_launcher_opened(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    work->mode = LAUNCHER_WAIT;
    work->frame = (Float)work->action->motion->nbFrame - 1.0F;
}

/* close the launcher, playing its action back from the last frame, once
 * its rocket has taken off */
static void rocket_launcher_close(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    work->mode = LAUNCHER_CLOSE;
    work->frame = (Float)work->action->motion->nbFrame - 1.0F;
    work->state &= ~LAUNCHER_FIRED;
}

static void rocket_launcher_draw(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_action(work->action, work->frame);
        njPopMatrix(1);
    }
}

static void rocket_launcher_destroy(struct task *task)
{
    struct launcher_work *work = (struct launcher_work *)task->work;

    world_remove(task, work->world);
    world_object_free(work->world);
}

/* a rocket's step until a rider takes hold: it starts, rises, then waits
 * for player 0 or 1 to take hold. it ends once its launcher has left range,
 * or once its parent task's work is no longer its launcher's */
static void rocket_step(struct task *task)
{
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct launcher_work *launcher = (struct launcher_work *)task->parent->work;
    struct rocket_work *work = (struct rocket_work *)task->work;

    if (launcher != motion->launcher || (launcher->state & LAUNCHER_GONE)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case ROCKET_START:
        rocket_start(task);
        break;
    case ROCKET_RISE:
        rocket_rise(work, motion);
        break;
    case ROCKET_WAIT:
        if (!rocket_take(task, 0)) {
            rocket_take(task, 1);
        }
        break;
    /* @hack redundant: the default's break, which loads collision_register's
     * address into R2 and object_step_draw's into R3 (0x8c067220 and
     * 0x8c067228); without it, the two trade and 4 of 55 words differ */
    default:
        break;
    }
    collision_register((struct task_work *)work);
    object_step_draw(task);
}

static void rocket_pull_step(struct task *task)
{
    rocket_pull(task);
    object_step_draw(task);
}

static void rocket_take_off_step(struct task *task)
{
    rocket_take_off(task);
    object_step_draw(task);
}

/* fly, with smoke behind, until the rider lets go, or is made to */
static void rocket_fly_step(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct task_work *rider;

    if (work->state & ROCKET_RELEASE) {
        rider = work->rider->work;
        task->step = rocket_drop_step;
        work->mode = ROCKET_DROPPED;
        player_set_mode(PLAYER_VARS(rider)->player, PLAYER_MODE_RESET);
        collision_shapes_on(rider);
        return;
    }
    if (!(work->state & ROCKET_UP)) {
        rocket_fly_h(task);
    } else {
        rocket_fly_v(task);
    }
    object_step_draw(task);
    rocket_thrust_emit(task);
    rocket_move(task);
}

/* fly on once the rider of a rocket that flies along has let go. after
 * ALONG_BLINK steps the rocket blinks, and at ALONG_VANISH it ends */
static void rocket_vanish_h_step(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;

    work->timer++;
    if (work->timer == ALONG_VANISH) {
        task->step = rocket_end_step;
        work->mode = ROCKET_END;
    }
    /* @hack redundant: the else after the return, which loads
     * object_step_draw's address into R2 (MOV.L at 0x8c0672d8); without it, R3,
     * and 2 of 33 words differ */
    if (work->timer > ALONG_BLINK && !(work->timer & 3)) {
        return;
    } else {
        object_step_draw(task);
        rocket_move(task);
    }
}

/* fly on, blinking, once the rider of a rocket that flies up has let go.
 * after UP_VANISH steps, wait for the rider to land */
static void rocket_vanish_v_step(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;

    work->timer++;
    if (work->timer == UP_VANISH) {
        task->step = rocket_land_step;
        work->mode = ROCKET_END;
    }
    /* @hack redundant: the else after the return, which loads
     * object_step_draw's address into R3 (MOV.L at 0x8c06734c); without it, R2,
     * and 2 of 29 words differ */
    if (!(work->timer & 3)) {
        return;
    } else {
        object_step_draw(task);
        rocket_move(task);
    }
}

static void rocket_end_step(struct task *task)
{
    task_kill(task);
}

/* the step of a rocket that flew up, once it has vanished. when the rider
 * lands, or falls below the target, the rocket ends, and gives the camera
 * back if the rider is player 0 */
static void rocket_land_step(struct task *task)
{
    /* @hack redundant: register on work, which puts work in R4, task's
     * register, and rider in R5 (MOV.L @(32,R4),R4 at 0x8c06736c); without
     * it, SHC copies task to R5 for work, the function runs 2 bytes long
     * and 32 of 34 words differ */
    register struct rocket_work *work = (struct rocket_work *)task->work;
    struct task_work *rider = work->rider->work;

    if (work->state & ROCKET_UP) {
        if ((rider->flags & PLAYER_ON_GROUND) ||
            rider->pos.y < work->target.y) {
            if (PLAYER_VARS(rider)->player == 0) {
                camera_restore();
            }
            task_kill(task);
        }
    }
}

static void rocket_drop_step(struct task *task)
{
    task_kill(task);
}

/* note where the rocket starts, give it its shape, and rise */
static void rocket_start(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_origin *origin = (struct rocket_origin *)task->force;

    work->mode = ROCKET_RISE;
    task->draw = rocket_draw;
    task->destructor = rocket_destroy;
    origin->pos.x = work->pos.x;
    origin->pos.y = work->pos.y;
    origin->pos.z = work->pos.z;
    collision_init(task, &rocket_shape, COLLISION_OBJECT, 1, 1);
}

/* rise until the count runs out, then note where the grip is, and wait
 * for a rider */
static void rocket_rise(struct rocket_work *work, struct rocket_motion *motion)
{
    motion->count -= 1.0F;
    if (motion->count > 0.0F) {
        work->pos.y += motion->speed.y;
    } else {
        work->mode = ROCKET_WAIT;
        rocket_get_grip(work, work->action, work->frame, &scratch_vector);
        motion->grip.x = work->pos.x + scratch_vector.x;
        motion->grip.y = work->pos.y + scratch_vector.y;
        motion->grip.z = work->pos.z + scratch_vector.z;
    }
}

/* from the flight's ALONG_LET_GO step, jump or action on pad 0 makes the
 * rider let go. once the rocket is farther across from its start than its
 * reach, the rider lets go anyway */
static void rocket_fly_h(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct rocket_origin *origin = (struct rocket_origin *)task->force;
    struct task_work *rider = work->rider->work;
    Float dx;
    Float dz;

    work->timer++;
    if (work->timer == ALONG_LET_GO) {
        work->state |= ROCKET_LET_GO;
    }
    dx = work->pos.x - origin->pos.x;
    dz = work->pos.z - origin->pos.z;
    if (((per_info[0]->press & (pad_jump_buttons | pad_action_buttons)) &&
         (work->state & ROCKET_LET_GO)) ||
        (dx * dx) + (dz * dz) > motion->reach) {
        task->step = rocket_vanish_h_step;
        work->mode = ROCKET_VANISH;
        work->timer = 0;
        player_set_mode(PLAYER_VARS(rider)->player, PLAYER_MODE_RESET);
        collision_shapes_on(rider);
    }
}

/* from the flight's UP_LET_GO step, jump or action on pad 0 makes the
 * rider let go. at that step, a rider that is player 0 has its camera look
 * at the target. UP_REACH above the target, the rider lets go anyway. a
 * rider that is player 0 gets its camera back as it lets go. playing Sonic,
 * a rider at or above the target gets it back from rocket_land_step
 * instead */
static void rocket_fly_v(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct task_work *rider = work->rider->work;

    work->timer++;
    if (work->timer == UP_LET_GO) {
        work->state |= ROCKET_LET_GO;
        if (PLAYER_VARS(rider)->player == 0) {
            camera_override_view.target.x = work->target.x;
            camera_override_view.target.y = work->target.y;
            camera_override_view.target.z = work->target.z;
            camera_override(camera_step_view_target, 0, CAMERA_LOOK_AT);
        }
    }
    if (((per_info[0]->press & (pad_jump_buttons | pad_action_buttons)) &&
         (work->state & ROCKET_LET_GO)) ||
        work->pos.y > work->target.y + UP_REACH) {
        task->step = rocket_vanish_v_step;
        work->mode = ROCKET_VANISH;
        work->timer = 0;
        player_set_mode(PLAYER_VARS(rider)->player, PLAYER_MODE_RESET);
        if ((get_character() || rider->pos.y < work->target.y) &&
            PLAYER_VARS(rider)->player == 0) {
            camera_restore();
        }
        collision_shapes_on(rider);
    }
}

/* take player as the rider, if it stands on the ground with its hands
 * within TAKE_RANGE of the grip and within TAKE_HEIGHT of its height. the
 * rider turns to face along the rocket, and is pulled onto the grip in
 * PULL_STEPS steps. TRUE when it takes hold, else FALSE */
static Bool rocket_take(struct task *task, Uint8 player)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct task *rider_task;
    struct task_work *rider;
    const NJS_POINT3 *hands;
    NJS_VECTOR offset;
    NJS_POINT3 grip;
    Float distance;
    Float divisor;

    /* @hack spelling: rider_task set in the test, which keeps it in R8 and
     * task in R9 (MOV R0,R8 at 0x8c067644); set apart, the two trade and 9
     * of 209 words differ */
    if ((rider_task = player_get_task(player)) == NULL) {
        return FALSE;
    }
    rider = rider_task->work;
    if (!(rider->flags & PLAYER_ON_GROUND)) {
        return FALSE;
    }
    hands = &rocket_grip_hands[PLAYER_VARS(rider)->character];
    scratch_vector.x = hands->x;
    scratch_vector.y = hands->y;
    scratch_vector.z = hands->z;
    player_turn_to_world(rider, &scratch_vector);
    grip.x = rider->pos.x + scratch_vector.x;
    grip.y = rider->pos.y + scratch_vector.y;
    grip.z = rider->pos.z + scratch_vector.z;
    offset.x = grip.x - motion->grip.x;
    offset.y = grip.y - motion->grip.y;
    offset.z = grip.z - motion->grip.z;
    if (_builtin_fabsf(offset.y) > TAKE_HEIGHT) {
        return FALSE;
    }
    distance =
        (offset.x * offset.x) + (offset.y * offset.y) + (offset.z * offset.z);
    if (distance > TAKE_RANGE * TAKE_RANGE) {
        return FALSE;
    }
    work->mode = ROCKET_PULL;
    task->step = rocket_pull_step;
    work->rider = rider_task;
    player_set_mode(player, PLAYER_MODE_ROCKET);
    /* @hack spelling: z stored in each arm, which keeps scratch_vector's
     * address in R14 and motion in R13 (MOV.L @(36,R9),R13 at 0x8c067638);
     * stored once after the if, the two trade and 28 of 209 words differ */
    if (!(work->state & ROCKET_UP)) {
        scratch_vector.x = 1.0F;
        scratch_vector.y = 0.0F;
        scratch_vector.z = 0.0F;
    } else {
        scratch_vector.x = 0.0F;
        scratch_vector.y = 1.0F;
        scratch_vector.z = 0.0F;
    }
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njCalcVector(NULL, &scratch_vector, &scratch_vector2);
    njPopMatrix(1);
    player_face_along(PLAYER_VARS(rider)->player, scratch_vector2.x,
                      scratch_vector2.y, scratch_vector2.z);
    /* each step of the pull, the rider closes an eighth of the gap across,
     * and the rocket an eighth of the gap up or down */
    /* @hack spelling: 8.0F through divisor, a local, which loads it ahead of
     * PULL_STEPS (FMOV.S @R0,FR4 at 0x8c067776); written in place,
     * PULL_STEPS comes first and 5 of 209 words differ */
    divisor = 8.0F;
    motion->steps = PULL_STEPS;
    motion->speed.x = (motion->grip.x - grip.x) / divisor;
    motion->speed.y = (grip.y - motion->grip.y) / divisor;
    motion->speed.z = (motion->grip.z - grip.z) / divisor;
    return TRUE;
}

/* the pull: the rider slides toward the grip, and the rocket up or down
 * toward the rider's hands. on the last step the rider is set at the grip
 * and left out of the collision checks, and the rocket takes off. TRUE on
 * that step, else FALSE */
static Bool rocket_pull(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct task_work *rider = work->rider->work;
    const NJS_POINT3 *hands;
    Float y;

    motion->steps -= 1.0F;
    if (motion->steps > 0.0F) {
        rider->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        rider->pos.z += motion->speed.z;
    } else {
        task->step = rocket_take_off_step;
        work->mode = ROCKET_TAKE_OFF;
        motion->count = (Float)work->action->motion->nbFrame - 1.0F;
        hands = &rocket_grip_hands[PLAYER_VARS(rider)->character];
        scratch_vector.x = hands->x;
        scratch_vector.y = hands->y;
        scratch_vector.z = hands->z;
        player_turn_to_world(rider, &scratch_vector);
        y = rider->pos.y + scratch_vector.y;
        work->pos.y += y - motion->grip.y;
        motion->grip.y = y;
        player_set_position(PLAYER_VARS(rider)->player,
                            motion->grip.x - scratch_vector.x,
                            motion->grip.y - scratch_vector.y,
                            motion->grip.z - scratch_vector.z);
        collision_shapes_off(rider);
        return TRUE;
    }
    return FALSE;
}

/* play the rocket's action as it takes off, the grip held where the
 * rider's hands are. at the action's last frame the rocket flies, its
 * launcher closes and its jet lights. TRUE then, else FALSE */
static Bool rocket_take_off(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct rocket_jet *jet = (struct rocket_jet *)task->extra;
    struct task *rider_task;
    struct task_work *rider;
    struct launcher_work *launcher;
    struct task *parent;

    /* @hack spelling: the rider's task through rider_task, which loads it
     * into R5 before its work into R11 (MOV.L @(16,R14),R5 at 0x8c067948);
     * read in one expression, R11 takes both and 2 of 106 words differ */
    rider_task = work->rider;
    rider = rider_task->work;
    work->frame += 0.2F;
    if (work->frame < motion->count) {
        rocket_get_grip(work, work->action, work->frame, &scratch_vector);
        work->pos.x = motion->grip.x - scratch_vector.x;
        work->pos.y = motion->grip.y - scratch_vector.y;
        work->pos.z = motion->grip.z - scratch_vector.z;
    } else {
        /* @hack spelling: the launcher's task through parent, which loads
         * it into R5 ahead of the mode's store (MOV.L @(8,R4),R5 at
         * 0x8c067992); read in one expression, it goes to R12 and 6 of 106
         * words differ */
        parent = task->parent;
        launcher = (struct launcher_work *)parent->work;
        work->mode = ROCKET_FLY;
        task->step = rocket_fly_step;
        if (!(work->state & ROCKET_UP)) {
            player_set_mode(PLAYER_VARS(rider)->player,
                            PLAYER_MODE_ROCKET_ALONG);
        } else {
            player_set_mode(PLAYER_VARS(rider)->player, PLAYER_MODE_ROCKET_UP);
        }
        if (launcher == motion->launcher) {
            launcher->state |= LAUNCHER_FIRED;
        }
        work->state |= ROCKET_FLYING;
        work->frame = motion->count;
        work->timer = 0;
        motion->grip.x = 0.0F;
        jet->lit = 1;
        sound_play(SE_ROCKET_FIRE, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* move along the rocket's heading, with the speed growing by FLY_ACCEL
 * each step until it passes FLY_SPEED. while the rocket flies, bring the
 * rider along by its hands */
static void rocket_move(struct task *task)
{
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct task *rider_task;
    struct task_work *rider;
    const NJS_POINT3 *hands;

    if (motion->grip.x < FLY_SPEED) {
        motion->grip.x += FLY_ACCEL;
    }
    njPushMatrix(&_unit_matrix_);
    scratch_vector.x = motion->grip.x;
    scratch_vector.y = 0.0F;
    scratch_vector.z = 0.0F;
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njCalcVector(NULL, &scratch_vector, &scratch_vector2);
    work->pos.x += scratch_vector2.x;
    work->pos.y += scratch_vector2.y;
    work->pos.z += scratch_vector2.z;
    if (work->mode == ROCKET_FLY) {
        /* @hack spelling: the rider's task through rider_task, which keeps
         * motion in R13 (MOV.L @(36,R4),R13 at 0x8c067a8e); read in one
         * expression, motion takes R12 and 30 of 165 words differ */
        rider_task = work->rider;
        rider = rider_task->work;
        if (rider->submode != PLAYER_MODE_RESET) {
            rocket_get_grip(work, work->action, work->frame, &scratch_vector);
            scratch_vector.x += work->pos.x;
            scratch_vector.y += work->pos.y;
            scratch_vector.z += work->pos.z;
            /* @hack spelling: the hands copied in each arm, which keeps
             * motion in R13 as well; copied once after the if, motion takes
             * R12 and 28 of 165 words differ */
            if (!(work->state & ROCKET_UP)) {
                hands = &rocket_along_hands[PLAYER_VARS(rider)->character];
                scratch_vector2.x = hands->x;
                scratch_vector2.y = hands->y;
                scratch_vector2.z = hands->z;
            } else {
                hands = &rocket_up_hands[PLAYER_VARS(rider)->character];
                scratch_vector2.x = hands->x;
                scratch_vector2.y = hands->y;
                scratch_vector2.z = hands->z;
            }
            player_turn_to_world(rider, &scratch_vector2);
            rider->pos.x = scratch_vector.x - scratch_vector2.x;
            rider->pos.y = scratch_vector.y - scratch_vector2.y;
            rider->pos.z = scratch_vector.z - scratch_vector2.z;
        }
    }
    njPopMatrix(1);
}

/* set out to the grip's offset from the rocket, turned with the rocket, as
 * its action stands at frame */
static void rocket_get_grip(struct rocket_work *work, NJS_ACTION *action,
                            Float frame, NJS_POINT3 *out)
{
    /* @hack spelling: scratch_matrix through matrix, a local, which keeps
     * its address in R14 across the calls (MOV.L at 0x8c067bd8); named at
     * each use, 55 of 57 words differ */
    NJS_MATRIX *matrix = &scratch_matrix;

    scratch_vector.x = 0.0F;
    scratch_vector.y = 0.0F;
    scratch_vector.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    pose_action(NULL, action, frame, GRIP_NODE, matrix);
    njCalcPoint(matrix, &scratch_vector, out);
    njPopMatrix(1);
}

static void rocket_draw(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, work->ang.z);
        draw_action(work->action, work->frame);
        njPopMatrix(1);
        rocket_jet_draw(task);
    }
}

static void rocket_destroy(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;

    task->parent = NULL;
    /* @bug logic: a rocket that flies up, or found rockets full, has
     * slot 0, and clears rockets[0] though another rocket may hold it
     * (MOV.L R6,@R3 at 0x8c067d14) */
    rockets[work->slot] = NULL;
}

/* make a puff of smoke from rocket_thrust_pool at pos, moving at speed,
 * its size scale times DUST_SIZE */
static void rocket_thrust_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                 Float scale)
{
    struct particle_pool *pool = rocket_thrust_pool.pool;
    struct particle *particle = particle_alloc(pool, dust_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        DUST(particle)->params = &rocket_thrust_params;
    }
}

/* draw the lit jet as four sprites at the rocket's tail, on a frame's
 * first step only. each call steps the jet's frame through 0, 1 and 2 */
static void rocket_jet_draw(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_jet *jet = (struct rocket_jet *)task->extra;
    Float frame;

    frame = jet->frame + 1.0F;
    if (frame >= 3.0F) {
        frame = 0.0F;
    }
    jet->frame = frame;
    if (jet->lit && !frame_step) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, work->ang.z);
        njTranslate(NULL, -8.0F, 2.4F, -2.4F);
        njDrawSprite3D(&rocket_jet_sprite, (Int)frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
        njTranslate(NULL, 0.0F, 0.0F, 4.8F);
        njDrawSprite3D(&rocket_jet_sprite, (Int)frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
        njTranslate(NULL, 0.0F, -4.8F, 0.0F);
        njDrawSprite3D(&rocket_jet_sprite, (Int)frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
        njTranslate(NULL, 0.0F, 0.0F, -4.8F);
        njDrawSprite3D(&rocket_jet_sprite, (Int)frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
        njPopMatrix(1);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
    }
}

/* while the jet is lit, on a frame's first step, make a puff of smoke
 * behind the rocket's tail, moving with the rocket */
static void rocket_thrust_emit(struct task *task)
{
    struct rocket_work *work = (struct rocket_work *)task->work;
    struct rocket_motion *motion = (struct rocket_motion *)task->motion;
    struct rocket_jet *jet = (struct rocket_jet *)task->extra;

    if (jet->lit && !frame_step) {
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, work->ang.z);
        scratch_point.x = -motion->grip.x - 3.0F;
        scratch_point.y = 0.0F;
        scratch_point.z = 0.0F;
        njCalcPoint(NULL, &scratch_point, &scratch_point2);
        vector_add(&work->pos, &scratch_point2, &scratch_point);
        scratch_vector.x = motion->grip.x;
        scratch_vector.y = 0.0F;
        scratch_vector.z = 0.0F;
        njCalcVector(NULL, &scratch_vector, &scratch_vector2);
        njPopMatrix(1);
        rocket_thrust_create(&scratch_point, &scratch_vector2, 0.8F);
    }
}
