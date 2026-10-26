/* folding door of four panels that opens when a player touches it, which no
 * object list names */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/folding_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the door's action, which the hotel doors play too */
/* @hack type: the action declared as an array. SHC then pools its address and
 * reads its motion through it, MOV.L @(4,R3),R2 at 0x0c902b54. declared as one
 * NJS_ACTION, SHC pools the motion field's address, and 9 of
 * folding_door_turn's 85 words, 5 of folding_door_close's 49 and 3 of
 * folding_door_open's 54 differ */
extern NJS_ACTION folding_door_action[];

#define DOOR_SPEED 0.2F /* motion frames a step */
#define DOOR_WAIT 90 /* steps the door stays fully open */
#define DOOR_TURN 12743.0F /* each panel's turn when fully open, 70 degrees */
#define PANEL_HALF 6.25F /* half a panel's width */
/* half the four panels' width, where each pair hinges */
#define DOOR_HALF 25.0F

struct folding_door_vars {
    Float frame; /* the door's frame in its motion, 0 shut */
};

#define VARS(work) ((struct folding_door_vars *)&(work)->vars)

static void folding_door_close(struct task *task);
static void folding_door_wait(struct task *task);
static void folding_door_open(struct task *task);
static void folding_door_idle(struct task *task);

/* shapes of the four panels, from the left */
/* clang-format off */
static struct collision_shape folding_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-18.75F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-6.25F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {6.25F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {18.75F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

static Bool folding_door_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

static void folding_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(folding_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* turn each panel's shape as far as frame has folded the door, each pair toward
 * its own side. then register the record for this step's checks. x is a panel's
 * half width along the doorway, so an outer panel's center lies x in from its
 * hinge, and an inner one's 3x */
static void folding_door_turn(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    Angle ang = floorf(VARS(work)->frame * DOOR_TURN /
                       (folding_door_action->motion->nbFrame - 1));
    Float x = njCos(ang) * PANEL_HALF;
    Float z = njSin(ang) * PANEL_HALF;

    shapes[3].center.x = -(shapes[0].center.x = x - DOOR_HALF);
    shapes[2].center.x = -(shapes[1].center.x = (x * 3.0F) - DOOR_HALF);
    shapes[0].center.z = shapes[1].center.z = shapes[2].center.z =
        shapes[3].center.z = z;
    shapes[2].ang.y = shapes[0].ang.y = -ang;
    shapes[3].ang.y = shapes[1].ang.y = ang;
    collision_register(work);
}

/* fold the door shut, and open it again at a player's touch */
static void folding_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last set and never read, which loads the motion's frame
     * count and converts it ahead of the range check (FLOAT at 0x0c902c58);
     * without it, 49 of 49 words differ */
    Float last = folding_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    folding_door_draw(task);
    if (!folding_door_touched(work)) {
        /* @hack spelling: the frame decremented in the test, which SHC tests in
         * FR2 and stores in the slot of the BT/S at 0x0c902c80. as a statement
         * of its own, SHC reads the frame back to test it, and 29 of 49 words
         * differ */
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = folding_door_idle;
        }
    } else {
        task->step = folding_door_open;
    }
    folding_door_turn(work);
}

static void folding_door_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    folding_door_draw(task);
    if (!--work->tick) {
        task->step = folding_door_close;
    }
    folding_door_turn(work);
}

static void folding_door_open(struct task *task)
{
    struct task_work *work = task->work;
    Float last = folding_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    folding_door_draw(task);
    VARS(work)->frame += DOOR_SPEED;
    if (VARS(work)->frame >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = folding_door_wait;
    }
    folding_door_turn(work);
}

/* wait shut until a player touches the door */
static void folding_door_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        folding_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        folding_door_draw(task);
        if (folding_door_touched(work)) {
            task->step = folding_door_open;
        }
        folding_door_turn(work);
    }
}

void folding_door_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    collision_init(task, folding_door_shapes, COLLISION_OBJECT, 1, 4);
    task->step = folding_door_idle;
    task->draw = folding_door_draw;
}
