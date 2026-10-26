/* O URADOOR back door that swings open when a player touches it in the sewer
 * act */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/back_door.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the door's action declared as an array. SHC then pools its
 * address and reads its motion through it, MOV.L @(4,R3),R2 at 0x0c905890.
 * declared as one NJS_ACTION, SHC pools the motion field's address, and 11 of
 * back_door_turn's 57 words, 8 of back_door_close's 49 and 3 of
 * back_door_open's 54 differ */
extern NJS_ACTION back_door_action[];

#define DOOR_SPEED 0.2F /* motion frames a step */
/* the door's turn over its whole motion, a quarter turn */
#define DOOR_TURN 16384.0F
#define DOOR_WAIT 90 /* steps the door stays fully open */
#define DOOR_HALF 5.0F /* half the door's width */

struct back_door_vars {
    Float frame; /* the door's frame in its motion, 0 shut */
};

#define VARS(work) ((struct back_door_vars *)&(work)->vars)

static void back_door_close(struct task *task);
static void back_door_wait(struct task *task);
static void back_door_open(struct task *task);
static void back_door_locked(struct task *task);
static void back_door_idle(struct task *task);

/* clang-format off */
static struct collision_shape back_door_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 10.0F, 0.0F},
    0.5F, 10.0F, DOOR_HALF, {0}, 0, NULL,
};
/* clang-format on */

static Bool back_door_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

static void back_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(back_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* turn the door's shape about its hinge as far as frame has swung the door,
 * and register the record for this step's checks */
static void back_door_turn(struct task_work *work)
{
    struct collision_shape *shape = work->collision->shapes;
    Angle ang;

    ang = floorf(VARS(work)->frame * DOOR_TURN /
                 (back_door_action->motion->nbFrame - 1));
    shape->center.x = -(njSin(ang) * DOOR_HALF);
    shape->center.z = -(njCos(ang) * DOOR_HALF) + DOOR_HALF;
    shape->ang.y = ang;
    collision_register(work);
}

/* swing shut, reopening if a player touches the door on the way */
static void back_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last set and never read, which loads the motion's
     * frame count and converts it ahead of the range check (FLOAT at
     * 0x0c905906); without it, 48 of 49 words differ */
    Float last = back_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    back_door_draw(task);
    if (!back_door_touched(work)) {
        /* @hack spelling: the frame decremented in the test, which SHC tests in
         * FR2 and stores in the slot of the BT/S at 0x0c90592e. as a statement
         * of its own, SHC reads the frame back to test it, and 29 of 49 words
         * differ */
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = back_door_idle;
        }
    } else {
        task->step = back_door_open;
    }
    back_door_turn(work);
}

static void back_door_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    back_door_draw(task);
    if (--work->tick == 0) {
        task->step = back_door_close;
    }
    back_door_turn(work);
}

static void back_door_open(struct task *task)
{
    struct task_work *work = task->work;
    Float last = back_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    back_door_draw(task);
    VARS(work)->frame += DOOR_SPEED;
    if (VARS(work)->frame >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = back_door_wait;
    }
    back_door_turn(work);
}

/* outside the sewer act, keep the door shut */
static void back_door_locked(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        back_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        back_door_draw(task);
        back_door_turn(work);
    }
}

/* wait shut until a player touches the door */
static void back_door_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        back_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        back_door_draw(task);
        if (back_door_touched(work)) {
            task->step = back_door_open;
        }
        back_door_turn(work);
    }
}

static Bool back_door_opens(void)
{
    return (Uint8)stage_get_stage_act() == STATION_SQUARE_GESUI;
}

void back_door_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    collision_init(task, &back_door_shape, COLLISION_OBJECT, 1, 1);
    if (back_door_opens()) {
        task->step = back_door_idle;
    } else {
        task->step = back_door_locked;
    }
    task->draw = back_door_draw;
}
