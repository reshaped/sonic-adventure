/* the lever O LEVER: a stand with a handle on top. player 0 takes hold of the
 * handle and pushes it, and the deck's gravity tilts with the handle */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "event/play.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg06/lever.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT lever_handle_object;
extern NJS_OBJECT lever_object; /* the stand */

/* the square of the distance from the camera or player 0 within which the
 * lever stays */
#define LEVER_RANGE (1010.0F * 1010.0F)
/* the world tests the stand while a player is within LEVER_NEAR of it */
#define LEVER_NEAR 68.13577F
/* how far above the stand the handle sits */
#define HANDLE_HEIGHT 6.0F

/* how far a push tilts the handle about x or z. the handle turns TILT_SPEED
 * a step on the way */
#define TILT NJM_DEG_ANG(20)
#define TILT_SPEED NJM_DEG_ANG(1)
/* the rumble while the handle turns. TILT_SOUND_ID keeps it in one slot, and
 * each step leaves it TILT_SOUND_UPDATES + 1 updates to play */
#define SE_TILT 958
#define TILT_SOUND_ID 0x1041
#define TILT_SOUND_UPDATES 30

struct lever_vars {
    /* what lever_draw draws: the stand's world copy, or the handle's object */
    NJS_OBJECT *object;
    NJS_OBJECT *stand; /* the stand's object, which lever_add_world copies */
};

#define VARS(work) ((struct lever_vars *)&(work)->vars)

/* the stand's modes. in LEVER_STOP its step neither draws it nor steps the
 * handle, and no step here sets it */
enum lever_mode { LEVER_INIT = 0, LEVER_RUN = 1, LEVER_STOP = 3 };
/* the handle's modes: waiting for player 0 to hold it, then turning. no step
 * here sets HANDLE_STOP */
enum handle_mode { HANDLE_INIT, HANDLE_WAIT, HANDLE_TILT, HANDLE_STOP };

static struct collision_shape lever_handle_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     0,
     0,
     {0.0F, 10.7F, 0.0F},
     8.0F,
     4.3F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 10.7F, 0.0F},
     2.0F,
     4.3F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the angles the handle turns toward, about x and about z */
static Angle lever_tilt_x;
static Angle lever_tilt_z;

static void lever_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_model(VARS(work)->object->model);
    njPopMatrix(1);
}

/* take the stand's world copy out of the world, and return it to
 * world_object_alloc's pool. the handle's destructor is this too. neither call
 * finds the handle's own object, so for the handle neither changes anything */
static void lever_release(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

/* @hack redundant: register on task, which keeps it in R14 across
 * task_destroy_children (MOV R4,R14 at 0x0c90b22c); without it, task is kept
 * on the stack, 9 of 9 words differ and the function ends 2 bytes early */
static void lever_destroy(register struct task *task)
{
    task_destroy_children(task);
    lever_release(task);
}

/* put a copy of the stand's model in the world at the stand's position and
 * angles. the copy has no child or sibling */
static void lever_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    VARS(work)->object = world = world_object_alloc();
    world->evalflags =
        VARS(work)->stand->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = VARS(work)->stand->model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WORLD_LARGE, task, world);
}

static void lever_update_world(struct task_work *work)
{
    NJS_OBJECT *world = VARS(work)->object;

    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    if (player_in_range(&work->pos, LEVER_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* while player 0 holds the handle, a push aims the handle at TILT about x or
 * about z. it aims the other angle at 0. a push is the handle's motion->accel.
 * a push of 0 keeps the angle the handle has. the handle turns toward its aim
 * TILT_SPEED a step, and the deck's gravity tilts with it. it turns until it
 * gets there, or until player 0 lets go of the action buttons. while an event
 * has control, gravity points straight down instead */
static void lever_handle_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record;
    int side;

    switch (work->mode) {
    case HANDLE_INIT:
        work->mode = HANDLE_WAIT;
        work->pos.y = task->parent->work->pos.y + HANDLE_HEIGHT;
        VARS(work)->object = &lever_handle_object;
        collision_init(task, lever_handle_shapes, COLLISION_OBJECT, 1, 2);
        carry_set_kind(work, motion, CARRY_KIND_MEDIUM);
        task->destructor = lever_release;
        task->draw = lever_draw;
        lever_tilt_x = 0;
        lever_tilt_z = 0;
        break;
    case HANDLE_WAIT:
        record = player_get(0);
        if (record->held == task) {
            /* side is the bearing from the handle to player 0. within 45
             * degrees of +z or -z, a push along z aims the handle about x. that
             * aim takes the push's sign. elsewhere, a push along x aims it
             * about z. that aim takes the opposite sign */
            /* @hack spelling: the angle and its low half as two statements,
             * which loads TILT after the FTRC (MOV.W @(0x0c90b442),R4 at
             * 0x0c90b420); as one statement, SHC loads it ahead of the FTRC
             * and 25 of 252 words differ */
            side = (Angle)(atan2f(player_work[0]->pos.x - work->pos.x,
                                  player_work[0]->pos.z - work->pos.z) *
                           ANGLE_PER_RADIAN);
            side = (Uint16)side;
            /* @hack spelling: each way ends in a goto to tilt, and the ways
             * meet at 0x0c90b52c; as an else if chain, 76 of 252 words differ
             */
            if (side < NJM_DEG_ANG(45)) {
                if (motion->accel.z > 0.0F) {
                    lever_tilt_x = TILT;
                } else if (motion->accel.z < 0.0F) {
                    lever_tilt_x = -TILT;
                } else {
                    lever_tilt_x = work->ang.x;
                }
                lever_tilt_z = 0;
                goto tilt;
            }
            if (side < NJM_DEG_ANG(135)) {
                if (motion->accel.x > 0.0F) {
                    lever_tilt_z = -TILT;
                } else if (motion->accel.x < 0.0F) {
                    lever_tilt_z = TILT;
                } else {
                    lever_tilt_z = work->ang.z;
                }
                lever_tilt_x = 0;
                goto tilt;
            }
            if (side < NJM_DEG_ANG(225)) {
                if (motion->accel.z > 0.0F) {
                    lever_tilt_x = TILT;
                } else if (motion->accel.z < 0.0F) {
                    lever_tilt_x = -TILT;
                } else {
                    lever_tilt_x = work->ang.x;
                }
                lever_tilt_z = 0;
                goto tilt;
            }
            if (side < NJM_DEG_ANG(315)) {
                if (motion->accel.x > 0.0F) {
                    lever_tilt_z = -TILT;
                } else if (motion->accel.x < 0.0F) {
                    lever_tilt_z = TILT;
                } else {
                    lever_tilt_z = work->ang.z;
                }
                lever_tilt_x = 0;
                goto tilt;
            }
            if (motion->accel.z > 0.0F) {
                lever_tilt_x = TILT;
            } else if (motion->accel.z < 0.0F) {
                lever_tilt_x = -TILT;
            } else {
                lever_tilt_x = work->ang.x;
            }
            lever_tilt_z = 0;
        tilt:
            work->mode = HANDLE_TILT;
        }
        break;
    case HANDLE_TILT:
        sound_play_for(SE_TILT, TILT_SOUND_ID, 1, 0, TILT_SOUND_UPDATES);
        work->ang.x = angle_approach(work->ang.x, lever_tilt_x, TILT_SPEED);
        work->ang.z = angle_approach(work->ang.z, lever_tilt_z, TILT_SPEED);
        if ((work->ang.x == lever_tilt_x && work->ang.z == lever_tilt_z) ||
            !(pad_records[0].on & pad_action_buttons)) {
            work->mode = HANDLE_WAIT;
        }
        work->ang.x &= 0xffff;
        work->ang.z &= 0xffff;
        if (event_in_control()) {
            world_tilt_gravity(0, 0);
        } else {
            world_tilt_gravity(work->ang.x, work->ang.z);
        }
        break;
    case HANDLE_STOP:
        break;
    }
    collision_register(work);
    object_clear_touch(work, motion);
    lever_draw(task);
}

/* @hack redundant: register on task, which keeps it in R14 and work in R13
 * (MOV R4,R14 at 0x0c90b648); without it, the two trade and 14 of 90 words
 * differ */
void lever_step(register struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, LEVER_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&lever_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case LEVER_INIT:
            work->mode = LEVER_RUN;
            VARS(work)->stand = &lever_object;
            lever_add_world(task);
            task->destructor = lever_destroy;
            task->draw = lever_draw;
            task_create_child(TASK_WORK | TASK_MOTION, lever_handle_step, task);
            break;
        case LEVER_RUN:
            lever_update_world(work);
            lever_draw(task);
            task_step_children(task);
            break;
        case LEVER_STOP:
            break;
        }
    }
}
