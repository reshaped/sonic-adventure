/* O ROTDOOR door that swings open as the nearest player comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/rotating_door.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL rotating_door_model;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* how far the collision record reaches from the hinge, far past the 43
 * units the door's box reaches */
#define REACH 10000.0F

/* the door opens while the nearest player is this near the hinge, along the
 * shut door's z axis */
#define NEAR 50.0F

#define SWING_MAX 16384.0F /* a quarter turn, from shut to open */
#define SWING_SPEED 768.0F /* the top speed, in angle units a step */
#define SWING_MIN_SPEED 112.0F /* the speed braking leaves the door */
#define SWING_ACCEL 40.0F /* the speed gained each step until braking */
#define SWING_BRAKE 30.0F /* the speed lost each step braking */
#define BRAKE_OPENING (SWING_MAX * 0.65F) /* where opening starts braking */
#define BRAKE_SHUTTING (SWING_MAX * 0.35F) /* where shutting starts braking */

struct door_vars {
    Float speed; /* the swing's speed, below 0 while it shuts */
    Sint16 shut_y; /* the door's angle y while shut */
    Uint8 unk_06[2];
    Float swing; /* the door's turn from shut */
};

#define VARS(work) ((struct door_vars *)&(work)->vars)

enum door_mode { DOOR_INIT, DOOR_RUN };

static void rotating_door_init(struct task *task);
static void rotating_door_reset(struct task *task);
static void rotating_door_swing(struct task *task);
static void rotating_door_draw(struct task *task);
static void rotating_door_destroy(struct task *task);

/* the door, a box reaching 30 units from the hinge along -x, 30 high and 6
 * thick. it pushes every group, and no group's shapes push it */
static struct collision_shape rotating_door_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {-15.0F, 15.0F, -1.6F},
    15.0F,
    15.0F,
    3.0F,
    {0},
    0,
    NULL,
};

void rotating_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_INIT:
            rotating_door_init(task);
            break;
        case DOOR_RUN:
            rotating_door_draw(task);
            rotating_door_swing(task);
            break;
        }
    } else {
        switch (work->mode) {
        case DOOR_INIT:
            rotating_door_init(task);
            break;
        case DOOR_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            rotating_door_draw(task);
            rotating_door_swing(task);
            break;
        }
    }
}

static void rotating_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = rotating_door_draw;
    task->destructor = rotating_door_destroy;
    rotating_door_reset(task);
    work->mode = DOOR_RUN;
}

static void rotating_door_reset(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &rotating_door_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = REACH;
    VARS(work)->shut_y = work->ang.y;
    VARS(work)->swing = 0.0F;
    VARS(work)->speed = 0.0F;
}

/* swing the door open while the nearest player is within NEAR, and shut
 * otherwise. each way, it speeds up until it passes BRAKE_OPENING or
 * BRAKE_SHUTTING, and then brakes */
static void rotating_door_swing(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_POINT3 local;
    Angle ang;

    pos = player_work[player_nearest(&work->pos)]->pos;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, VARS(work)->shut_y, ang);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &pos, &local);
    njPopMatrix(1);
    work->submode = local.z >= -NEAR && local.z <= NEAR;
    if (work->submode) {
        if (VARS(work)->swing < BRAKE_OPENING) {
            VARS(work)->speed += SWING_ACCEL;
            if (VARS(work)->speed > SWING_SPEED) {
                VARS(work)->speed = SWING_SPEED;
            }
        } else {
            VARS(work)->speed -= SWING_BRAKE;
            if (VARS(work)->speed < SWING_MIN_SPEED) {
                VARS(work)->speed = SWING_MIN_SPEED;
            }
        }
    } else {
        if (VARS(work)->swing > BRAKE_SHUTTING) {
            VARS(work)->speed -= SWING_ACCEL;
            if (VARS(work)->speed < -SWING_SPEED) {
                VARS(work)->speed = -SWING_SPEED;
            }
        } else {
            VARS(work)->speed += SWING_BRAKE;
            if (VARS(work)->speed > -SWING_MIN_SPEED) {
                VARS(work)->speed = -SWING_MIN_SPEED;
            }
        }
    }
    VARS(work)->swing += VARS(work)->speed;
    if (VARS(work)->swing < 0.0F) {
        VARS(work)->swing = 0.0F;
    } else if (VARS(work)->swing > SWING_MAX) {
        VARS(work)->swing = SWING_MAX;
    }
    work->ang.y = (Angle)(VARS(work)->shut_y + VARS(work)->swing);
    collision_register(work);
}

static void rotating_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&rotating_door_model);
        njPopMatrix(1);
    }
}

static void rotating_door_destroy(struct task *task)
{
}
