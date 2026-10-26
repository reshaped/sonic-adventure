/* O BLIFT, a lift that rides up once player 0 stands on it, and comes back
 * down once player 0 steps off */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/blift.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT blift_object;
extern NJS_OBJECT blift_solid_object;

#define SOLID_FLAGS (WORLD_HUGE | WORLD_MOVES)

/* the collision world tests the surface while a player is within the
 * surface model's radius of the lift, plus TESTED_REACH */
#define TESTED_REACH 10.0F

struct blift_vars {
    NJS_OBJECT *solid; /* in the collision world */
    /* the object whose model, evalflags, scale and offset solid takes */
    NJS_OBJECT *from;
};

#define VARS(work) ((struct blift_vars *)&(work)->vars)
/* whether player 0 stood on the lift when the step last looked. leaving the
 * lift at the top sets it too, so the lift rides back down on its own */
#define STOOD_ON(work) ((work)->tick)
/* the way the last ride went, a blift_went */
#define WENT(work) ((work)->submode)
/* the set entry's scale x and y: how far the lift rides a step, and the
 * steps a ride takes */
#define LIFT_SPEED(work) ((work)->scl.x)
#define LIFT_STEPS(work) ((work)->scl.y)
/* what LIFT_SPEED and LIFT_STEPS take where the set entry's value isn't
 * above 0 */
#define DEFAULT_SPEED 1.0F
#define DEFAULT_STEPS 180.0F

/* the steps the lift rests at the end of a ride */
#define BLIFT_REST 10

#define SE_LIFT_STOP 559
#define SE_LIFT_MOVE 560

enum blift_mode {
    BLIFT_WAIT = 0,
    BLIFT_UP = 1,
    BLIFT_DOWN = 2,
    BLIFT_LEAVE = 5, /* until player 0 steps off */
    BLIFT_REST_END = 6 /* for BLIFT_REST steps */
};
/* WENT holds WENT_NOWHERE during a ride */
enum blift_went { WENT_NOWHERE, WENT_UP, WENT_DOWN };

static void blift_step(struct task *task);
static void blift_draw(struct task *task);
static void blift_destroy(struct task *task);

/* nothing reads these two shapes, as the lift calls no collision_init. the
 * image holds them in this source's data, at 0x0ca18aa0 */
static struct collision_shape blift_shapes[] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 7.0F, -14.0F},
     14.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 7.0F, -14.0F},
     20.0F,
     3.0F,
     14.0F,
     {0},
     0,
     NULL},
};

/* set the lift at rest with its first ride up, and put its surface in the
 * collision world unless the set editor holds the task */
void egg_carrier_blift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *solid;

    work->ang.x = 0;
    work->ang.z = 0;
    if (!(LIFT_SPEED(work) > 0.0F)) {
        LIFT_SPEED(work) = DEFAULT_SPEED;
    }
    if (!(LIFT_STEPS(work) > 0.0F)) {
        LIFT_STEPS(work) = DEFAULT_STEPS;
    }
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    work->mode = BLIFT_WAIT;
    WENT(work) = WENT_DOWN;
    STOOD_ON(work) = 0;
    work->timer = 0;
    if (!placement_editing(task)) {
        VARS(work)->from = &blift_solid_object;
        solid = world_object_alloc();
        VARS(work)->solid = solid;
        solid->evalflags = VARS(work)->from->evalflags;
        solid->scl[0] = VARS(work)->from->scl[0];
        solid->scl[1] = VARS(work)->from->scl[1];
        solid->scl[2] = VARS(work)->from->scl[2];
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x + VARS(work)->from->pos[0];
        solid->pos[1] = work->pos.y + VARS(work)->from->pos[1] - 3.0F;
        solid->pos[2] = work->pos.z + VARS(work)->from->pos[2];
        solid->model = VARS(work)->from->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(SOLID_FLAGS, task, solid);
    }
    task->step = blift_step;
    task->draw = blift_draw;
    task->destructor = blift_destroy;
}

/* start a ride once player 0 stands on the lift, the other way from the
 * last ride. at the ride's end, the lift rests BLIFT_REST steps, then waits
 * for player 0 to step off. once player 0 has stepped off at the top, the
 * lift rides back down on its own */
static void blift_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid = VARS(work)->solid;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        if (player_in_range(&work->pos, solid->model->r + TESTED_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    switch (work->mode) {
    case BLIFT_WAIT:
        if (placement_editing(task)) {
            break;
        }
        if (STOOD_ON(work)) {
            STOOD_ON(work) = 0;
            work->timer = 0;
            if (WENT(work) == WENT_UP) {
                work->mode = BLIFT_DOWN;
                WENT(work) = WENT_NOWHERE;
                ((struct task_force *)task->force)->speed.x = -0.0F;
                ((struct task_force *)task->force)->speed.y = -LIFT_SPEED(work);
                ((struct task_force *)task->force)->speed.z = -0.0F;
            } else {
                work->mode = BLIFT_UP;
                WENT(work) = WENT_NOWHERE;
                ((struct task_force *)task->force)->speed.x = 0.0F;
                ((struct task_force *)task->force)->speed.y = LIFT_SPEED(work);
                ((struct task_force *)task->force)->speed.z = 0.0F;
            }
        } else {
            STOOD_ON(work) = player_stands_on(0, task);
        }
        break;
    case BLIFT_UP:
        work->timer++;
        if (TASK_TIMER(work) >= (Uint16)LIFT_STEPS(work)) {
            ((struct task_force *)task->force)->speed.x = 0.0F;
            ((struct task_force *)task->force)->speed.y = 0.0F;
            ((struct task_force *)task->force)->speed.z = 0.0F;
            work->mode = BLIFT_REST_END;
            WENT(work) = WENT_UP;
            work->timer = 0;
            sound_play(SE_LIFT_STOP, 0, 0, 0);
        }
        /* @hack redundant: the moves by 0 along x and z, FADDs of FR15, which
         * FLDI0 sets at 0x0c925244 (0x0c92538a to 0x0c9253ba); without them,
         * 185 of 334 words differ */
        work->pos.x += 0.0F;
        work->pos.y += LIFT_SPEED(work);
        work->pos.z += 0.0F;
        solid->pos[0] += 0.0F;
        solid->pos[1] += LIFT_SPEED(work);
        solid->pos[2] += 0.0F;
        sound_play_for(SE_LIFT_MOVE, (int)task, 1, 0, 2);
        break;
    case BLIFT_DOWN:
        work->timer++;
        if (TASK_TIMER(work) >= (Uint16)LIFT_STEPS(work)) {
            ((struct task_force *)task->force)->speed.x = 0.0F;
            ((struct task_force *)task->force)->speed.y = 0.0F;
            ((struct task_force *)task->force)->speed.z = 0.0F;
            work->mode = BLIFT_REST_END;
            WENT(work) = WENT_DOWN;
            work->timer = 0;
            sound_play(SE_LIFT_STOP, 0, 0, 0);
        }
        /* @hack redundant: the moves by 0 along x and z, FSUBs of FR15
         * (0x0c925406 to 0x0c925436); without them, 158 of 334 words
         * differ */
        work->pos.x -= 0.0F;
        work->pos.y -= LIFT_SPEED(work);
        work->pos.z -= 0.0F;
        solid->pos[0] -= 0.0F;
        solid->pos[1] -= LIFT_SPEED(work);
        solid->pos[2] -= 0.0F;
        sound_play_for(SE_LIFT_MOVE, (int)task, 1, 0, 2);
        break;
    case BLIFT_REST_END:
        work->timer++;
        if (TASK_TIMER(work) >= BLIFT_REST) {
            work->timer = 0;
            work->mode = BLIFT_LEAVE;
            STOOD_ON(work) = player_stands_on(0, task);
        }
        break;
    case BLIFT_LEAVE:
        if (placement_editing(task)) {
            break;
        }
        if (STOOD_ON(work)) {
            /* @hack redundant: a 0 for the call below to overwrite, which
             * the image stores (MOV.B R0,@(3,R14) at 0x0c9254b8); without
             * it, 59 of 334 words differ */
            STOOD_ON(work) = 0;
            STOOD_ON(work) = player_stands_on(0, task);
        } else {
            work->mode = BLIFT_WAIT;
            STOOD_ON(work) = 0;
            /* a lift that went up rides back down on the next step,
             * ridden or not */
            if (WENT(work) == WENT_UP) {
                STOOD_ON(work) = 1;
            }
        }
        break;
    }
    blift_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> LIFT SPEED");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> MOVE COUNT");
    }
}

static void blift_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&blift_object);
    njPopMatrix(1);
}

static void blift_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}
