/* the elevator O SKY EV: a base and a wall with a lift between them. player 0
 * standing on the lift at the bottom sends it up. it waits at the top, then
 * comes back down */
#include <ninja.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg06/elevator.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT elevator_lift_object;
extern NJS_OBJECT elevator_base_object;
extern NJS_OBJECT elevator_wall_object;

/* the square of the distance from the camera or player 0 within which the
 * elevator stays */
#define ELEVATOR_RANGE (1010.0F * 1010.0F)
/* the world tests each part while a player is within this of the part's
 * position */
#define ELEVATOR_NEAR 100.0F
/* each part's world flags. the base and the wall take WORLD_MOVES too, as the
 * lift does */
#define ELEVATOR_FLAGS (WORLD_MEDIUM | WORLD_TURNED | WORLD_MOVES)

/* LIFT_TOP and LIFT_BOTTOM are the world heights the lift stops at. it moves
 * LIFT_SPEED a step, and waits over LIFT_WAIT steps at the top */
#define LIFT_TOP (-117.5F)
#define LIFT_BOTTOM (-214.32F)
#define LIFT_SPEED 1.0F
#define LIFT_WAIT 240

/* the box above the lift in which player 0, on land, counts as standing on it.
 * LIFT_MIDDLE is its middle's height above the lift, and the rest its half
 * sizes */
#define LIFT_MIDDLE 17.56F
#define LIFT_HALF_X 18.14F
#define LIFT_HALF_Y 10.0F
#define LIFT_HALF_Z 16.53F

/* the base's and the wall's modes. in ELEVATOR_GONE their steps do nothing, and
 * no step here sets it */
enum elevator_mode { ELEVATOR_INIT = 0, ELEVATOR_RUN = 1, ELEVATOR_GONE = 5 };
/* the lift's modes. in LIFT_GONE its step does nothing, and no step here sets
 * it */
enum lift_mode {
    LIFT_INIT,
    LIFT_BOTTOM_WAIT,
    LIFT_RISE,
    LIFT_TOP_WAIT,
    LIFT_LOWER,
    LIFT_GONE
};

struct elevator_vars {
    NJS_OBJECT *world; /* the part's, in the collision world */
    NJS_OBJECT *object; /* what the part draws */
};

#define VARS(work) ((struct elevator_vars *)&(work)->vars)

/* put a copy of the part's object in the world where the part stands, without
 * NJD_EVAL_UNIT_POS and NJD_EVAL_UNIT_ANG */
static void elevator_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    VARS(work)->world = world = world_object_alloc();
    world->evalflags = VARS(work)->object->evalflags &
                       ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = VARS(work)->object->model;
    world->child = VARS(work)->object->child;
    world->sibling = VARS(work)->object->sibling;
    world_add(ELEVATOR_FLAGS, task, world);
}

/* move the part's world object to the part's position. the world tests it while
 * a player is within ELEVATOR_NEAR */
static void elevator_update_world(struct task_work *work)
{
    NJS_OBJECT *world = VARS(work)->world;

    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    if (player_in_range(&work->pos, ELEVATOR_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void elevator_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

static void elevator_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->world) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}

static void elevator_destroy(struct task *task)
{
    /* @hack spelling: a copy of task, which the image keeps in a saved register
     * across both calls; without it, 9 of 9 words differ and the function ends
     * 2 bytes early */
    struct task *elevator = task;

    task_destroy_children(elevator);
    elevator_remove(elevator);
}

/* player 0 standing on the lift at the bottom sends it up. at the top it waits
 * over LIFT_WAIT steps, whoever stands on it, then comes back down. it moves by
 * the speed in its task->force, which carries what stands on it */
static void elevator_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_VECTOR to;
    NJS_VECTOR local;
    Angle ang;

    switch (work->mode) {
    case LIFT_INIT:
        work->mode = LIFT_BOTTOM_WAIT;
        VARS(work)->object = &elevator_lift_object;
        elevator_add_world(task);
        task->destructor = elevator_remove;
        task->draw = elevator_draw;
        work->ang.x = work->ang.z = 0;
        work->scl.x = work->scl.z = 0.0F;
        work->scl.y = work->pos.y;
        break;
    case LIFT_BOTTOM_WAIT:
        elevator_update_world(work);
        elevator_draw(task);
        to.x = player_work[0]->pos.x - work->pos.x;
        to.y = player_work[0]->pos.y - work->pos.y - LIFT_MIDDLE;
        to.z = player_work[0]->pos.z - work->pos.z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, 0x10000 - work->ang.y, ang);
        njCalcVector(NULL, &to, &local);
        njPopMatrix(1);
        if ((player_work[0]->flags & PLAYER_ON_LAND) &&
            njAbs(local.x) < LIFT_HALF_X && njAbs(local.y) < LIFT_HALF_Y &&
            njAbs(local.z) < LIFT_HALF_Z) {
            work->mode = LIFT_RISE;
        }
        break;
    case LIFT_RISE:
        force->speed.y = LIFT_SPEED;
        work->pos.y += force->speed.y;
        if (work->pos.y > LIFT_TOP) {
            force->speed.y = 0.0F;
            work->scl.y = LIFT_TOP;
            work->mode = LIFT_TOP_WAIT;
        }
        elevator_update_world(work);
        elevator_draw(task);
        break;
    case LIFT_TOP_WAIT:
        if (TASK_TIMER(work)++ > LIFT_WAIT) {
            work->timer = 0;
            work->mode = LIFT_LOWER;
        }
        elevator_update_world(work);
        elevator_draw(task);
        break;
    case LIFT_LOWER:
        force->speed.y = -LIFT_SPEED;
        work->pos.y += force->speed.y;
        if (work->pos.y < LIFT_BOTTOM) {
            force->speed.y = 0.0F;
            work->scl.y = LIFT_BOTTOM;
            work->mode = LIFT_BOTTOM_WAIT;
        }
        elevator_update_world(work);
        elevator_draw(task);
        break;
    case LIFT_GONE:
        break;
    }
}

static void elevator_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ELEVATOR_INIT:
        work->mode = ELEVATOR_RUN;
        VARS(work)->object = &elevator_wall_object;
        elevator_add_world(task);
        task->destructor = elevator_remove;
        task->draw = elevator_draw;
        work->ang.x = work->ang.z = 0;
        break;
    case ELEVATOR_RUN:
        elevator_update_world(work);
        elevator_draw(task);
        break;
    case ELEVATOR_GONE:
        break;
    }
}

void elevator_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, ELEVATOR_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&elevator_lift_object);
        njEasyDrawObject(&elevator_base_object);
        njEasyDrawObject(&elevator_wall_object);
        njPopMatrix(1);
        return;
    } else {
        /* @hack redundant: the else after the return, which gives the cases'
         * scratch values R3 and R2 as the image holds them (0x0c90ad2c to
         * 0x0c90ad74); without it, the two trade and 17 of 104 words differ */
        switch (work->mode) {
        case ELEVATOR_INIT:
            work->mode = ELEVATOR_RUN;
            VARS(work)->object = &elevator_base_object;
            elevator_add_world(task);
            task->destructor = elevator_destroy;
            task->draw = elevator_draw;
            work->ang.x = work->ang.z = 0;
            task_create_child(TASK_WORK | TASK_FORCE, elevator_lift_step, task);
            task_create_child(TASK_WORK | TASK_FORCE, elevator_wall_step, task);
            break;
        case ELEVATOR_RUN:
            elevator_update_world(work);
            elevator_draw(task);
            task_step_children(task);
            break;
        case ELEVATOR_GONE:
            break;
        }
    }
}
